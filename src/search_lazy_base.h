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
int planSearchLazyBaseInitStep(plan_search_lazy_base_t *lb, int heur_as_cost);

/**
 * Creates a new node from parent and operator.
 */
plan_state_space_node_t *planSearchLazyBaseExpandNode(
            plan_search_lazy_base_t *lb,
            plan_state_id_t parent_state_id,
            plan_op_t *parent_op,
            int *ret);

/**
 * Inserts a new node into open-list with specified cost.
 */
void planSearchLazyBaseInsertNode(plan_search_lazy_base_t *lb,
                                  plan_state_space_node_t *node,
                                  plan_cost_t cost);

/**
 * Adds successors of the specified state to the list in lazy fashion.
 * Applicable operators must be already loaded in .search->app_ops and
 * argument preferred must be one of PLAN_SEARCH_PREFERRED_* constants.
 */
void planSearchLazyBaseAddSuccessors(plan_search_lazy_base_t *lb,
                                     plan_state_id_t state_id,
                                     plan_cost_t cost);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_SEARCH_LAZY_BASE_H__ */
