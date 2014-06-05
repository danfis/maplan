#include <boruvka/alloc.h>
#include <boruvka/list.h>
#include "plan/statespace_fifo.h"

struct _plan_state_space_fifo_node_t {
    plan_state_space_node_t node;
    bor_list_t fifo; /*!< Connector to the fifo queue */
};
typedef struct _plan_state_space_fifo_node_t plan_state_space_fifo_node_t;

struct _plan_state_space_fifo_t {
    plan_state_space_t state_space;
    bor_list_t fifo;
};
typedef struct _plan_state_space_fifo_t plan_state_space_fifo_t;


/** State space callbacks: */
static plan_state_space_node_t *pop(void *state_space);
static void insert(void *state_space, plan_state_space_node_t *node);


plan_state_space_t *planStateSpaceFifoNew(plan_state_pool_t *state_pool)
{
    plan_state_space_fifo_t *ss;
    plan_state_space_fifo_node_t node_init;

    ss = BOR_ALLOC(plan_state_space_fifo_t);

    planStateSpaceNodeInit(&node_init.node);
    borListInit(&node_init.fifo);

    planStateSpaceInit(&ss->state_space, state_pool,
                       sizeof(plan_state_space_fifo_node_t), &node_init,
                       pop, insert, NULL, NULL);

    borListInit(&ss->fifo);

    return &ss->state_space;
}

void planStateSpaceFifoDel(plan_state_space_t *_ss)
{
    plan_state_space_fifo_t *ss = (plan_state_space_fifo_t *)_ss;
    planStateSpaceFree(&ss->state_space);
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
