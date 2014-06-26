#ifndef __PLAN_PRIOQUEUE_H__
#define __PLAN_PRIOQUEUE_H__

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
    int bucket_size;                     /*!< Number of buckets */
    int lowest_key;                      /*!< Lowest key so far */
    int size;                            /*!< Number of elements stored in
                                              queue */
};
typedef struct _plan_bucket_queue_t plan_bucket_queue_t;


/**
 * Initializes bucket-based priority queue.
 */
void planBucketQueueInit(plan_bucket_queue_t *q);

/**
 * Frees allocated resources.
 */
void planBucketQueueFree(plan_bucket_queue_t *q);

/**
 * Inserts an element into queue.
 */
void planBucketQueuePush(plan_bucket_queue_t *q, int key, int value);

/**
 * Removes and returns the lowest element.
 */
int planBucketQueuePop(plan_bucket_queue_t *q, int *key);

/**
 * Returns true if the queue is empty.
 */
_bor_inline int planBucketQueueEmpty(const plan_bucket_queue_t *q);


/**** INLINES ****/
_bor_inline int planBucketQueueEmpty(const plan_bucket_queue_t *q)
{
    return q->size == 0;
}

#endif /* __PLAN_PRIOQUEUE_H__ */
