#include <boruvka/alloc.h>
#include <boruvka/pairheap.h>
#include "plan/statespace_heap.h"

struct _plan_state_space_heap_node_t {
    plan_state_space_node_t node;
    bor_pairheap_node_t heap; /*!< Connector to an open list */
};
typedef struct _plan_state_space_heap_node_t plan_state_space_heap_node_t;

struct _plan_state_space_heap_t {
    plan_state_space_t state_space;
    bor_pairheap_t *heap;
};
typedef struct _plan_state_space_heap_t plan_state_space_heap_t;

static plan_state_space_t *new(plan_state_pool_t *sp, bor_pairheap_lt heap_lt);

static int heapCostLessThan(const bor_pairheap_node_t *n1,
                            const bor_pairheap_node_t *n2,
                            void *data);
static int heapHeuristicLessThan(const bor_pairheap_node_t *n1,
                                 const bor_pairheap_node_t *n2,
                                 void *data);
static int heapCostHeuristicLessThan(const bor_pairheap_node_t *n1,
                                     const bor_pairheap_node_t *n2,
                                     void *data);

static plan_state_space_node_t *pop(void *state_space);
static void insert(void *state_space, plan_state_space_node_t *node);

plan_state_space_t *planStateSpaceHeapCostNew(plan_state_pool_t *sp)
{
    return new(sp, heapCostLessThan);
}

plan_state_space_t *planStateSpaceHeapHeuristicNew(plan_state_pool_t *sp)
{
    return new(sp, heapHeuristicLessThan);
}

plan_state_space_t *planStateSpaceHeapCostHeuristicNew(plan_state_pool_t *sp)
{
    return new(sp, heapCostHeuristicLessThan);
}

void planStateSpaceHeapDel(plan_state_space_t *_ss)
{
    plan_state_space_heap_t *ss = (plan_state_space_heap_t *)_ss;
    if (ss->heap)
        borPairHeapDel(ss->heap);
    planStateSpaceFree(&ss->state_space);
    BOR_FREE(ss);
}

static plan_state_space_t *new(plan_state_pool_t *sp, bor_pairheap_lt heap_lt)
{
    plan_state_space_heap_t *ss;
    plan_state_space_heap_node_t node_init;

    ss = BOR_ALLOC(plan_state_space_heap_t);

    planStateSpaceNodeInit(&node_init.node);
    planStateSpaceInit(&ss->state_space, sp,
                       sizeof(plan_state_space_heap_node_t),
                       NULL, &node_init,
                       pop, insert, NULL, NULL);

    ss->heap = borPairHeapNew(heap_lt, ss);
    return &ss->state_space;
}

static int heapCostLessThan(const bor_pairheap_node_t *n1,
                            const bor_pairheap_node_t *n2,
                            void *data)
{
    plan_state_space_heap_node_t *el1
            = bor_container_of(n1, plan_state_space_heap_node_t, heap);
    plan_state_space_heap_node_t *el2
            = bor_container_of(n2, plan_state_space_heap_node_t, heap);
    return el1->node.cost < el2->node.cost;
}

static int heapHeuristicLessThan(const bor_pairheap_node_t *n1,
                                 const bor_pairheap_node_t *n2,
                                 void *data)
{
    plan_state_space_heap_node_t *el1
            = bor_container_of(n1, plan_state_space_heap_node_t, heap);
    plan_state_space_heap_node_t *el2
            = bor_container_of(n2, plan_state_space_heap_node_t, heap);
    return el1->node.heuristic < el2->node.heuristic;
}

static int heapCostHeuristicLessThan(const bor_pairheap_node_t *n1,
                                     const bor_pairheap_node_t *n2,
                                     void *data)
{
    plan_state_space_heap_node_t *el1
            = bor_container_of(n1, plan_state_space_heap_node_t, heap);
    plan_state_space_heap_node_t *el2
            = bor_container_of(n2, plan_state_space_heap_node_t, heap);
    return (el1->node.cost + el1->node.heuristic)
                < (el2->node.cost + el2->node.heuristic);
}

static plan_state_space_node_t *pop(void *_ss)
{
    plan_state_space_heap_t *ss = (plan_state_space_heap_t *)_ss;
    plan_state_space_heap_node_t *node;
    bor_pairheap_node_t *heap_node;

    // check whether the open list (heap) contains any nodes at all
    if (borPairHeapEmpty(ss->heap))
        return NULL;

    // get the minimal node and removes it from the heap
    heap_node = borPairHeapExtractMin(ss->heap);

    // resolve node pointer and set it as closed
    node = bor_container_of(heap_node, plan_state_space_heap_node_t, heap);

    return &node->node;
}

static void insert(void *_ss, plan_state_space_node_t *_n)
{
    plan_state_space_heap_t *ss = (plan_state_space_heap_t *)_ss;
    plan_state_space_heap_node_t *n = (plan_state_space_heap_node_t *)_n;
    borPairHeapAdd(ss->heap, &n->heap);
}
