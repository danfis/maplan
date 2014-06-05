#ifndef __PLAN_STATESPACE_HEAP_H__
#define __PLAN_STATESPACE_HEAP_H__

#include <plan/statespace.h>

/**
 * State Space with Heap Based Open List
 * ======================================
 */

/**
 * Creates a new state space that sorts open states by their cost.
 */
plan_state_space_t *planStateSpaceHeapCostNew(plan_state_pool_t *);

/**
 * Creates a new state space that sorts open states by their heuristic.
 */
plan_state_space_t *planStateSpaceHeapHeuristicNew(plan_state_pool_t *);

/**
 * Creates a new state space that sorts open states by their sum of cost
 * and heuristic.
 */
plan_state_space_t *planStateSpaceHeapCostHeuristicNew(plan_state_pool_t *);

/**
 * Frees all allocated resources of the state space.
 */
void planStateSpaceHeapDel(plan_state_space_t *ss);

#endif /* __PLAN_STATESPACE_H__ */

