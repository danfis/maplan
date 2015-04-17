/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#include <boruvka/alloc.h>
#include <boruvka/fifo.h>
#include "plan/list_lazy.h"

/** Initial number of buckets */
#define BUCKET_INIT_SIZE 1024

/** Maximal key the bucket-heap take -- it must be reasonable high number
 *  to prevent consumption of a whole memory. */
#define BUCKET_MAX_KEY (1024 * 1024)

struct _node_t {
    plan_state_id_t parent_state_id;
    plan_op_t *op;
};
typedef struct _node_t node_t;

struct _plan_list_lazy_bucket_t {
    plan_list_lazy_t list_lazy;

    bor_fifo_t *bucket;
    int bucket_size;
    plan_cost_t lowest_key;
    int size;
};
typedef struct _plan_list_lazy_bucket_t plan_list_lazy_bucket_t;

#define LIST_FROM_PARENT(list) \
    bor_container_of((list), plan_list_lazy_bucket_t, list_lazy)

static void planListLazyBucketDel(plan_list_lazy_t *l);
static void planListLazyBucketPush(plan_list_lazy_t *l,
                                   plan_cost_t cost,
                                   plan_state_id_t parent_state_id,
                                   plan_op_t *op);
static int planListLazyBucketPop(plan_list_lazy_t *l,
                                 plan_state_id_t *parent_state_id,
                                 plan_op_t **op);
static void planListLazyBucketClear(plan_list_lazy_t *l);


plan_list_lazy_t *planListLazyBucketNew(void)
{
    plan_list_lazy_bucket_t *b;
    int i;

    b = BOR_ALLOC(plan_list_lazy_bucket_t);
    b->bucket_size = BUCKET_INIT_SIZE;
    b->bucket = BOR_ALLOC_ARR(bor_fifo_t, b->bucket_size);
    b->lowest_key  = INT_MAX;
    b->size        = 0;

    for (i = 0; i < b->bucket_size; ++i){
        borFifoInit(b->bucket + i, sizeof(node_t));
    }

    planListLazyInit(&b->list_lazy,
                     planListLazyBucketDel,
                     planListLazyBucketPush,
                     planListLazyBucketPop,
                     planListLazyBucketClear);

    return &b->list_lazy;
}

static void planListLazyBucketDel(plan_list_lazy_t *_l)
{
    plan_list_lazy_bucket_t *l = LIST_FROM_PARENT(_l);
    int i;

    planListLazyFree(&l->list_lazy);

    for (i = 0; i < l->bucket_size; ++i)
        borFifoFree(l->bucket + i);
    BOR_FREE(l->bucket);
    BOR_FREE(l);
}

static void planListLazyBucketPush(plan_list_lazy_t *_l,
                                   plan_cost_t cost,
                                   plan_state_id_t parent_state_id,
                                   plan_op_t *op)
{
    plan_list_lazy_bucket_t *l = LIST_FROM_PARENT(_l);
    bor_fifo_t *bucket;
    node_t n;
    int i;

    // Check the value of key
    if (cost > BUCKET_MAX_KEY){
        fprintf(stderr, "Error: planListLazyBucket: Key %d to is too high."
                        " Are you sure this type of list is suitable for"
                        " you? Exiting...\n",
                        (int)cost);
        exit(-1);
    }

    // Expand buckets if neccessary
    if (cost >= l->bucket_size){
        i = l->bucket_size;
        l->bucket_size = BOR_MAX(cost + 1, l->bucket_size * 2);
        l->bucket = BOR_REALLOC_ARR(l->bucket, bor_fifo_t, l->bucket_size);
        for (; i < l->bucket_size; ++i){
            borFifoInit(l->bucket + i, sizeof(node_t));
        }
    }

    // get the right bucket insert values there
    bucket = l->bucket + cost;
    n.parent_state_id = parent_state_id;
    n.op = op;
    borFifoPush(bucket, &n);

    // update lowest key if needed
    if (cost < l->lowest_key)
        l->lowest_key = cost;

    ++l->size;
}

static int planListLazyBucketPop(plan_list_lazy_t *_l,
                                 plan_state_id_t *parent_state_id,
                                 plan_op_t **op)
{
    plan_list_lazy_bucket_t *l = LIST_FROM_PARENT(_l);
    bor_fifo_t *bucket;
    node_t *node;

    // check if the heap isn't empty
    if (l->size == 0)
        return -1;

    // find out the first non-empty bucket
    bucket = l->bucket + l->lowest_key;
    while (borFifoEmpty(bucket)){
        ++l->lowest_key;
        ++bucket;
    }

    // read values from the node
    node = borFifoFront(bucket);
    *parent_state_id = node->parent_state_id;
    *op              = node->op;

    // remove the node from bucket
    borFifoPop(bucket);
    --l->size;
    return 0;
}

static void planListLazyBucketClear(plan_list_lazy_t *_l)
{
    plan_list_lazy_bucket_t *l = LIST_FROM_PARENT(_l);
    int i;

    for (i = l->lowest_key; i < l->bucket_size; ++i){
        borFifoClear(l->bucket + i);
    }
}
