#ifndef __PLAN_STATESPACE_FIFO_H__
#define __PLAN_STATESPACE_FIFO_H__

#include <boruvka/list.h>
#include <plan/statespace.h>

/**
 * FIFO State Space
 * =================
 */

/**
 * Creates a new FIFO state space that uses provided state_pool.
 */
plan_state_space_t *planStateSpaceFifoNew(plan_state_pool_t *state_pool);

/**
 * Frees all allocated resources of the state space.
 */
void planStateSpaceFifoDel(plan_state_space_t *ss);

#endif /* __PLAN_STATESPACE_H__ */
