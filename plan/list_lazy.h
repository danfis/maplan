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

#ifndef __PLAN_LIST_LAZY_H__
#define __PLAN_LIST_LAZY_H__

#include <plan/state.h>
#include <plan/op.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _plan_list_lazy_t plan_list_lazy_t;

typedef void (*plan_list_lazy_del_fn)(plan_list_lazy_t *);
typedef void (*plan_list_lazy_push_fn)(plan_list_lazy_t *, plan_cost_t cost,
                                       plan_state_id_t parent_state_id,
                                       plan_op_t *op);
typedef int (*plan_list_lazy_pop_fn)(plan_list_lazy_t *,
                                     plan_state_id_t *parent_state_id,
                                     plan_op_t **op);
typedef void (*plan_list_lazy_clear_fn)(plan_list_lazy_t *);

struct _plan_list_lazy_t {
    plan_list_lazy_del_fn del_fn;
    plan_list_lazy_push_fn push_fn;
    plan_list_lazy_pop_fn pop_fn;
    plan_list_lazy_clear_fn clear_fn;
};

/**
 * Creates a new lazy fifo list.
 * This list ignores cost argument in push() method.
 */
plan_list_lazy_t *planListLazyFifoNew(void);

/**
 * Creates a new lazy list based on heap.
 */
plan_list_lazy_t *planListLazyHeapNew(void);

/**
 * Creates a new bucket-based lazy list.
 */
plan_list_lazy_t *planListLazyBucketNew(void);

/**
 * Lazy list based on red-black tree
 */
plan_list_lazy_t *planListLazyRBTreeNew(void);

/**
 * Lazy list based on red-black tree
 */
plan_list_lazy_t *planListLazySplayTreeNew(void);

/**
 * Destroys the list.
 */
_bor_inline void planListLazyDel(plan_list_lazy_t *l);

/**
 * Inserts an element with the specified cost into the list.
 */
_bor_inline void planListLazyPush(plan_list_lazy_t *l,
                                  plan_cost_t cost,
                                  plan_state_id_t parent_state_id,
                                  plan_op_t *op);

/**
 * Pops the next element from the list that has the lowest cost.
 * Returns 0 on success, -1 if the heap is empty.
 */
_bor_inline int planListLazyPop(plan_list_lazy_t *l,
                                plan_state_id_t *parent_state_id,
                                plan_op_t **op);

/**
 * Empties the list.
 */
_bor_inline void planListLazyClear(plan_list_lazy_t *l);

/**** INLINES ****/
_bor_inline void planListLazyDel(plan_list_lazy_t *l)
{
    l->del_fn(l);
}

_bor_inline void planListLazyPush(plan_list_lazy_t *l,
                                  plan_cost_t cost,
                                  plan_state_id_t parent_state_id,
                                  plan_op_t *op)
{
    l->push_fn(l, cost, parent_state_id, op);
}

_bor_inline int planListLazyPop(plan_list_lazy_t *l,
                                plan_state_id_t *parent_state_id,
                                plan_op_t **op)
{
    return l->pop_fn(l, parent_state_id, op);
}

_bor_inline void planListLazyClear(plan_list_lazy_t *l)
{
    l->clear_fn(l);
}


/**
 * Initializes lazy list.
 */
void planListLazyInit(plan_list_lazy_t *l,
                      plan_list_lazy_del_fn del_fn,
                      plan_list_lazy_push_fn push_fn,
                      plan_list_lazy_pop_fn pop_fn,
                      plan_list_lazy_clear_fn clear_fn);

/**
 * Frees resources.
 */
void planListLazyFree(plan_list_lazy_t *l);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_LIST_LAZY_H__ */
