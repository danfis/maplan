#ifndef __PLAN_STATESPACE_BUCKET_H__
#define __PLAN_STATESPACE_BUCKET_H__

#include <plan/statespace.h>

/**
 * State Space with Bucket Based Open List
 * ========================================
 */

/**
 * Creates a new bucket based state space.
 */
plan_state_space_t *planStateSpaceBucketCostNew(plan_state_pool_t *state_pool);
plan_state_space_t *planStateSpaceBucketHeuristicNew(
            plan_state_pool_t *state_pool);
plan_state_space_t *planStateSpaceBucketCostHeuristicNew(
            plan_state_pool_t *state_pool);

/**
 * Frees all allocated resources of the state space.
 */
void planStateSpaceBucketDel(plan_state_space_t *ss);

#endif /* __PLAN_STATESPACE_H__ */

