#include <boruvka/alloc.h>
#include "plan/statespace_fifo.h"

/** State space callbacks: */
static plan_state_space_node_t *pop(void *state_space);
static void insert(void *state_space, plan_state_space_node_t *node);
static void clear(void *state_space);
static void closeAll(void *state_space);


plan_state_space_fifo_t *planStateSpaceFifoNew(plan_state_pool_t *state_pool)
{
    plan_state_space_fifo_t *ss;
    plan_state_space_fifo_node_t node_init;

    ss = BOR_ALLOC(plan_state_space_fifo_t);

    planStateSpaceNodeInit(&node_init.node);
    borListInit(&node_init.fifo);

    planStateSpaceInit(&ss->state_space, state_pool,
                       sizeof(plan_state_space_fifo_node_t), &node_init,
                       pop, insert, clear, closeAll);

    borListInit(&ss->fifo);

    return ss;
}

void planStateSpaceFifoDel(plan_state_space_fifo_t *ss)
{
    BOR_FREE(ss);
}


static plan_state_space_node_t *pop(void *_ss)
{
    plan_state_space_fifo_node_t *node;
    plan_state_space_fifo_t *ss = (plan_state_space_fifo_t *)_ss;
    bor_list_t *item;

    if (borListEmpty(&ss->fifo))
        return NULL;

    item = borListNext(&ss->fifo);
    borListDel(item);
    node = BOR_LIST_ENTRY(item, plan_state_space_fifo_node_t, fifo);
    return &node->node;
}

static void insert(void *_ss, plan_state_space_node_t *node)
{
    plan_state_space_fifo_t *ss = (plan_state_space_fifo_t *)_ss;
    plan_state_space_fifo_node_t *n;
    n = bor_container_of(node, plan_state_space_fifo_node_t, node);
    borListAppend(&ss->fifo, &n->fifo);
}

static void clear(void *_ss)
{
    plan_state_space_fifo_t *ss = (plan_state_space_fifo_t *)_ss;
    plan_state_space_fifo_node_t *node;

    while ((node = planStateSpaceFifoPop(ss)) != NULL){
        node->node.state = PLAN_STATE_SPACE_NODE_NEW;
    }
}

static void closeAll(void *_ss)
{
    plan_state_space_fifo_t *ss = (plan_state_space_fifo_t *)_ss;
    while (planStateSpaceFifoPop(ss) != NULL);
}

#endif
