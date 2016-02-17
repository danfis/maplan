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

#include <strings.h>
#include <stdio.h>
#include <boruvka/alloc.h>
#include "plan/prio_queue.h"

struct _heap_node_t {
    int key;
    int value;
    bor_pairheap_node_t node;
};
typedef struct _heap_node_t heap_node_t;

static void planBucketQueueInit(plan_bucket_queue_t *q);
static void planBucketQueueFree(plan_bucket_queue_t *q);
static void planBucketQueuePush(plan_bucket_queue_t *q, int key, int value);
static int planBucketQueuePop(plan_bucket_queue_t *q, int *key);
/** Convets bucket queue to heap queue */
static void planBucketQueueToHeapQueue(plan_bucket_queue_t *b,
                                       plan_heap_queue_t *h);

static void planHeapQueueInit(plan_heap_queue_t *q);
static void planHeapQueueFree(plan_heap_queue_t *q);
static void planHeapQueuePush(plan_heap_queue_t *q, int key, int value);
static int planHeapQueuePop(plan_heap_queue_t *q, int *key);

void planPrioQueueInit(plan_prio_queue_t *q)
{
    planBucketQueueInit(&q->bucket_queue);
    q->bucket = 1;
}

void planPrioQueueFree(plan_prio_queue_t *q)
{
    if (q->bucket){
        planBucketQueueFree(&q->bucket_queue);
    }else{
        planHeapQueueFree(&q->heap_queue);
    }
}

void planPrioQueuePush(plan_prio_queue_t *q, int key, int value)
{
    if (q->bucket){
        if (key >= PLAN_BUCKET_QUEUE_SIZE){
            planHeapQueueInit(&q->heap_queue);
            planBucketQueueToHeapQueue(&q->bucket_queue, &q->heap_queue);
            planBucketQueueFree(&q->bucket_queue);
            q->bucket = 0;
            planHeapQueuePush(&q->heap_queue, key, value);
        }else{
            planBucketQueuePush(&q->bucket_queue, key, value);
        }
    }else{
        planHeapQueuePush(&q->heap_queue, key, value);
    }
}

int planPrioQueuePop(plan_prio_queue_t *q, int *key)
{
    if (q->bucket){
        return planBucketQueuePop(&q->bucket_queue, key);
    }else{
        return planHeapQueuePop(&q->heap_queue, key);
    }
}




static void planBucketQueueInit(plan_bucket_queue_t *q)
{
    q->bucket_size = PLAN_BUCKET_QUEUE_SIZE;
    q->bucket = BOR_ALLOC_ARR(plan_prioqueue_bucket_t, q->bucket_size);
    bzero(q->bucket, sizeof(plan_prioqueue_bucket_t) * q->bucket_size);
    q->lowest_key = q->bucket_size;
    q->size = 0;
}

static void planBucketQueueFree(plan_bucket_queue_t *q)
{
    int i;

    for (i = 0; i < q->bucket_size; ++i){
        if (q->bucket[i].value)
            BOR_FREE(q->bucket[i].value);
    }
    BOR_FREE(q->bucket);
}

static void planBucketQueuePush(plan_bucket_queue_t *q, int key, int value)
{
    plan_prioqueue_bucket_t *bucket;

    if (key >= PLAN_BUCKET_QUEUE_SIZE){
        fprintf(stderr, "Error: planBucketQueue: key %d is over a size of"
                        " the bucket queue, which is %d.",
                        key, PLAN_BUCKET_QUEUE_SIZE);
        exit(-1);
    }

    bucket = q->bucket + key;
    if (bucket->value == NULL){
        bucket->alloc = PLAN_BUCKET_QUEUE_BUCKET_INIT_SIZE;
        bucket->value = BOR_ALLOC_ARR(int, bucket->alloc);

    }else if (bucket->size + 1 > bucket->alloc){
        bucket->alloc *= PLAN_BUCKET_QUEUE_BUCKET_EXPANSION_FACTOR;
        bucket->value = BOR_REALLOC_ARR(bucket->value,
                                        int, bucket->alloc);
    }
    bucket->value[bucket->size++] = value;
    ++q->size;

    if (key < q->lowest_key)
        q->lowest_key = key;
}

static int planBucketQueuePop(plan_bucket_queue_t *q, int *key)
{
    plan_prioqueue_bucket_t *bucket;
    int val;

    bucket = q->bucket + q->lowest_key;
    while (bucket->size == 0){
        ++q->lowest_key;
        bucket += 1;
    }

    val = bucket->value[--bucket->size];
    *key = q->lowest_key;
    --q->size;
    return val;
}

static void planBucketQueueToHeapQueue(plan_bucket_queue_t *b,
                                       plan_heap_queue_t *h)
{
    plan_prioqueue_bucket_t *bucket;
    int i, j;

    for (i = b->lowest_key; i < PLAN_BUCKET_QUEUE_SIZE; ++i){
        bucket = b->bucket + i;
        for (j = 0; j < bucket->size; ++j){
            planHeapQueuePush(h, i, bucket->value[j]);
        }
        if (bucket->value != NULL)
            BOR_FREE(bucket->value);
        bucket->value = NULL;
        bucket->size = bucket->alloc = 0;
    }
    b->size = 0;
}


static int heapLT(const bor_pairheap_node_t *_n1,
                  const bor_pairheap_node_t *_n2, void *_)
{
    heap_node_t *n1 = bor_container_of(_n1, heap_node_t, node);
    heap_node_t *n2 = bor_container_of(_n2, heap_node_t, node);
    return n1->key <= n2->key;
}

static void heapClear(bor_pairheap_node_t *_n, void *_)
{
    heap_node_t *n = bor_container_of(_n, heap_node_t, node);
    BOR_FREE(n);
}

static void planHeapQueueInit(plan_heap_queue_t *q)
{
    q->heap = borPairHeapNew(heapLT, NULL);
}

static void planHeapQueueFree(plan_heap_queue_t *q)
{
    borPairHeapClear(q->heap, heapClear, NULL);
    borPairHeapDel(q->heap);
}

static void planHeapQueuePush(plan_heap_queue_t *q, int key, int value)
{
    heap_node_t *n;

    n = BOR_ALLOC(heap_node_t);
    n->value = value;
    n->key   = key;
    borPairHeapAdd(q->heap, &n->node);
}

static int planHeapQueuePop(plan_heap_queue_t *q, int *key)
{
    bor_pairheap_node_t *hn;
    heap_node_t *n;
    int value;

    hn = borPairHeapExtractMin(q->heap);
    n = bor_container_of(hn, heap_node_t, node);
    *key = n->key;
    value = n->value;
    BOR_FREE(n);

    return value;
}
