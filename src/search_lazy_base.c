#include "search_lazy_base.h"

/**
 * Creates a new node from parent and operator.
 */
static plan_state_space_node_t *createNode(plan_search_lazy_base_t *lb,
                                           plan_state_id_t parent_state_id,
                                           plan_op_t *parent_op,
                                           int *ret);

#define LAZYBASE(parent) \
    bor_container_of((parent), plan_search_lazy_base_t, search)

void planSearchLazyBaseInit(plan_search_lazy_base_t *lb,
                            plan_list_lazy_t *list, int list_del,
                            int use_preferred_ops,
                            int cost_type)
{
    lb->list = list;
    lb->list_del = list_del;
    lb->use_preferred_ops = use_preferred_ops;
    lb->cost_type = cost_type;
}

void planSearchLazyBaseFree(plan_search_lazy_base_t *lb)
{
    if (lb->list_del && lb->list)
        planListLazyDel(lb->list);
}

int planSearchLazyBaseInitStep(plan_search_t *search)
{
    plan_search_lazy_base_t *lb = LAZYBASE(search);
    plan_state_id_t init_state;
    plan_state_space_node_t *node;
    plan_cost_t cost;
    int res;

    init_state = search->initial_state;
    node = planStateSpaceNode(search->state_space, init_state);
    planStateSpaceOpen(search->state_space, node);
    planStateSpaceClose(search->state_space, node);
    node->parent_state_id = PLAN_NO_STATE;
    node->op = NULL;
    node->cost = 0;

    res = _planSearchHeuristic(search, init_state, &node->heuristic, NULL);
    if (res != PLAN_SEARCH_CONT)
        return res;

    cost = 0;
    if (lb->cost_type == PLAN_SEARCH_LAZY_BASE_COST_HEUR)
        cost = node->heuristic;
    planListLazyPush(lb->list, cost, init_state, NULL);
    return PLAN_SEARCH_CONT;
}

int planSearchLazyBaseNext(plan_search_lazy_base_t *lb,
                           plan_state_space_node_t **node)
{
    plan_state_id_t parent_state_id;
    plan_op_t *parent_op;
    plan_state_space_node_t *cur_node;
    plan_cost_t h;
    plan_search_applicable_ops_t *pref_ops;
    int ret;

    *node = NULL;

    if (planListLazyPop(lb->list, &parent_state_id, &parent_op) != 0){
        return PLAN_SEARCH_NOT_FOUND;
    }

    if (parent_op){
        cur_node = createNode(lb, parent_state_id, parent_op, &ret);
        if (cur_node == NULL)
            return ret;
    }else{
        cur_node = planStateSpaceNode(lb->search.state_space, parent_state_id);
    }

    if (_planSearchCheckGoal(&lb->search, cur_node)){
        return PLAN_SEARCH_FOUND;
    }
    _planSearchExpandedNode(&lb->search, cur_node);

    if (!parent_op){
        // If the current node wasn't generated we need to find
        // applicable operators and optionally the preferred operators.
        _planSearchFindApplicableOps(&lb->search, cur_node->state_id);
        if (lb->use_preferred_ops){
           pref_ops = &lb->search.app_ops;
           _planSearchHeuristic(&lb->search, cur_node->state_id, &h, pref_ops);
        }
    }

    *node = cur_node;
    return PLAN_SEARCH_CONT;
}

void planSearchLazyBaseExpand(plan_search_lazy_base_t *lb,
                              plan_state_space_node_t *node)
{
    plan_search_t *search = &lb->search;
    plan_cost_t cost = 0;
    int i, op_size;

    op_size = search->app_ops.op_found;
    if (lb->use_preferred_ops == PLAN_SEARCH_PREFERRED_ONLY)
        op_size = search->app_ops.op_preferred;

    if (lb->cost_type == PLAN_SEARCH_LAZY_BASE_COST_HEUR)
        cost = node->heuristic;

    for (i = 0; i < op_size; ++i){
        planListLazyPush(lb->list, cost, node->state_id, search->app_ops.op[i]);
        planSearchStatIncGeneratedStates(&search->stat);
    }
}

void planSearchLazyBaseInsertNode(plan_search_t *search,
                                  plan_state_space_node_t *node)
{
    plan_search_lazy_base_t *lb = LAZYBASE(search);
    plan_cost_t cost = 0;

    if (planStateSpaceNodeIsNew(node)){
        planStateSpaceOpen(search->state_space, node);
        planStateSpaceClose(search->state_space, node);
    }else{
        planStateSpaceReopen(search->state_space, node);
        planStateSpaceClose(search->state_space, node);
    }

    if (lb->cost_type == PLAN_SEARCH_LAZY_BASE_COST_HEUR)
        cost = node->heuristic;
    planListLazyPush(lb->list, cost, node->state_id, NULL);
}

static plan_state_space_node_t *createNode(plan_search_lazy_base_t *lb,
                                           plan_state_id_t parent_state_id,
                                           plan_op_t *parent_op,
                                           int *ret)
{
    plan_search_t *search = &lb->search;
    plan_state_id_t cur_state_id;
    plan_state_space_node_t *cur_node, *parent_node;
    plan_cost_t cur_heur;
    plan_search_applicable_ops_t *pref_ops = NULL;
    int res;

    // Create a new state and check whether the state was already visited
    cur_state_id = planOpApply(parent_op, parent_state_id);
    cur_node = planStateSpaceNode(search->state_space, cur_state_id);
    if (!planStateSpaceNodeIsNew(cur_node)){
        *ret = PLAN_SEARCH_CONT;
        return NULL;
    }

    // find applicable operators in the current state
    _planSearchFindApplicableOps(search, cur_state_id);

    // compute heuristic value for the current node
    if (lb->use_preferred_ops)
        pref_ops = &search->app_ops;
    res = _planSearchHeuristic(search, cur_state_id, &cur_heur, pref_ops);
    if (res != PLAN_SEARCH_CONT){
        *ret = res;
        return NULL;
    }

    // get parent node for path cost computation
    parent_node = planStateSpaceNode(search->state_space, parent_state_id);

    // Update current node's data
    planStateSpaceOpen(search->state_space, cur_node);
    planStateSpaceClose(search->state_space, cur_node);
    cur_node->parent_state_id = parent_state_id;
    cur_node->op = parent_op;
    cur_node->cost = parent_node->cost + parent_op->cost;
    cur_node->heuristic = cur_heur;
    planSearchStatIncExpandedStates(&lb->search.stat);

    return cur_node;
}

