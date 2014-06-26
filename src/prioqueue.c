#include <strings.h>
#include <stdio.h>
#include <boruvka/alloc.h>
#include "plan/prioqueue.h"

/** Initializes bucket-based priority queue. */
static void planBucketQueueInit(plan_bucket_queue_t *q);
/** Frees allocated resources. */
static void planBucketQueueFree(plan_bucket_queue_t *q);
/** Inserts an element into queue. */
static void planBucketQueuePush(plan_bucket_queue_t *q, int key, int value);
/** Removes and returns the lowest element. */
static int planBucketQueuePop(plan_bucket_queue_t *q, int *key);
/** Returns true if the queue is empty. */
_bor_inline int planBucketQueueEmpty(const plan_bucket_queue_t *q);

void planPrioQueueInit(plan_prio_queue_t *q)
{
    planBucketQueueInit(q);
}

void planPrioQueueFree(plan_prio_queue_t *q)
{
    planBucketQueueFree(q);
}

void planPrioQueuePush(plan_prio_queue_t *q, int key, int value)
{
    planBucketQueuePush(q, key, value);
}

int planPrioQueuePop(plan_prio_queue_t *q, int *key)
{
    return planBucketQueuePop(q, key);
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
        // TODO: remove constant
        bucket->alloc = 64;
        bucket->value = BOR_ALLOC_ARR(int, bucket->alloc);

    }else if (bucket->size + 1 > bucket->alloc){
        // TODO: constant
        bucket->alloc *= 2;
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

