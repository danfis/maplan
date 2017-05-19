/***
 * maplan
 * -------
 * Copyright (c)2017 Daniel Fiser <danfis@danfis.cz>,
 * AIC, Department of Computer Science,
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

#include <stdio.h>
#include <boruvka/alloc.h>
#include <plan/pq.h>

static void planPQBucketQueueInit(plan_pq_bucket_queue_t *q);
static void planPQBucketQueueFree(plan_pq_bucket_queue_t *q);
static void planPQBucketQueuePush(plan_pq_bucket_queue_t *q,
                                  int key, plan_pq_el_t *el);
static plan_pq_el_t *planPQBucketQueuePop(plan_pq_bucket_queue_t *q, int *key);
static void planPQBucketQueueUpdate(plan_pq_bucket_queue_t *q,
                                    int key, plan_pq_el_t *el);
/** Convets bucket queue to heap queue */
static void planPQBucketQueueToHeapQueue(plan_pq_bucket_queue_t *b,
                                         plan_pq_heap_queue_t *h);

static void planPQHeapQueueInit(plan_pq_heap_queue_t *q);
static void planPQHeapQueueFree(plan_pq_heap_queue_t *q);
static void planPQHeapQueuePush(plan_pq_heap_queue_t *q,
                                int key, plan_pq_el_t *el);
static plan_pq_el_t *planPQHeapQueuePop(plan_pq_heap_queue_t *q, int *key);
static void planPQHeapQueueUpdate(plan_pq_heap_queue_t *q,
                                  int key, plan_pq_el_t *el);

void planPQInit(plan_pq_t *q)
{
    planPQBucketQueueInit(&q->bucket_queue);
    q->bucket = 1;
}

void planPQFree(plan_pq_t *q)
{
    if (q->bucket){
        planPQBucketQueueFree(&q->bucket_queue);
    }else{
        planPQHeapQueueFree(&q->heap_queue);
    }
}

void planPQPush(plan_pq_t *q, int key, plan_pq_el_t *el)
{
    if (q->bucket){
        if (key >= PLAN_PQ_BUCKET_SIZE){
            planPQHeapQueueInit(&q->heap_queue);
            planPQBucketQueueToHeapQueue(&q->bucket_queue, &q->heap_queue);
            planPQBucketQueueFree(&q->bucket_queue);
            q->bucket = 0;
            planPQHeapQueuePush(&q->heap_queue, key, el);
        }else{
            planPQBucketQueuePush(&q->bucket_queue, key, el);
        }
    }else{
        planPQHeapQueuePush(&q->heap_queue, key, el);
    }
}

plan_pq_el_t *planPQPop(plan_pq_t *q, int *key)
{
    if (q->bucket){
        return planPQBucketQueuePop(&q->bucket_queue, key);
    }else{
        return planPQHeapQueuePop(&q->heap_queue, key);
    }
}

void planPQUpdate(plan_pq_t *q, int key, plan_pq_el_t *el)
{
    if (q->bucket){
        planPQBucketQueueUpdate(&q->bucket_queue, key, el);
    }else{
        planPQHeapQueueUpdate(&q->heap_queue, key, el);
    }
}

static void planPQBucketQueueInit(plan_pq_bucket_queue_t *q)
{
    q->bucket_size = PLAN_PQ_BUCKET_SIZE;
    q->bucket = BOR_CALLOC_ARR(plan_pq_bucket_t, q->bucket_size);
    q->lowest_key = q->bucket_size;
    q->size = 0;
}

static void planPQBucketQueueFree(plan_pq_bucket_queue_t *q)
{
    int i;

    for (i = 0; i < q->bucket_size; ++i){
        if (q->bucket[i].el)
            BOR_FREE(q->bucket[i].el);
    }
    BOR_FREE(q->bucket);
}

