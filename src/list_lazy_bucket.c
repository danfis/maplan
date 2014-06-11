#include <boruvka/alloc.h>
#include "plan/list_lazy_bucket.h"

struct _node_t {
    plan_cost_t cost;
    plan_state_id_t parent_state_id;
    plan_operator_t *op;
    bor_list_t list;
};
typedef struct _node_t node_t;

static void bucketInit(void *el, const void *userdata);

plan_list_lazy_bucket_t *planListLazyBucketNew(void)
{
    plan_list_lazy_bucket_t *b;

    b = BOR_ALLOC(plan_list_lazy_bucket_t);
    b->bucket = planDataArrNew(sizeof(bor_list_t), bucketInit, NULL);
    b->size = 0;
    b->lowest_key = 0;

    return b;
}

void planListLazyBucketDel(plan_list_lazy_bucket_t *l)
{
    planListLazyBucketClear(l);
    if (l->bucket)
        planDataArrDel(l->bucket);
    BOR_FREE(l);
}

void planListLazyBucketPush(plan_list_lazy_bucket_t *l,
                            plan_cost_t cost,
                            plan_state_id_t parent_state_id,
                            plan_operator_t *op)
{
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

int planListLazyBucketPop(plan_list_lazy_bucket_t *l,
                          plan_state_id_t *parent_state_id,
                          plan_operator_t **op)
{
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

void planListLazyBucketClear(plan_list_lazy_bucket_t *l)
{
    plan_state_id_t sid;
    plan_operator_t *op;
    while (planListLazyBucketPop(l, &sid, &op) == 0);
}

static void bucketInit(void *el, const void *userdata)
{
    bor_list_t *n = el;
    borListInit(n);
}
