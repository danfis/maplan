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
#include <boruvka/pairheap.h>
#include "plan/list_lazy.h"

struct _plan_list_lazy_heap_t {
    plan_list_lazy_t list;
    bor_pairheap_t *heap;
};
typedef struct _plan_list_lazy_heap_t plan_list_lazy_heap_t;

struct _heap_node_t {
    plan_cost_t cost;
    plan_state_id_t parent_state_id;
    plan_op_t *op;
    bor_pairheap_node_t heap; /*!< Connector to an open list */
};
typedef struct _heap_node_t heap_node_t;

#define LIST_FROM_PARENT(_list) \
    bor_container_of((_list), plan_list_lazy_heap_t, list)

static int heapLessThan(const bor_pairheap_node_t *n1,
                        const bor_pairheap_node_t *n2,
                        void *data);
static void planListLazyHeapDel(plan_list_lazy_t *);
static void planListLazyHeapPush(plan_list_lazy_t *,
                                 plan_cost_t cost,
                                 plan_state_id_t parent_state_id,
                                 plan_op_t *op);
static int planListLazyHeapPop(plan_list_lazy_t *,
                               plan_state_id_t *parent_state_id,
                               plan_op_t **op);
static void planListLazyHeapClear(plan_list_lazy_t *);


plan_list_lazy_t *planListLazyHeapNew(void)
{
    plan_list_lazy_heap_t *l;

    l = BOR_ALLOC(plan_list_lazy_heap_t);
    l->heap = borPairHeapNew(heapLessThan, NULL);
    planListLazyInit(&l->list,
                     planListLazyHeapDel,
                     planListLazyHeapPush,
                     planListLazyHeapPop,
                     planListLazyHeapClear);

    return &l->list;
}

static void planListLazyHeapDel(plan_list_lazy_t *_l)
{
    plan_list_lazy_heap_t *l = LIST_FROM_PARENT(_l);
    planListLazyHeapClear(_l);
    borPairHeapDel(l->heap);
    BOR_FREE(l);
}

static void planListLazyHeapPush(plan_list_lazy_t *_l,
                                 plan_cost_t cost,
                                 plan_state_id_t parent_state_id,
                                 plan_op_t *op)
{
    plan_list_lazy_heap_t *l = LIST_FROM_PARENT(_l);
    heap_node_t *n;

    n = BOR_ALLOC(heap_node_t);
    n->cost            = cost;
    n->parent_state_id = parent_state_id;
    n->op              = op;
    borPairHeapAdd(l->heap, &n->heap);
}

static int planListLazyHeapPop(plan_list_lazy_t *_l,
                               plan_state_id_t *parent_state_id,
                               plan_op_t **op)
{
    plan_list_lazy_heap_t *l = LIST_FROM_PARENT(_l);
    bor_pairheap_node_t *heap_node;
    heap_node_t *n;

    if (borPairHeapEmpty(l->heap))
        return -1;

    heap_node = borPairHeapExtractMin(l->heap);
    n = bor_container_of(heap_node, heap_node_t, heap);

    *parent_state_id = n->parent_state_id;
    *op              = n->op;
    BOR_FREE(n);

    return 0;
}

static void clearFn(bor_pairheap_node_t *pn, void *_)
{
    heap_node_t *n = bor_container_of(pn, heap_node_t, heap);
    BOR_FREE(n);
}
static void planListLazyHeapClear(plan_list_lazy_t *_l)
{
    plan_list_lazy_heap_t *l = LIST_FROM_PARENT(_l);
    borPairHeapClear(l->heap, clearFn, NULL);
}


static int heapLessThan(const bor_pairheap_node_t *_n1,
                        const bor_pairheap_node_t *_n2,
                        void *data)
{
    heap_node_t *n1 = bor_container_of(_n1, heap_node_t, heap);
    heap_node_t *n2 = bor_container_of(_n2, heap_node_t, heap);
    return n1->cost <= n2->cost;
}
