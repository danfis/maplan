#include <boruvka/alloc.h>
#include "plan/statespace_fifo.h"

plan_state_space_fifo_t *planStateSpaceFifoNew(plan_state_pool_t *state_pool)
{
    plan_state_space_fifo_t *ss;
    plan_state_space_fifo_node_t node_init;

    ss = BOR_ALLOC(plan_state_space_fifo_t);
    ss->state_pool = state_pool;

    node_init.state_id        = PLAN_NO_STATE;
    node_init.parent_state_id = PLAN_NO_STATE;
    node_init.op              = NULL;
    node_init.state           = PLAN_STATE_SPACE_NODE_NEW;
    borListInit(&node_init.fifo);
    ss->data_id = planStatePoolDataReserve(ss->state_pool,
                                           sizeof(plan_state_space_fifo_node_t),
                                           &node_init);

    borListInit(&ss->fifo);

    return ss;
}

void planStateSpaceFifoDel(plan_state_space_fifo_t *ss)
{
    BOR_FREE(ss);
}

plan_state_space_fifo_node_t *planStateSpaceFifoNode(plan_state_space_fifo_t *ss,
                                                     plan_state_id_t state_id)
{
    plan_state_space_fifo_node_t *n;
    n = planStatePoolData(ss->state_pool, ss->data_id, state_id);
    n->state_id = state_id;
    return n;
}

plan_state_space_fifo_node_t *planStateSpaceFifoPop(plan_state_space_fifo_t *ss)
{
    bor_list_t *item;
    plan_state_space_fifo_node_t *node;

    if (borListEmpty(&ss->fifo))
        return NULL;

    item = borListNext(&ss->fifo);
    borListDel(item);
    node = BOR_LIST_ENTRY(item, plan_state_space_fifo_node_t, fifo);
    node->state = PLAN_STATE_SPACE_NODE_CLOSED;
    return node;
}

int planStateSpaceFifoOpen(plan_state_space_fifo_t *ss,
                           plan_state_space_fifo_node_t *node)
{
    if (!planStateSpaceFifoNodeIsNew(node))
        return -1;

    node->state = PLAN_STATE_SPACE_NODE_OPEN;
    borListAppend(&ss->fifo, &node->fifo);
    return 0;
}

plan_state_space_fifo_node_t *planStateSpaceFifoOpen2(
                plan_state_space_fifo_t *ss,
                plan_state_id_t state_id,
                plan_state_id_t parent_state_id,
                plan_operator_t *op)
{
    plan_state_space_fifo_node_t *node;

    node = planStateSpaceFifoNode(ss, state_id);
    if (!planStateSpaceFifoNodeIsNew(node))
        return NULL;

    node->parent_state_id = parent_state_id;
    node->op              = op;

    planStateSpaceFifoOpen(ss, node);

    return node;
}

void planStateSpaceFifoClear(plan_state_space_fifo_t *ss)
{
    plan_state_space_fifo_node_t *node;

    while ((node = planStateSpaceFifoPop(ss)) != NULL){
        node->state = PLAN_STATE_SPACE_NODE_NEW;
    }
}

void planStateSpaceFifoCloseAll(plan_state_space_fifo_t *ss)
{
    while (planStateSpaceFifoPop(ss) != NULL);
}

