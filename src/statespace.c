#include <boruvka/alloc.h>

#include "plan/statespace.h"

void planStateSpaceNodeInit(plan_state_space_node_t *n)
{
    n->state_id        = PLAN_NO_STATE;
    n->parent_state_id = PLAN_NO_STATE;
    n->state           = PLAN_STATE_SPACE_NODE_NEW;
    n->op              = NULL;
    n->cost            = -1;
    n->heuristic       = -1;
}

void planStateSpaceInit(plan_state_space_t *state_space,
                        plan_state_pool_t *state_pool,
                        size_t node_size,
                        plan_data_arr_el_init_fn init_fn,
                        const void *init_data,
                        plan_state_space_pop_fn fn_pop,
                        plan_state_space_insert_fn fn_insert,
                        plan_state_space_clear_fn fn_clear,
                        plan_state_space_close_all_fn fn_close_all)
{

    state_space->state_pool = state_pool;
    state_space->data_id = planStatePoolDataReserve(state_pool, node_size,
                                                    init_fn, init_data);

    state_space->fn_pop       = fn_pop;
    state_space->fn_insert    = fn_insert;
    state_space->fn_clear     = fn_clear;
    state_space->fn_close_all = fn_close_all;
}

void planStateSpaceFree(plan_state_space_t *ss)
{
}

plan_state_space_node_t *planStateSpaceNode(plan_state_space_t *ss,
                                            plan_state_id_t state_id)
{
    plan_state_space_node_t *n;
    n = planStatePoolData(ss->state_pool, ss->data_id, state_id);
    n->state_id = state_id;
    return n;
}

plan_state_space_node_t *planStateSpacePop(plan_state_space_t *ss)
{
    plan_state_space_node_t *node;
    node = ss->fn_pop(ss);
    if (!node)
        return NULL;

    node->state = PLAN_STATE_SPACE_NODE_CLOSED;
    return node;
}

int planStateSpaceOpen(plan_state_space_t *ss,
                       plan_state_space_node_t *node)
{
    if (!planStateSpaceNodeIsNew(node))
        return -1;

    ss->fn_insert(ss, node);
    node->state = PLAN_STATE_SPACE_NODE_OPEN;
    return 0;
}

plan_state_space_node_t *planStateSpaceOpen2(plan_state_space_t *ss,
                                             plan_state_id_t state_id,
                                             plan_state_id_t parent_state_id,
                                             plan_operator_t *op,
                                             unsigned cost,
                                             unsigned heuristic)
{
    plan_state_space_node_t *node;

    node = planStateSpaceNode(ss, state_id);
    if (!planStateSpaceNodeIsNew(node))
        return NULL;

    node->parent_state_id = parent_state_id;
    node->op              = op;
    node->cost            = cost;
    node->heuristic       = heuristic;

    planStateSpaceOpen(ss, node);

    return node;
}

void planStateSpaceClear(plan_state_space_t *ss)
{
    plan_state_space_node_t *node;

    if (ss->fn_clear){
        ss->fn_clear(ss);
    }else{
        while ((node = planStateSpacePop(ss)) != NULL){
            node->state = PLAN_STATE_SPACE_NODE_NEW;
        }
    }
}

void planStateSpaceCloseAll(plan_state_space_t *ss)
{
    if (ss->fn_close_all){
        ss->fn_close_all(ss);
    }else{
        while (planStateSpacePop(ss) != NULL);
    }
}
