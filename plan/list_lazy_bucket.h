#ifndef __PLAN_LAZY_BUCKET_H__
#define __PLAN_LAZY_BUCKET_H__

#include <plan/dataarr.h>
#include <plan/state.h>
#include <plan/operator.h>

struct _plan_list_lazy_bucket_t {
    plan_data_arr_t *bucket;
    long size;
    plan_cost_t lowest_key;
};
typedef struct _plan_list_lazy_bucket_t plan_list_lazy_bucket_t;

/**
 * Creates a new bucket-based lazy list.
 */
plan_list_lazy_bucket_t *planListLazyBucketNew(void);

/**
 * Destroys the list.
 */
void planListLazyBucketDel(plan_list_lazy_bucket_t *l);

/**
 * Inserts an element with specified cost into the list.
 */
void planListLazyBucketPush(plan_list_lazy_bucket_t *l,
                            plan_cost_t cost,
                            plan_state_id_t parent_state_id,
                            plan_operator_t *op);

/**
 * Pops the next element from the bucket that has the lowest cost.
 * Returns 0 on success, -1 if the bucket is empty.
 */
int planListLazyBucketPop(plan_list_lazy_bucket_t *l,
                          plan_state_id_t *parent_state_id,
                          plan_operator_t **op);

/**
 * Empty the whole list.
 */
void planListLazyBucketClear(plan_list_lazy_bucket_t *l);

#endif /* __PLAN_LAZY_BUCKET_H__ */

