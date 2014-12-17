#ifndef __PLAN_SEARCH_LAZY_BASE_H__
#define __PLAN_SEARCH_LAZY_BASE_H__

#include <plan/search.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PLAN_SEARCH_LAZY_BASE_COST_ZERO 0
#define PLAN_SEARCH_LAZY_BASE_COST_HEUR 1

/**
 * Base structure for "lazy" search algorithms.
 */
struct _plan_search_lazy_base_t {
    plan_search_t search;

    plan_list_lazy_t *list; /*!< List to keep track of the states */
    int list_del;           /*!< True if .list should be deleted */
    int use_preferred_ops;  /*!< True if preferred operators from heuristic
                                 should be used. */
    int cost_type;
};
typedef struct _plan_search_lazy_base_t plan_search_lazy_base_t;

/**
 * Initializes lazy-base structure.
 * Note that .search structure must be initialized separately!!
 */
void planSearchLazyBaseInit(plan_search_lazy_base_t *lb,
                            plan_list_lazy_t *list, int list_del,
                            int use_preferred_ops,
                            int cost_type);

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
