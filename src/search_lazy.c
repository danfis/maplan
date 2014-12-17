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

#define LAZY(parent) \
    bor_container_of((parent), plan_search_lazy_t, search)

static void addSuccessors(plan_search_lazy_t *lazy,
                          plan_state_id_t state_id,
                          plan_cost_t heur_val);

static void planSearchLazyDel(plan_search_t *_lazy);
static int planSearchLazyInit(plan_search_t *_lazy);
static int planSearchLazyStep(plan_search_t *_lazy);
static void planSearchLazyInsertNode(plan_search_t *search,
                                     plan_state_space_node_t *node);

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
                    planSearchLazyInsertNode,
                    NULL);

    lazy->use_preferred_ops = params->use_preferred_ops;
    lazy->list              = params->list;
    lazy->list_del          = params->list_del;

    lazy->pref_ops = NULL;
    if (lazy->use_preferred_ops)
        lazy->pref_ops = &lazy->search.app_ops;

    return &lazy->search;
}

static void planSearchLazyDel(plan_search_t *_lazy)
{
    plan_search_lazy_t *lazy = LAZY(_lazy);
    _planSearchFree(&lazy->search);
    if (lazy->list_del)
        planListLazyDel(lazy->list);
    BOR_FREE(lazy);
}

static int planSearchLazyInit(plan_search_t *search)
{
    plan_search_lazy_t *lazy = LAZY(search);
    plan_state_space_node_t *node;
    int res;

    node = planStateSpaceNode(search->state_space, search->initial_state);
    planStateSpaceOpen(search->state_space, node);
    planStateSpaceClose(search->state_space, node);
    node->parent_state_id = PLAN_NO_STATE;
    node->op = NULL;
    node->cost = 0;

    res = _planSearchHeuristic(search, node->state_id, &node->heuristic, NULL);
    if (res != PLAN_SEARCH_CONT)
        return res;

    planListLazyPush(lazy->list, node->heuristic, node->state_id, NULL);
    return PLAN_SEARCH_CONT;
}

static plan_state_space_node_t *expandNode(plan_search_lazy_t *lazy,
                                           plan_state_id_t parent_state_id,
                                           plan_op_t *parent_op,
                                           int *r)
{
    plan_state_id_t cur_state_id;
    plan_state_space_node_t *cur_node, *parent_node;
    plan_cost_t cur_heur;
    int res;

    // Create a new state and check whether the state was already visited
    cur_state_id = planOpApply(parent_op, parent_state_id);
    cur_node = planStateSpaceNode(lazy->search.state_space, cur_state_id);
    if (!planStateSpaceNodeIsNew(cur_node)){
        *r = PLAN_SEARCH_CONT;
        return NULL;
    }

    // find applicable operators in the current state
    _planSearchFindApplicableOps(&lazy->search, cur_state_id);

    // compute heuristic value for the current node
    res = _planSearchHeuristic(&lazy->search, cur_state_id,
                               &cur_heur, lazy->pref_ops);
    if (res != PLAN_SEARCH_CONT){
        *r = res;
        return NULL;
    }

    // get parent node for path cost computation
    parent_node = planStateSpaceNode(lazy->search.state_space, parent_state_id);

    // Update current node's data
    planStateSpaceOpen(lazy->search.state_space, cur_node);
    planStateSpaceClose(lazy->search.state_space, cur_node);
    cur_node->parent_state_id = parent_state_id;
    cur_node->op = parent_op;
    cur_node->cost = parent_node->cost + parent_op->cost;
    cur_node->heuristic = cur_heur;
    planSearchStatIncExpandedStates(&lazy->search.stat);

    return cur_node;
}

static int planSearchLazyStep(plan_search_t *search)
{
    plan_search_lazy_t *lazy = LAZY(search);
    plan_state_id_t parent_state_id;
    plan_op_t *parent_op;
    plan_state_space_node_t *cur_node;
    int res;

    // get next node from the list
    if (planListLazyPop(lazy->list, &parent_state_id, &parent_op) != 0)
        return PLAN_SEARCH_NOT_FOUND;

    if (parent_op){
        cur_node = expandNode(lazy, parent_state_id, parent_op, &res);
        if (cur_node == NULL)
            return res;

    }else{
        cur_node = planStateSpaceNode(search->state_space, parent_state_id);
    }

    // check if the current state is the goal
    if (_planSearchCheckGoal(&lazy->search, cur_node))
        return PLAN_SEARCH_FOUND;
    _planSearchExpandedNode(search, cur_node);

    if (cur_node->heuristic != PLAN_HEUR_DEAD_END){
        if (!parent_op){
            _planSearchFindApplicableOps(search, cur_node->state_id);
            if (lazy->pref_ops){
                plan_cost_t h;
                _planSearchHeuristic(&lazy->search, cur_node->state_id,
                                     &h, lazy->pref_ops);
            }
        }
        addSuccessors(lazy, cur_node->state_id, cur_node->heuristic);
    }

    return PLAN_SEARCH_CONT;
}

static void planSearchLazyInsertNode(plan_search_t *search,
                                     plan_state_space_node_t *node)
{
    plan_search_lazy_t *lazy = LAZY(search);

    if (planStateSpaceNodeIsNew(node)){
        planStateSpaceOpen(search->state_space, node);
        planStateSpaceClose(search->state_space, node);
    }else{
        planStateSpaceReopen(search->state_space, node);
        planStateSpaceClose(search->state_space, node);
    }

    planListLazyPush(lazy->list, node->heuristic, node->state_id, NULL);
}


static void addSuccessors(plan_search_lazy_t *lazy,
                          plan_state_id_t state_id,
                          plan_cost_t heur_val)
{
    if (lazy->use_preferred_ops == PLAN_SEARCH_PREFERRED_ONLY){
        _planSearchAddLazySuccessors(&lazy->search, state_id,
                                     lazy->search.app_ops.op,
                                     lazy->search.app_ops.op_preferred,
                                     heur_val, lazy->list);
    }else{
        _planSearchAddLazySuccessors(&lazy->search, state_id,
                                     lazy->search.app_ops.op,
                                     lazy->search.app_ops.op_found,
                                     heur_val, lazy->list);
    }
}

