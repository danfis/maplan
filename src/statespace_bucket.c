#include <boruvka/alloc.h>
#include <boruvka/list.h>
#include "plan/statespace_bucket.h"

struct _plan_state_space_bucket_node_t {
    plan_state_space_node_t node;
    bor_list_t bucket;
};
typedef struct _plan_state_space_bucket_node_t plan_state_space_bucket_node_t;

typedef unsigned (*plan_state_space_bucket_key_fn)(plan_state_space_node_t *n);

struct _plan_state_space_bucket_t {
    plan_state_space_t state_space;
    plan_data_arr_t *bucket_arr;
    size_t size;
    size_t lowest_key;
    plan_state_space_bucket_key_fn fn_key;
};
typedef struct _plan_state_space_bucket_t plan_state_space_bucket_t;

static plan_state_space_t *new(plan_state_pool_t *state_pool,
                               plan_state_space_bucket_key_fn key_fn);
static plan_state_space_node_t *pop(void *state_space);
static void insert(void *state_space, plan_state_space_node_t *node);

static unsigned keyCost(plan_state_space_node_t *n);
static unsigned keyHeuristic(plan_state_space_node_t *n);
static unsigned keyCostHeuristic(plan_state_space_node_t *n);
static void nodeInit(void *node, const void *data);
static void bucketInit(void *b, const void *data);

plan_state_space_t *planStateSpaceBucketCostNew(plan_state_pool_t *state_pool)
{
    return new(state_pool, keyCost);
}

plan_state_space_t *planStateSpaceBucketHeuristicNew(plan_state_pool_t *state_pool)
{
    return new(state_pool, keyHeuristic);
}

plan_state_space_t *planStateSpaceBucketCostHeuristicNew(plan_state_pool_t *state_pool)
{
    return new(state_pool, keyCostHeuristic);
}

void planStateSpaceBucketDel(plan_state_space_t *_ss)
{
    plan_state_space_bucket_t *ss = (plan_state_space_bucket_t *)_ss;
    planStateSpaceFree(&ss->state_space);
    if (ss->bucket_arr)
        planDataArrDel(ss->bucket_arr);
    BOR_FREE(ss);
}

static plan_state_space_t *new(plan_state_pool_t *state_pool,
                               plan_state_space_bucket_key_fn fn_key)
{
    plan_state_space_bucket_t *ss;

    ss = BOR_ALLOC(plan_state_space_bucket_t);

    planStateSpaceInit(&ss->state_space, state_pool,
                       sizeof(plan_state_space_bucket_node_t),
                       nodeInit, NULL,
                       pop, insert, NULL, NULL);

    ss->bucket_arr = planDataArrNew(sizeof(bor_list_t), 8196, bucketInit, NULL);
    ss->size = 0;
    ss->lowest_key = -1L;
    ss->fn_key = fn_key;

    return &ss->state_space;
}


static plan_state_space_node_t *pop(void *_ss)
{
    plan_state_space_bucket_t *ss = (plan_state_space_bucket_t *)_ss;
    bor_list_t *bucket, *item;
    plan_state_space_bucket_node_t *node;

    if (ss->size == 0)
        return NULL;

    bucket = planDataArrGet(ss->bucket_arr, ss->lowest_key);
    while (borListEmpty(bucket)){
        ++ss->lowest_key;
        bucket = planDataArrGet(ss->bucket_arr, ss->lowest_key);
    }

    item = borListNext(bucket);
    borListDel(item);
    --ss->size;
    node = BOR_LIST_ENTRY(item, plan_state_space_bucket_node_t, bucket);
    return &node->node;
}

static void insert(void *_ss, plan_state_space_node_t *_n)
{
    plan_state_space_bucket_t *ss = (plan_state_space_bucket_t *)_ss;
    plan_state_space_bucket_node_t *n = (plan_state_space_bucket_node_t *)_n;
    bor_list_t *bucket;
    unsigned key;

    key = ss->fn_key(&n->node);
    bucket = planDataArrGet(ss->bucket_arr, key);
    borListAppend(bucket, &n->bucket);
    if (key < ss->lowest_key)
        ss->lowest_key = key;
    ++ss->size;
}

static unsigned keyCost(plan_state_space_node_t *n)
{
    return n->cost;
}

static unsigned keyHeuristic(plan_state_space_node_t *n)
{
    return n->heuristic;
}

static unsigned keyCostHeuristic(plan_state_space_node_t *n)
{
    return n->cost + n->heuristic;
}

static void nodeInit(void *node, const void *data)
{
    plan_state_space_bucket_node_t *n = node;
    planStateSpaceNodeInit(&n->node);
    borListInit(&n->bucket);
}

static void bucketInit(void *b, const void *data)
{
    bor_list_t *bucket = b;
    borListInit(bucket);
}
