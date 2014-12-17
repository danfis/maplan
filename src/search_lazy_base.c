#include "search_lazy_base.h"

void planSearchLazyBaseInit(plan_search_lazy_base_t *lb,
                            plan_list_lazy_t *list, int list_del,
                            int use_preferred_ops)
{
    lb->list = list;
    lb->list_del = list_del;
    lb->use_preferred_ops = use_preferred_ops;
}

void planSearchLazyBaseFree(plan_search_lazy_base_t *lb)
{
    if (lb->list_del && lb->list)
        planListLazyDel(lb->list);
}

int planSearchLazyBaseInitStep(plan_search_lazy_base_t *lb, int heur_as_cost)
{
    plan_search_t *search = &lb->search;
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
    if (heur_as_cost)
        cost = node->heuristic;
    planListLazyPush(lb->list, cost, init_state, NULL);
    return PLAN_SEARCH_CONT;
}

plan_state_space_node_t *planSearchLazyBaseExpandNode(
            plan_search_lazy_base_t *lb,
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

void planSearchLazyBaseInsertNode(plan_search_lazy_base_t *lb,
                                  plan_state_space_node_t *node,
                                  plan_cost_t cost)
{
    plan_search_t *search = &lb->search;

    if (planStateSpaceNodeIsNew(node)){
        planStateSpaceOpen(search->state_space, node);
        planStateSpaceClose(search->state_space, node);
    }else{
        planStateSpaceReopen(search->state_space, node);
        planStateSpaceClose(search->state_space, node);
    }
    planListLazyPush(lb->list, cost, node->state_id, NULL);
}

void planSearchLazyBaseAddSuccessors(plan_search_lazy_base_t *lb,
                                     plan_state_id_t state_id,
                                     plan_cost_t cost)
{
    plan_search_t *search = &lb->search;
    int i, op_size;

    op_size = search->app_ops.op_found;
    if (lb->use_preferred_ops == PLAN_SEARCH_PREFERRED_ONLY)
        op_size = search->app_ops.op_preferred;

    for (i = 0; i < op_size; ++i){
        planListLazyPush(lb->list, cost, state_id, search->app_ops.op[i]);
        planSearchStatIncGeneratedStates(&search->stat);
    }
}
