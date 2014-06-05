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
                        void *node_init,
                        plan_state_space_pop_fn fn_pop,
                        plan_state_space_insert_fn fn_insert,
                        plan_state_space_clear_fn fn_clear,
                        plan_state_space_close_all_fn fn_close_all)
{

    state_space->state_pool = state_pool;
    state_space->data_id = planStatePoolDataReserve(state_pool, node_size,
                                                    node_init);

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

#if 0
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
#endif
