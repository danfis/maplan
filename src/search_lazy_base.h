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

#ifndef __PLAN_SEARCH_LAZY_BASE_H__
#define __PLAN_SEARCH_LAZY_BASE_H__

#include <plan/search.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Base structure for "lazy" search algorithms.
 */
struct _plan_search_lazy_base_t {
    plan_search_t search;

    plan_list_lazy_t *list; /*!< List to keep track of the states */
    int list_del;           /*!< True if .list should be deleted */
    int use_preferred_ops;  /*!< True if preferred operators from heuristic
                                 should be used. */
};
typedef struct _plan_search_lazy_base_t plan_search_lazy_base_t;

/**
 * Initializes lazy-base structure.
 * Note that .search structure must be initialized separately!!
 */
void planSearchLazyBaseInit(plan_search_lazy_base_t *lb,
                            plan_list_lazy_t *list, int list_del,
                            int use_preferred_ops);

/**
 * Frees resources.
 */
void planSearchLazyBaseFree(plan_search_lazy_base_t *lb);

/**
 * Initializes initial state.
 */
int planSearchLazyBaseInitStep(plan_search_t *search);

/**
 * Expands next node from the lazy list.
 */
int planSearchLazyBaseNext(plan_search_lazy_base_t *lb,
                           plan_state_space_node_t **node);

/**
 * Expands the given node into lazy list.
 */
void planSearchLazyBaseExpand(plan_search_lazy_base_t *lb,
                              plan_state_space_node_t *node);

/**
 * Insert-node callback for plan_search_t structure.
 */
void planSearchLazyBaseInsertNode(plan_search_t *search,
                                  plan_state_space_node_t *node);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_SEARCH_LAZY_BASE_H__ */
