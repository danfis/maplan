#include <boruvka/alloc.h>
#include <boruvka/multimap_int.h>
#include "plan/list_lazy.h"

struct _plan_list_lazy_multimap_t {
    plan_list_lazy_t list;
    bor_multimap_int_t *map;
};
typedef struct _plan_list_lazy_multimap_t plan_list_lazy_multimap_t;

struct _node_t {
    plan_state_id_t parent_state_id;
    plan_operator_t *op;
    bor_multimap_int_node_t node;
};
typedef struct _node_t node_t;

static void planListLazyMultiMapDel(void *);
static void planListLazyMultiMapPush(void *,
                                     plan_cost_t cost,
                                     plan_state_id_t parent_state_id,
                                     plan_operator_t *op);
static int planListLazyMultiMapPop(void *,
                                   plan_state_id_t *parent_state_id,
                                   plan_operator_t **op);
static void planListLazyMultiMapClear(void *);


plan_list_lazy_t *planListLazyMultiMapNew(void)
{
    plan_list_lazy_multimap_t *l;

    l = BOR_ALLOC(plan_list_lazy_multimap_t);
    l->map = borMultiMapIntNew();
    planListLazyInit(&l->list,
                     planListLazyMultiMapDel,
                     planListLazyMultiMapPush,
                     planListLazyMultiMapPop,
                     planListLazyMultiMapClear);

    return &l->list;
}

static void planListLazyMultiMapDel(void *_l)
{
    plan_list_lazy_multimap_t *l = _l;
    planListLazyMultiMapClear(l);
    borMultiMapIntDel(l->map);
    BOR_FREE(l);
}

static void planListLazyMultiMapPush(void *_l,
                                     plan_cost_t cost,
                                     plan_state_id_t parent_state_id,
                                     plan_operator_t *op)
{
    plan_list_lazy_multimap_t *l = _l;
    node_t *n;

    n = BOR_ALLOC(node_t);
    n->parent_state_id = parent_state_id;
    n->op              = op;
    borMultiMapIntInsert(l->map, cost, &n->node);
}

static int planListLazyMultiMapPop(void *_l,
                                   plan_state_id_t *parent_state_id,
                                   plan_operator_t **op)
{
    plan_list_lazy_multimap_t *l = _l;
    bor_multimap_int_node_t *map_node;
    node_t *n;

    if (borMultiMapIntEmpty(l->map))
        return -1;

    map_node = borMultiMapIntExtractMinNodeFifo(l->map, NULL);
    n = bor_container_of(map_node, node_t, node);

    *parent_state_id = n->parent_state_id;
    *op              = n->op;
    BOR_FREE(n);

    return 0;
}

static void planListLazyMultiMapClear(void *_l)
{
    plan_list_lazy_multimap_t *l = _l;
    plan_state_id_t sid;
    plan_operator_t *op;

    while (planListLazyMultiMapPop(l, &sid, &op) != -1);
}

