#include <boruvka/alloc.h>

#include "plan/search.h"

struct _plan_search_lazy_t {
    plan_search_t search;

    plan_list_lazy_t *list;          /*!< List to keep track of the states */
    plan_heur_t *heur;               /*!< Heuristic function */
};
typedef struct _plan_search_lazy_t plan_search_lazy_t;

static void planSearchLazyDel(void *_lazy);
static int planSearchLazyInit(void *_lazy);
static int planSearchLazyStep(void *_lazy);

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
                    planSearchLazyStep);

    lazy->heur = params->heur;
    lazy->list = params->list;

    return &lazy->search;
}

static void planSearchLazyDel(void *_lazy)
{
    plan_search_lazy_t *lazy = _lazy;
    _planSearchFree(&lazy->search);
    BOR_FREE(lazy);
}

static int planSearchLazyInit(void *_lazy)
{
    plan_search_lazy_t *lazy = _lazy;
    planListLazyPush(lazy->list, 0,
                     lazy->search.params.prob->initial_state, NULL);
    return PLAN_SEARCH_CONT;
}

static int planSearchLazyStep(void *_lazy)
{
    plan_search_lazy_t *lazy = _lazy;
    plan_state_id_t parent_state_id, cur_state_id;
    plan_operator_t *parent_op;
    plan_cost_t cur_heur;

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
    _planSearchNodeOpenClose(&lazy->search, cur_state_id,
                                   parent_state_id, parent_op,
                                   0, cur_heur);

    // check if the current state is the goal
    if (_planSearchCheckGoal(&lazy->search, cur_state_id))
        return PLAN_SEARCH_FOUND;

    _planSearchAddLazySuccessors(&lazy->search, cur_state_id,
                                 cur_heur, lazy->list);

    return PLAN_SEARCH_CONT;
}
