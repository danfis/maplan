#include <boruvka/alloc.h>
#include <boruvka/rbtree_int.h>
#include <boruvka/fifo.h>
#include "plan/list_lazy.h"

struct _plan_list_lazy_multimap_t {
    plan_list_lazy_t list;
    bor_rbtree_int_t tree;
};
typedef struct _plan_list_lazy_multimap_t plan_list_lazy_multimap_t;

struct _node_t {
    plan_state_id_t parent_state_id;
    plan_operator_t *op;
};
typedef struct _node_t node_t;

struct _keynode_t {
    bor_fifo_t fifo;
    bor_rbtree_int_node_t tree;
};
typedef struct _keynode_t keynode_t;

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
    borRBTreeIntInit(&l->tree);

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
    borRBTreeIntFree(&l->tree);
    BOR_FREE(l);
}

static void planListLazyMultiMapPush(void *_l,
                                     plan_cost_t cost,
                                     plan_state_id_t parent_state_id,
                                     plan_operator_t *op)
{
    plan_list_lazy_multimap_t *l = _l;
    node_t n;
    keynode_t *keynode;
    bor_rbtree_int_node_t *kn;

    keynode = BOR_ALLOC(keynode_t);
    borFifoInit(&keynode->fifo, sizeof(node_t));
    kn = borRBTreeIntInsert(&l->tree, cost, &keynode->tree);
    if (kn != NULL){
        borFifoFree(&keynode->fifo);
        BOR_FREE(keynode);
        keynode = bor_container_of(kn, keynode_t, tree);
    }

    n.parent_state_id = parent_state_id;
    n.op = op;
    borFifoPush(&keynode->fifo, &n);
}

static int planListLazyMultiMapPop(void *_l,
                                   plan_state_id_t *parent_state_id,
                                   plan_operator_t **op)
{
    plan_list_lazy_multimap_t *l = _l;
    node_t *n;
    keynode_t *keynode;
    bor_rbtree_int_node_t *kn;

    if (borRBTreeIntEmpty(&l->tree))
        return -1;

    kn = borRBTreeIntMin(&l->tree);
    keynode = bor_container_of(kn, keynode_t, tree);

    n = borFifoFront(&keynode->fifo);
    *parent_state_id = n->parent_state_id;
    *op              = n->op;
    borFifoPop(&keynode->fifo);

    if (borFifoEmpty(&keynode->fifo)){
        borRBTreeIntRemove(&l->tree, &keynode->tree);
        borFifoFree(&keynode->fifo);
        BOR_FREE(keynode);
    }

    return 0;
}

static void planListLazyMultiMapClear(void *_l)
{
    plan_list_lazy_multimap_t *l = _l;
    bor_rbtree_int_node_t *kn, *tmp;
    keynode_t *keynode;

    BOR_RBTREE_INT_FOR_EACH_SAFE(&l->tree, kn, tmp){
        borRBTreeIntRemove(&l->tree, kn);
        keynode = bor_container_of(kn, keynode_t, tree);
        borFifoFree(&keynode->fifo);
        BOR_FREE(keynode);
    }
}
