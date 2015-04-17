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
#include <boruvka/fifo.h>
#include "plan/list.h"

/** A structure containing a stored value */
struct _node_t {
    plan_state_id_t state_id;
};
typedef struct _node_t node_t;

/** A node holding a key and all the values. */
struct _keynode_t {
    bor_fifo_t fifo;              /*!< Structure containing all values */
    struct _keynode_t *spe_left;  /*!< Connector to splay-tree */
    struct _keynode_t *spe_right; /*!< Connector to splay-tree */
    /** Here are stored cost values */
};
typedef struct _keynode_t keynode_t;

/** Returns array of cost values from key-node pointer */
#define KEYNODE_COST(knode) \
    ((plan_cost_t *)(((char *)(knode)) + sizeof(keynode_t)))

/** Main structure */
struct _plan_list_tiebreaking_t {
    plan_list_t list;
    keynode_t *root;        /*!< Root of splay-tree */
    keynode_t *pre_keynode; /*!< Preinitialized key-node */
    int size;               /*!< Number of cost values the list is
                                 considering */
};
typedef struct _plan_list_tiebreaking_t plan_list_tiebreaking_t;

#define LIST_FROM_PARENT(parent) \
    bor_container_of(parent, plan_list_tiebreaking_t, list)


static void planListTieBreakingDel(plan_list_t *list);
static void planListTieBreakingPush(plan_list_t *list,
                                    const plan_cost_t *cost,
                                    plan_state_id_t state_id);
static int planListTieBreakingPop(plan_list_t *list,
                                  plan_state_id_t *state_id,
                                  plan_cost_t *cost);
static int planListTieBreakingTop(plan_list_t *list,
                                  plan_state_id_t *state_id,
                                  plan_cost_t *cost);
static void planListTieBreakingClear(plan_list_t *list);


_bor_inline int keynodeCmp(const plan_cost_t *kn1,
                           const plan_cost_t *kn2, int size);
static keynode_t *keynodeNew(int size);
static void keynodeDel(keynode_t *kn);

/** Define splay-tree structure */
#define BOR_SPLAY_TREE_NODE_T keynode_t
#define BOR_SPLAY_TREE_T plan_list_tiebreaking_t
#define BOR_SPLAY_KEY_T const plan_cost_t *
#define BOR_SPLAY_NODE_KEY(node) KEYNODE_COST(node)
#define BOR_SPLAY_NODE_SET_KEY(head, node, key) \
    memcpy(KEYNODE_COST(node), (key), (head)->size * sizeof(plan_cost_t))
#define BOR_SPLAY_KEY_CMP(head, key1, key2) \
    keynodeCmp((key1), (key2), (head)->size)
#include "boruvka/splaytree_def.h"

plan_list_t *planListTieBreaking(int num_costs)
{
    plan_list_tiebreaking_t *list;

    list = BOR_ALLOC(plan_list_tiebreaking_t);
    _planListInit(&list->list,
                  planListTieBreakingDel,
                  planListTieBreakingPush,
                  planListTieBreakingPop,
                  planListTieBreakingTop,
                  planListTieBreakingClear);
    list->size = num_costs;
    list->pre_keynode = keynodeNew(list->size);

    borSplayInit(list);

    return &list->list;
}

static void planListTieBreakingDel(plan_list_t *_list)

{
    plan_list_tiebreaking_t *list = LIST_FROM_PARENT(_list);
    planListTieBreakingClear(&list->list);
    if (list->pre_keynode)
        keynodeDel(list->pre_keynode);
    borSplayFree(list);
    _planListFree(&list->list);
    BOR_FREE(list);
}

static void planListTieBreakingPush(plan_list_t *_list,
                                    const plan_cost_t *cost,
                                    plan_state_id_t state_id)
{
    plan_list_tiebreaking_t *list = LIST_FROM_PARENT(_list);
    keynode_t *kn;
    node_t node;

    // Try to insert pre-allocated key-node
    kn = borSplayInsert(list, cost, list->pre_keynode);

    if (kn == NULL){
        // Insertion was successful, remember the inserted key-node and
        // preallocate next key-node for next time.
        kn = list->pre_keynode;
        list->pre_keynode = keynodeNew(list->size);
    }

    // Push next node into key-node container
    node.state_id = state_id;
    borFifoPush(&kn->fifo, &node);
}

static int planListTieBreakingPop(plan_list_t *_list,
                                  plan_state_id_t *state_id,
                                  plan_cost_t *cost)
{
    plan_list_tiebreaking_t *list = LIST_FROM_PARENT(_list);
    keynode_t *kn;
    node_t *n;

    if (list->root == NULL)
        return -1;

    // Find out minimal node
    kn = borSplayMin(list);

    // We know for sure that this key-node must contain some nodes because
    // an empty key-nodes are removed immediately.
    // Pop next node from the key-node.
    n = borFifoFront(&kn->fifo);
    *state_id = n->state_id;
    memcpy(cost, KEYNODE_COST(kn), sizeof(plan_cost_t) * list->size);
    borFifoPop(&kn->fifo);

    // If the key-node is empty, remove it from the tree
    if (borFifoEmpty(&kn->fifo)){
        borSplayRemove(list, kn);
        keynodeDel(kn);
    }

    return 0;
}

static int planListTieBreakingTop(plan_list_t *_list,
                                  plan_state_id_t *state_id,
                                  plan_cost_t *cost)
{
    plan_list_tiebreaking_t *list = LIST_FROM_PARENT(_list);
    keynode_t *kn;
    node_t *n;

    if (list->root == NULL)
        return -1;

    // Find out minimal node
    kn = borSplayMin(list);

    // Get the next node from the key-node.
    n = borFifoFront(&kn->fifo);
    *state_id = n->state_id;
    memcpy(cost, KEYNODE_COST(kn), sizeof(plan_cost_t) * list->size);
    return 0;
}

static void planListTieBreakingClear(plan_list_t *_list)
{
    plan_list_tiebreaking_t *list = LIST_FROM_PARENT(_list);
    keynode_t *kn;

    while (list->root){
        kn = list->root;
        borSplayRemove(list, list->root);
        keynodeDel(kn);
    }
}

_bor_inline int keynodeCmp(const plan_cost_t *kn1,
                           const plan_cost_t *kn2, int size)
{
    int i, cmp;
    for (i = 0; i < size && (cmp = kn1[i] - kn2[i]) == 0; ++i);
    return cmp;
}

static keynode_t *keynodeNew(int size)
{
    keynode_t *kn;
    kn = BOR_MALLOC(sizeof(keynode_t) + sizeof(plan_cost_t) * size);
    borFifoInit(&kn->fifo, sizeof(node_t));
    return kn;
}

static void keynodeDel(keynode_t *kn)
{
    borFifoFree(&kn->fifo);
    BOR_FREE(kn);
}
