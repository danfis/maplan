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

#ifndef __PLAN_PRIOQUEUE_H__
#define __PLAN_PRIOQUEUE_H__

#include <boruvka/pairheap.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Adaptive Priority Queue
 * ========================
 */

/**
 * Number of buckets available in bucket-based queue.
 * Inserting key greater or equal then this constant will end up in program
 * termination.
 */
#define PLAN_BUCKET_QUEUE_SIZE 1024

/**
 * Initial size of one bucket in bucket-queue.
 */
#define PLAN_BUCKET_QUEUE_BUCKET_INIT_SIZE 32

/**
 * Expansion factor of a bucket.
 */
#define PLAN_BUCKET_QUEUE_BUCKET_EXPANSION_FACTOR 2

/**
 * Bucket for storing int values.
 */
struct _plan_prioqueue_bucket_t {
    int *value; /*!< Stored values */
    int size;   /*!< Number of stored values */
    int alloc;  /*!< Size of the allocated array */
};
typedef struct _plan_prioqueue_bucket_t plan_prioqueue_bucket_t;

/**
 * Bucket based priority queue.
 */
struct _plan_bucket_queue_t {
    plan_prioqueue_bucket_t *bucket; /*!< Array of buckets */
    int bucket_size;                 /*!< Number of buckets */
    int lowest_key;                  /*!< Lowest key so far */
    int size;                        /*!< Number of elements stored in queue */
};
typedef struct _plan_bucket_queue_t plan_bucket_queue_t;

/**
 * Heap-based priority queue.
 */
struct _plan_heap_queue_t {
    bor_pairheap_t *heap;
};
typedef struct _plan_heap_queue_t plan_heap_queue_t;


struct _plan_prio_queue_t {
    plan_bucket_queue_t bucket_queue;
    plan_heap_queue_t heap_queue;
    int bucket;
};
typedef struct _plan_prio_queue_t plan_prio_queue_t;

/**
 * Initializes priority queue.
 */
void planPrioQueueInit(plan_prio_queue_t *q);

/**
 * Frees allocated resources.
 */
void planPrioQueueFree(plan_prio_queue_t *q);

/**
 * Inserts an element into queue.
 */
void planPrioQueuePush(plan_prio_queue_t *q, int key, int value);

/**
 * Removes and returns the lowest element.
 */
int planPrioQueuePop(plan_prio_queue_t *q, int *key);

/**
 * Returns true if the queue is empty.
 */
_bor_inline int planPrioQueueEmpty(const plan_prio_queue_t *q);



/**** INLINES ****/
_bor_inline int planBucketQueueEmpty(const plan_bucket_queue_t *q)
{
    return q->size == 0;
}

_bor_inline int planHeapQueueEmpty(const plan_heap_queue_t *q)
{
    return borPairHeapEmpty(q->heap);
}

_bor_inline int planPrioQueueEmpty(const plan_prio_queue_t *q)
{
    if (q->bucket){
        return planBucketQueueEmpty(&q->bucket_queue);
    }else{
        return planHeapQueueEmpty(&q->heap_queue);
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PRIOQUEUE_H__ */