static void planPQBucketQueuePush(plan_pq_bucket_queue_t *q,
                                  int key, plan_pq_el_t *el)
{
    plan_pq_bucket_t *bucket;

    if (key >= PLAN_PQ_BUCKET_SIZE){
        fprintf(stderr, "Error: planPQBucketQueue: key %d is over a size of"
                        " the bucket queue, which is %d.",
                        key, PLAN_PQ_BUCKET_SIZE);
        exit(-1);
    }

    bucket = q->bucket + key;
    if (bucket->size == bucket->alloc){
        if (bucket->alloc == 0){
            bucket->alloc = PLAN_PQ_BUCKET_INIT_SIZE;
        }else{
            bucket->alloc *= PLAN_PQ_BUCKET_EXPANSION_FACTOR;
        }
        bucket->el = BOR_REALLOC_ARR(bucket->el, plan_pq_el_t *,
                                     bucket->alloc);

    }
    el->key = key;
    el->conn.bucket = bucket->size;
    bucket->el[bucket->size++] = el;
    ++q->size;

    if (key < q->lowest_key)
        q->lowest_key = key;
}

static plan_pq_el_t *planPQBucketQueuePop(plan_pq_bucket_queue_t *q, int *key)
{
    plan_pq_bucket_t *bucket;
    plan_pq_el_t *el;

    if (q->size == 0)
        return NULL;

    bucket = q->bucket + q->lowest_key;
    while (bucket->size == 0){
        ++q->lowest_key;
        bucket += 1;
    }

    el = bucket->el[--bucket->size];
    if (key)
        *key = q->lowest_key;
    --q->size;
    return el;
}

static void planPQBucketQueueUpdate(plan_pq_bucket_queue_t *q,
                                    int key, plan_pq_el_t *el)
{
    plan_pq_bucket_t *bucket;

    bucket = q->bucket + el->key;
    bucket->el[el->conn.bucket] = bucket->el[--bucket->size];
    bucket->el[el->conn.bucket]->conn.bucket = el->conn.bucket;
    --q->size;
    if (q->size == 0)
        q->lowest_key = q->bucket_size;
    planPQBucketQueuePush(q, key, el);
}

static void planPQBucketQueueToHeapQueue(plan_pq_bucket_queue_t *b,
                                         plan_pq_heap_queue_t *h)
{
    plan_pq_bucket_t *bucket;
    int i, j;

    for (i = b->lowest_key; i < b->bucket_size; ++i){
        bucket = b->bucket + i;
        for (j = 0; j < bucket->size; ++j){
            planPQHeapQueuePush(h, i, bucket->el[j]);
        }
        if (bucket->el != NULL)
            BOR_FREE(bucket->el);
        bucket->el = NULL;
        bucket->size = bucket->alloc = 0;
    }
    b->size = 0;
}


static int heapLT(const bor_pairheap_node_t *_n1,
                  const bor_pairheap_node_t *_n2, void *_)
{
    plan_pq_el_t *e1 = bor_container_of(_n1, plan_pq_el_t, conn.heap);
    plan_pq_el_t *e2 = bor_container_of(_n2, plan_pq_el_t, conn.heap);
    return e1->key <= e2->key;
}

static void planPQHeapQueueInit(plan_pq_heap_queue_t *q)
{
    q->heap = borPairHeapNew(heapLT, NULL);
}

static void planPQHeapQueueFree(plan_pq_heap_queue_t *q)
{
    borPairHeapDel(q->heap);
}

static void planPQHeapQueuePush(plan_pq_heap_queue_t *q,
                                int key, plan_pq_el_t *el)
{
    el->key = key;
    borPairHeapAdd(q->heap, &el->conn.heap);
}

static plan_pq_el_t *planPQHeapQueuePop(plan_pq_heap_queue_t *q, int *key)
{
    bor_pairheap_node_t *hn;
    plan_pq_el_t *el;

    hn = borPairHeapExtractMin(q->heap);
    el = bor_container_of(hn, plan_pq_el_t, conn.heap);
    if (key)
        *key = el->key;
    return el;
}

static void planPQHeapQueueUpdate(plan_pq_heap_queue_t *q,
                                  int key, plan_pq_el_t *el)
{
    el->key = key;
    borPairHeapUpdate(q->heap, &el->conn.heap);
}
