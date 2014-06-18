#include <boruvka/alloc.h>
#include <boruvka/bucketheap.h>
#include "plan/list_lazy.h"

struct _plan_list_lazy_bucket_t {
    plan_list_lazy_t list_lazy;
    bor_bucketheap_t *heap;
};
typedef struct _plan_list_lazy_bucket_t plan_list_lazy_bucket_t;

struct _node_t {
    plan_state_id_t parent_state_id;
    plan_operator_t *op;
    bor_bucketheap_node_t node;
};
typedef struct _node_t node_t;

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
    b->heap = borBucketHeapNew();

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
    if (l->heap)
        borBucketHeapDel(l->heap);
    BOR_FREE(l);
}

static void planListLazyBucketPush(void *_l,
                                   plan_cost_t cost,
                                   plan_state_id_t parent_state_id,
                                   plan_operator_t *op)
{
    plan_list_lazy_bucket_t *l = _l;
    node_t *n;

    n = BOR_ALLOC(node_t);
    n->parent_state_id = parent_state_id;
    n->op = op;
    borBucketHeapAdd(l->heap, cost, &n->node);
}

static int planListLazyBucketPop(void *_l,
                                 plan_state_id_t *parent_state_id,
                                 plan_operator_t **op)
{
    plan_list_lazy_bucket_t *l = _l;
    bor_bucketheap_node_t *bucket_node;
    node_t *node;

    if (borBucketHeapEmpty(l->heap))
        return -1;

    bucket_node = borBucketHeapExtractMin(l->heap, NULL);

    node = bor_container_of(bucket_node, node_t, node);
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
