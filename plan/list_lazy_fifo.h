#ifndef __PLAN_LAZY_FIFO_H__
#define __PLAN_LAZY_FIFO_H__

#include <boruvka/list.h>

#include <plan/state.h>
#include <plan/operator.h>

typedef bor_list_t plan_list_lazy_fifo_t;

/**
 * Creates a new lazy fifo list.
 */
plan_list_lazy_fifo_t *planListLazyFifoNew(void);

/**
 * Destroys the list.
 */
void planListLazyFifoDel(plan_list_lazy_fifo_t *l);

/**
 * Pushes an element that will contain given values into fifo.
 */
void planListLazyFifoPush(plan_list_lazy_fifo_t *l,
                          plan_state_id_t parent_state_id,
                          plan_operator_t *op);

/**
 * Pops next element from the fifo and fills the output arguments with its
 * values.
 * Returns 0 on success, -1 if the fifo is empty.
 */
int planListLazyFifoPop(plan_list_lazy_fifo_t *l,
                        plan_state_id_t *parent_state_id,
                        plan_operator_t **op);

/**
 * Empty the whole list.
 */
void planListLazyFifoClear(plan_list_lazy_fifo_t *l);

#endif /* __PLAN_LAZY_FIFO_H__ */
