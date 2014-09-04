#include <boruvka/alloc.h>

#include "plan/search.h"

struct _plan_search_lazy_t {
    plan_search_t search;

    plan_list_lazy_t *list; /*!< List to keep track of the states */
    int list_del;           /*!< True if .list should be deleted */
    int use_preferred_ops;  /*!< True if preferred operators from heuristic
                                 should be used. */
    plan_search_applicable_ops_t *pref_ops; /*!< This pointer is set to
                                                 &search->applicable_ops in
                                                 case preferred operators
                                                 are enabled. */
};
typedef struct _plan_search_lazy_t plan_search_lazy_t;

#define SEARCH_FROM_PARENT(parent) \
    bor_container_of((parent), plan_search_lazy_t, search)

static void addSuccessors(plan_search_lazy_t *lazy,
                          plan_state_id_t state_id,
                          plan_cost_t heur_val);

static void planSearchLazyDel(plan_search_t *_lazy);
static int planSearchLazyInit(plan_search_t *_lazy);
static int planSearchLazyStep(plan_search_t *_lazy);
static int planSearchLazyInjectState(plan_search_t *, plan_state_id_t state_id,
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

    lazy->use_preferred_ops = params->use_preferred_ops;
    lazy->list              = params->list;
    lazy->list_del          = params->list_del;

    lazy->pref_ops = NULL;
    if (lazy->use_preferred_ops)
        lazy->pref_ops = &lazy->search.applicable_ops;

    return &lazy->search;
}

static void planSearchLazyDel(plan_search_t *_lazy)
{
    plan_search_lazy_t *lazy = SEARCH_FROM_PARENT(_lazy);
    _planSearchFree(&lazy->search);
    if (lazy->list_del)
        planListLazyDel(lazy->list);
    BOR_FREE(lazy);
}

static int planSearchLazyInit(plan_search_t *_lazy)
{
    plan_search_lazy_t *lazy = SEARCH_FROM_PARENT(_lazy);
    planListLazyPush(lazy->list, 0,
                     lazy->search.params.prob->initial_state, NULL);
    return PLAN_SEARCH_CONT;
}

static int planSearchLazyStep(plan_search_t *_lazy)
{
    plan_search_lazy_t *lazy = SEARCH_FROM_PARENT(_lazy);
    plan_state_id_t parent_state_id, cur_state_id;
    plan_op_t *parent_op;
    plan_cost_t cur_heur;
    plan_state_space_node_t *cur_node;
    int res;

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
        cur_state_id = planOpApply(parent_op, parent_state_id);
    }

    // check whether the state was already visited
    if (!planStateSpaceNodeIsNew2(lazy->search.state_space, cur_state_id))
        return PLAN_SEARCH_CONT;

    // find applicable operators
    _planSearchFindApplicableOps(&lazy->search, cur_state_id);

    // compute heuristic value for the current node
    res = _planSearchHeuristic(&lazy->search, cur_state_id, &cur_heur,
                               lazy->pref_ops);
    if (res != PLAN_SEARCH_CONT)
        return res;

    // open and close the node so we can trace the path from goal to the
    // initial state
    cur_node = _planSearchNodeOpenClose(&lazy->search, cur_state_id,
                                        parent_state_id, parent_op,
                                        0, cur_heur);

    // check if the current state is the goal
    if (_planSearchCheckGoal(&lazy->search, cur_state_id))
        return PLAN_SEARCH_FOUND;

    // Useful only in MA node
    _planSearchMASendState(&lazy->search, cur_node);

    if (cur_heur != PLAN_HEUR_DEAD_END){
        addSuccessors(lazy, cur_state_id, cur_heur);
    }

    return PLAN_SEARCH_CONT;
}

static int planSearchLazyInjectState(plan_search_t *_lazy, plan_state_id_t state_id,
                                     plan_cost_t cost, plan_cost_t heuristic)
{
    plan_search_lazy_t *lazy = SEARCH_FROM_PARENT(_lazy);
    return _planSearchLazyInjectState(&lazy->search, lazy->list,
                                      state_id, cost, heuristic);
}

static void addSuccessors(plan_search_lazy_t *lazy,
                          plan_state_id_t state_id,
                          plan_cost_t heur_val)
{
    if (lazy->use_preferred_ops == PLAN_SEARCH_PREFERRED_ONLY){
        _planSearchAddLazySuccessors(&lazy->search, state_id,
                                     lazy->search.applicable_ops.op,
                                     lazy->search.applicable_ops.op_preferred,
                                     heur_val, lazy->list);
    }else{
        _planSearchAddLazySuccessors(&lazy->search, state_id,
                                     lazy->search.applicable_ops.op,
                                     lazy->search.applicable_ops.op_found,
                                     heur_val, lazy->list);
    }
}

