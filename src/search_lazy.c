#include <boruvka/alloc.h>

#include "plan/search.h"

struct _plan_search_lazy_t {
    plan_search_t search;

    plan_list_lazy_t *list; /*!< List to keep track of the states */
    int list_del;           /*!< True if .list should be deleted */
    plan_heur_t *heur;      /*!< Heuristic function */
    int heur_del;           /*!< True if .heur should be deleted */
};
typedef struct _plan_search_lazy_t plan_search_lazy_t;

static void planSearchLazyDel(void *_lazy);
static int planSearchLazyInit(void *_lazy);
static int planSearchLazyStep(void *_lazy,
                              plan_search_step_change_t *change);
static int planSearchLazyInjectState(void *, plan_state_id_t state_id,
                                     plan_cost_t cost, plan_cost_t heuristic);

void planSearchLazyParamsInit(plan_search_lazy_params_t *p)
{
    bzero(p, sizeof(*p));
    planSearchParamsInit(&p->search);
}

plan_search_t *planSearchLazyNew(const plan_search_lazy_params_t *params)
{
    plan_search_lazy_t *lazy;

    lazy = BOR_ALLOC(plan_search_lazy_t);
    _planSearchInit(&lazy->search, &params->search,
                    planSearchLazyDel,
                    planSearchLazyInit,
                    planSearchLazyStep,
                    planSearchLazyInjectState);

    lazy->heur     = params->heur;
    lazy->heur_del = params->heur_del;
    lazy->list     = params->list;
    lazy->list_del = params->list_del;

    return &lazy->search;
}

static void planSearchLazyDel(void *_lazy)
{
    plan_search_lazy_t *lazy = _lazy;
    _planSearchFree(&lazy->search);
    if (lazy->heur_del)
        planHeurDel(lazy->heur);
    if (lazy->list_del)
        planListLazyDel(lazy->list);
    BOR_FREE(lazy);
}

static int planSearchLazyInit(void *_lazy)
{
    plan_search_lazy_t *lazy = _lazy;
    planListLazyPush(lazy->list, 0,
                     lazy->search.params.prob->initial_state, NULL);
    return PLAN_SEARCH_CONT;
}

static int planSearchLazyStep(void *_lazy,
                              plan_search_step_change_t *change)
{
    plan_search_lazy_t *lazy = _lazy;
    plan_state_id_t parent_state_id, cur_state_id;
    plan_operator_t *parent_op;
    plan_cost_t cur_heur;
    plan_state_space_node_t *node;

    if (change)
        planSearchStepChangeReset(change);

    // get next node from the list
    if (planListLazyPop(lazy->list, &parent_state_id, &parent_op) != 0){
        // reached dead-end
        return PLAN_SEARCH_NOT_FOUND;
    }

    planSearchStatIncExpandedStates(&lazy->search.stat);

    if (parent_op == NULL){
        // use parent state as current state
        cur_state_id = parent_state_id;
        parent_state_id = PLAN_NO_STATE;
    }else{
        // create a new state
        cur_state_id = planOperatorApply(parent_op, parent_state_id);
    }

    // check whether the state was already visited
    if (!planStateSpaceNodeIsNew2(lazy->search.state_space, cur_state_id))
        return PLAN_SEARCH_CONT;

    // compute heuristic value for the current node
    cur_heur = _planSearchHeuristic(&lazy->search, cur_state_id, lazy->heur);

    // open and close the node so we can trace the path from goal to the
    // initial state
    node = _planSearchNodeOpenClose(&lazy->search, cur_state_id,
                                    parent_state_id, parent_op,
                                    0, cur_heur);
    if (change)
        planSearchStepChangeAddClosedNode(change, node);

    // check if the current state is the goal
    if (_planSearchCheckGoal(&lazy->search, cur_state_id))
        return PLAN_SEARCH_FOUND;

    _planSearchAddLazySuccessors(&lazy->search, cur_state_id,
                                 cur_heur, lazy->list);

    return PLAN_SEARCH_CONT;
}

static int planSearchLazyInjectState(void *_lazy, plan_state_id_t state_id,
                                     plan_cost_t cost, plan_cost_t heuristic)
{
    plan_search_lazy_t *lazy = _lazy;
    return _planSearchLazyInjectState(&lazy->search, lazy->heur, lazy->list,
                                      state_id, cost, heuristic);
}
