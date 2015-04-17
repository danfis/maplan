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

#ifndef __PLAN_LIST_H__
#define __PLAN_LIST_H__

#include <plan/state.h>
#include <plan/op.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***
 * Open Lists That Can Hold Only A State ID
 * =========================================
 */

typedef struct _plan_list_t plan_list_t;

/**
 * Destructor
 */
typedef void (*plan_list_del_fn)(plan_list_t *list);

/**
 * Inserts a state ID with a corresponding cost(s).
 */
typedef void (*plan_list_push_fn)(plan_list_t *list,
                                  const plan_cost_t *cost,
                                  plan_state_id_t state_id);

/**
 * Pops element with lowest cost from the list.
 * Returns 0 on success, -1 if the list is empty.
 */
typedef int (*plan_list_pop_fn)(plan_list_t *list,
                                plan_state_id_t *state_id,
                                plan_cost_t *cost);

/**
 * Peeks at the top of the list and returns an element with the lowest
 * cost.
 * Returns 0 on success, -1 if the list is empty.
 */
typedef int (*plan_list_top_fn)(plan_list_t *list,
                                plan_state_id_t *state_id,
                                plan_cost_t *cost);

/**
 * Removes all elements from the list.
 */
typedef void (*plan_list_clear_fn)(plan_list_t *list);

struct _plan_list_t {
    plan_list_del_fn del_fn;
    plan_list_push_fn push_fn;
    plan_list_pop_fn pop_fn;
    plan_list_top_fn top_fn;
    plan_list_clear_fn clear_fn;
};

/**
 * Creates a new tie-breaking list.
 * The argument {num_costs} specifies how many cost values will list use.
 */
plan_list_t *planListTieBreaking(int num_costs);

/**
 * Destroys the list.
 */
_bor_inline void planListDel(plan_list_t *l);

/**
 * Inserts an element with the specified cost into the list.
 */
_bor_inline void planListPush(plan_list_t *list,
                              const plan_cost_t *cost,
                              plan_state_id_t state_id);

/**
 * Pops the next element from the list that has the lowest cost.
 * Returns 0 on success, -1 if the heap is empty.
 */
_bor_inline int planListPop(plan_list_t *list,
                            plan_state_id_t *state_id,
                            plan_cost_t *cost);

/**
 * Peeks at the top of the list.
 * Returns 0 on success, -1 if the heap is empty.
 */
_bor_inline int planListTop(plan_list_t *list,
                            plan_state_id_t *state_id,
                            plan_cost_t *cost);

/**
 * Empties the list.
 */
_bor_inline void planListClear(plan_list_t *list);

/**** INLINES ****/
_bor_inline void planListDel(plan_list_t *l)
{
    l->del_fn(l);
}

_bor_inline void planListPush(plan_list_t *list,
                              const plan_cost_t *cost,
                              plan_state_id_t state_id)
{
    list->push_fn(list, cost, state_id);
}

_bor_inline int planListPop(plan_list_t *list,
                            plan_state_id_t *state_id,
                            plan_cost_t *cost)
{
    return list->pop_fn(list, state_id, cost);
}

_bor_inline int planListTop(plan_list_t *list,
                            plan_state_id_t *state_id,
                            plan_cost_t *cost)
{
    return list->top_fn(list, state_id, cost);
}

_bor_inline void planListClear(plan_list_t *l)
{
    l->clear_fn(l);
}


/**
 * Initializes parent object.
 */
void _planListInit(plan_list_t *l,
                   plan_list_del_fn del_fn,
                   plan_list_push_fn push_fn,
                   plan_list_pop_fn pop_fn,
                   plan_list_top_fn top_fn,
                   plan_list_clear_fn clear_fn);

/**
 * Frees resources of parent object.
 */
void _planListFree(plan_list_t *l);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_LIST_H__ */
