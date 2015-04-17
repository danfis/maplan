/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
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

#include <boruvka/alloc.h>
#include <boruvka/rbtree_int.h>
#include <boruvka/splaytree_int.h>
#include <boruvka/fifo.h>
#include "plan/list_lazy.h"

/** A structure containing a stored value */
struct _node_t {
    plan_state_id_t parent_state_id;
    plan_op_t *op;
};
typedef struct _node_t node_t;

/** A node holding a key and all the values. */
struct _keynode_t {
    bor_fifo_t fifo;               /*!< Structure containing all values */
    TREE_NODE_T tree; /*!< Connector to the tree */
};
typedef struct _keynode_t keynode_t;

/** A main structure */
struct _plan_list_lazy_map_t {
    plan_list_lazy_t list;    /*!< Parent class */
    TREE_T tree; /*!< Instance of a tree */
    keynode_t *pre_keynode;   /*!< Preinitialized key-node */
};
typedef struct _plan_list_lazy_map_t plan_list_lazy_map_t;

#define LIST_FROM_PARENT(l) \
    bor_container_of((l), plan_list_lazy_map_t, list)

static void planListLazyMapDel(plan_list_lazy_t *);
static void planListLazyMapPush(plan_list_lazy_t *,
                                plan_cost_t cost,
                                plan_state_id_t parent_state_id,
                                plan_op_t *op);
static int planListLazyMapPop(plan_list_lazy_t *,
                              plan_state_id_t *parent_state_id,
                              plan_op_t **op);
static void planListLazyMapClear(plan_list_lazy_t *);


plan_list_lazy_t *NEW_FN(void)
{
    plan_list_lazy_map_t *l;

    l = BOR_ALLOC(plan_list_lazy_map_t);
    TREE_INIT(&l->tree);

    l->pre_keynode = BOR_ALLOC(keynode_t);
    borFifoInit(&l->pre_keynode->fifo, sizeof(node_t));

    planListLazyInit(&l->list,
                     planListLazyMapDel,
                     planListLazyMapPush,
                     planListLazyMapPop,
                     planListLazyMapClear);

    return &l->list;
}

static void planListLazyMapDel(plan_list_lazy_t *_l)
{
    plan_list_lazy_map_t *l = LIST_FROM_PARENT(_l);
    planListLazyMapClear(_l);
    if (l->pre_keynode){
        borFifoFree(&l->pre_keynode->fifo);
        BOR_FREE(l->pre_keynode);
    }
    TREE_FREE(&l->tree);
    BOR_FREE(l);
}

static void planListLazyMapPush(plan_list_lazy_t *_l,
                                plan_cost_t cost,
                                plan_state_id_t parent_state_id,
                                plan_op_t *op)
{
    plan_list_lazy_map_t *l = LIST_FROM_PARENT(_l);
    node_t n;
    keynode_t *keynode;
    TREE_NODE_T *kn;

    // Try tu push the preinitialized keynode with the cost as a key
    kn = TREE_INSERT(&l->tree, cost, &l->pre_keynode->tree);

    if (kn == NULL){
        // The insertion was successful, so remember the inserted key-node
        // and preinitialize a next one.
        keynode = l->pre_keynode;
        l->pre_keynode = BOR_ALLOC(keynode_t);
        borFifoInit(&l->pre_keynode->fifo, sizeof(node_t));

    }else{
        // Key already in tree
        keynode = bor_container_of(kn, keynode_t, tree);
    }

    // Set up the actual values and insert it into key-node.
    n.parent_state_id = parent_state_id;
    n.op = op;
    borFifoPush(&keynode->fifo, &n);
}

static int planListLazyMapPop(plan_list_lazy_t *_l,
                              plan_state_id_t *parent_state_id,
                              plan_op_t **op)
{
    plan_list_lazy_map_t *l = LIST_FROM_PARENT(_l);
    node_t *n;
    keynode_t *keynode;
    TREE_NODE_T *kn;

    if (TREE_EMPTY(&l->tree))
        return -1;

    // Get the minimal key-node
    kn = TREE_MIN(&l->tree);
    keynode = bor_container_of(kn, keynode_t, tree);

    // We know for sure that this key-node must contain some values because
    // an empty key-nodes are removed.
    // Pop the values from the key-node.
    n = borFifoFront(&keynode->fifo);
    *parent_state_id = n->parent_state_id;
    *op              = n->op;
    borFifoPop(&keynode->fifo);

    // If the key-node is empty, remove it from the tree
    if (borFifoEmpty(&keynode->fifo)){
        TREE_REMOVE(&l->tree, &keynode->tree);
        borFifoFree(&keynode->fifo);
        BOR_FREE(keynode);
    }

    return 0;
}

static void planListLazyMapClear(plan_list_lazy_t *_l)
{
    plan_list_lazy_map_t *l = LIST_FROM_PARENT(_l);
    TREE_NODE_T *kn, *tmp;
    keynode_t *keynode;

    TREE_FOR_EACH_SAFE(&l->tree, kn, tmp){
        TREE_REMOVE(&l->tree, kn);
        keynode = bor_container_of(kn, keynode_t, tree);
        borFifoFree(&keynode->fifo);
        BOR_FREE(keynode);
    }
}
