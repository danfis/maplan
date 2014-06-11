#ifndef __PLAN_LAZY_HEAP_H__
#define __PLAN_LAZY_HEAP_H__

#include <boruvka/pairheap.h>

#include <plan/state.h>
#include <plan/operator.h>

/*
struct _plan_list_lazy_heap_t {
    bor_pairheap_t *heap;
};
typedef struct _plan_list_lazy_heap_t plan_list_lazy_heap_t;
*/
typedef bor_pairheap_t plan_list_lazy_heap_t;

/**
 * Creates a new lazy heap.
 */
plan_list_lazy_heap_t *planListLazyHeapNew(void);

/**
 * Destroys the heap.
 */
void planListLazyHeapDel(plan_list_lazy_heap_t *l);

/**
 * Inserts an element with specified cost into the heap.
 */
void planListLazyHeapPush(plan_list_lazy_heap_t *l,
                          plan_cost_t cost,
                          plan_state_id_t parent_state_id,
                          plan_operator_t *op);

/**
 * Pops the next element from the heap that has the lowest cost.
 * Returns 0 on success, -1 if the heap is empty.
 */
int planListLazyHeapPop(plan_list_lazy_heap_t *l,
                        plan_state_id_t *parent_state_id,
                        plan_operator_t **op);

/**
 * Empty the whole list.
 */
void planListLazyHeapClear(plan_list_lazy_heap_t *l);

#endif /* __PLAN_LAZY_HEAP_H__ */
