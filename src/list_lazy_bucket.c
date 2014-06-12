#include <boruvka/alloc.h>
#include "plan/dataarr.h"
#include "plan/list_lazy.h"

struct _plan_list_lazy_bucket_t {
    plan_list_lazy_t list_lazy;
    plan_data_arr_t *bucket;
    long size;
    plan_cost_t lowest_key;
};
typedef struct _plan_list_lazy_bucket_t plan_list_lazy_bucket_t;

struct _node_t {
    plan_cost_t cost;
    plan_state_id_t parent_state_id;
    plan_operator_t *op;
    bor_list_t list;
};
typedef struct _node_t node_t;

static void bucketInit(void *el, const void *userdata);
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

    b = BOR_ALLOC(plan_list_lazy_bucket_t);
    b->bucket = planDataArrNew(sizeof(bor_list_t), bucketInit, NULL);
    b->size = 0;
    b->lowest_key = 0;

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
    planListLazyFree(&l->list_lazy);
    planListLazyBucketClear(l);
    if (l->bucket)
        planDataArrDel(l->bucket);
    BOR_FREE(l);
}

static void planListLazyBucketPush(void *_l,
                                   plan_cost_t cost,
                                   plan_state_id_t parent_state_id,
                                   plan_operator_t *op)
{
    plan_list_lazy_bucket_t *l = _l;
    node_t *n;
    bor_list_t *bucket;

    n = BOR_ALLOC(node_t);
    n->cost = cost;
    n->parent_state_id = parent_state_id;
    n->op = op;
    borListInit(&n->list);

    bucket = planDataArrGet(l->bucket, cost);
    borListAppend(bucket, &n->list);
    ++l->size;

    if (cost < l->lowest_key)
        l->lowest_key = cost;
}

static int planListLazyBucketPop(void *_l,
                                 plan_state_id_t *parent_state_id,
                                 plan_operator_t **op)
{
    plan_list_lazy_bucket_t *l = _l;
    bor_list_t *bucket, *item;
    node_t *node;

    if (l->size == 0)
        return -1;

    bucket = planDataArrGet(l->bucket, l->lowest_key);
    while (borListEmpty(bucket)){
        ++l->lowest_key;
        bucket = planDataArrGet(l->bucket, l->lowest_key);
    }

    item = borListNext(bucket);
    borListDel(item);
    --l->size;
    node = BOR_LIST_ENTRY(item, node_t, list);

    *parent_state_id = node->parent_state_id;
    *op = node->op;

    BOR_FREE(node);
    return 0;
}

static void planListLazyBucketClear(void *_l)
{
    plan_list_lazy_bucket_t *l = _l;
    plan_state_id_t sid;
    plan_operator_t *op;
    while (planListLazyBucketPop(l, &sid, &op) == 0);
}

static void bucketInit(void *el, const void *userdata)
{
    bor_list_t *n = el;
    borListInit(n);
}
