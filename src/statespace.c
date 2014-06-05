#include <boruvka/alloc.h>

#include "plan/statespace.h"

/** Comparator for pairing heap */
static int heapLessThan(const bor_pairheap_node_t *n1,
                        const bor_pairheap_node_t *n2,
                        void *data);

plan_state_space_t *planStateSpaceNew(plan_state_pool_t *state_pool)
{
    plan_state_space_t *ss;
    plan_state_space_node_t elinit;

    ss = BOR_ALLOC(plan_state_space_t);
    ss->heap = borPairHeapNew(heapLessThan, ss);

    ss->state_pool = state_pool;

    elinit.state_id = PLAN_NO_STATE;
    elinit.parent_state_id = PLAN_NO_STATE;
    elinit.state = PLAN_NODE_STATE_NEW;
    elinit.op    = NULL;
    elinit.heuristic = -1;
    ss->data_id = planStatePoolDataReserve(ss->state_pool,
                                           sizeof(plan_state_space_node_t),
                                           &elinit);

    return ss;
}

void planStateSpaceDel(plan_state_space_t *ss)
{
    if (ss->heap)
        borPairHeapDel(ss->heap);
    BOR_FREE(ss);
}

plan_state_space_node_t *planStateSpaceExtractMin(plan_state_space_t *ss)
{
    plan_state_space_node_t *node;
    bor_pairheap_node_t *heap_node;

    // check whether the open list (heap) contains any nodes at all
    if (borPairHeapEmpty(ss->heap))
        return NULL;

    // get the minimal node and removes it from the heap
    heap_node = borPairHeapExtractMin(ss->heap);

    // resolve node pointer and set it as closed
    node = bor_container_of(heap_node, plan_state_space_node_t, heap);
    node->state = PLAN_NODE_STATE_CLOSED;

    return node;
}

plan_state_space_node_t *planStateSpaceOpenNode(plan_state_space_t *ss,
                                                plan_state_id_t sid,
                                                plan_state_id_t parent_sid,
                                                plan_operator_t *op,
                                                unsigned heuristic)
{
    plan_state_space_node_t *n;

    n = planStateSpaceNode(ss, sid);
    if (!planStateSpaceNodeIsNew(n)){
        return NULL;
    }

    // set up value of heuristic
    n->state_id        = sid;
    n->parent_state_id = parent_sid;
    n->state           = PLAN_NODE_STATE_OPEN;
    n->op              = op;
    n->heuristic       = heuristic;

    // insert node into open list
    borPairHeapAdd(ss->heap, &n->heap);

    return n;
}

plan_state_space_node_t *planStateSpaceReopenNode(plan_state_space_t *ss,
                                                  plan_state_id_t sid,
                                                  plan_state_id_t parent_sid,
                                                  plan_operator_t *op,
                                                  unsigned heuristic)
{
    plan_state_space_node_t *n;

    n = planStateSpaceNode(ss, sid);
    if (!planStateSpaceNodeIsClosed(n))
        return NULL;

    // set up heuristic value
    n->state_id        = sid;
    n->parent_state_id = parent_sid;
    n->state           = PLAN_NODE_STATE_OPEN;
    n->op              = op;
    n->heuristic       = heuristic;

    // and insert it into heap
    borPairHeapAdd(ss->heap, &n->heap);

    return n;
}

plan_state_space_node_t *planStateSpaceCloseNode(plan_state_space_t *ss,
                                                 plan_state_id_t sid)
{
    plan_state_space_node_t *n;

    n = planStateSpaceNode(ss, sid);
    if (!planStateSpaceNodeIsOpen(n))
        return NULL;

    borPairHeapRemove(ss->heap, &n->heap);
    n->state = PLAN_NODE_STATE_CLOSED;

    return n;
}

plan_state_space_node_t *planStateSpaceNode(plan_state_space_t *ss,
                                            plan_state_id_t sid)
{
    plan_state_space_node_t *n;
    n = planStatePoolData(ss->state_pool, ss->data_id, sid);
    n->state_id = sid;
    return n;
}

void planStateSpaceClearOpenNodes(plan_state_space_t *ss)
{
    // TODO: This can be done much more efficiently but requires change in
    // boruvka/pairheap
    plan_state_space_node_t *n;

    while ((n = planStateSpaceExtractMin(ss)) != NULL){
        n->state = PLAN_NODE_STATE_NEW;
    }
}

static int heapLessThan(const bor_pairheap_node_t *n1,
                        const bor_pairheap_node_t *n2,
                        void *data)
{
    plan_state_space_node_t *el1
            = bor_container_of(n1, plan_state_space_node_t, heap);
    plan_state_space_node_t *el2
            = bor_container_of(n2, plan_state_space_node_t, heap);
    return el1->heuristic < el2->heuristic;
}
