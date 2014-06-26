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
    plan_operator_t *op;
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

static void planListLazyBucketDel(void *l);
static void planListLazyBucketPush(void *l,
                                   plan_cost_t cost,
                                   plan_state_id_t parent_state_id,
                                   plan_operator_t *op);
static int planListLazyBucketPop(void *l,
                                 plan_state_id_t *parent_state_id,
                                 plan_operator_t **op);
static void planListLazyBucketClear(void *l);


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

static void planListLazyBucketDel(void *_l)
{
    plan_list_lazy_bucket_t *l = _l;
    int i;

    planListLazyFree(&l->list_lazy);

    for (i = 0; i < l->bucket_size; ++i)
        borFifoFree(l->bucket + i);
    BOR_FREE(l->bucket);
    BOR_FREE(l);
}

static void planListLazyBucketPush(void *_l,
                                   plan_cost_t cost,
                                   plan_state_id_t parent_state_id,
                                   plan_operator_t *op)
{
    plan_list_lazy_bucket_t *l = _l;
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

static int planListLazyBucketPop(void *_l,
                                 plan_state_id_t *parent_state_id,
                                 plan_operator_t **op)
{
    plan_list_lazy_bucket_t *l = _l;
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

static void planListLazyBucketClear(void *_l)
{
    plan_list_lazy_bucket_t *l = _l;
    int i;

    for (i = l->lowest_key; i < l->bucket_size; ++i){
        borFifoClear(l->bucket + i);
    }
}
