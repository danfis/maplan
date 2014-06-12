#ifndef __PLAN_SEARCH_LAZY_H__
#define __PLAN_SEARCH_LAZY_H__

#include <plan/search.h>
#include <plan/heur.h>
#include <plan/list_lazy.h>

/**
 * Lazy Best First Search
 * =======================
 */

struct _plan_search_lazy_params_t {
    plan_search_params_t search; /*!< Common parameters */

    plan_heur_t *heur;      /*!< Heuristic function that ought to be used */
    plan_list_lazy_t *list; /*!< Lazy list that will be used. */
};
typedef struct _plan_search_lazy_params_t plan_search_lazy_params_t;

void planSearchLazyParamsInit(plan_search_lazy_params_t *p);

struct _plan_search_lazy_t {
    plan_search_t search;

    plan_list_lazy_t *list;          /*!< List to keep track of the states */
    plan_heur_t *heur;               /*!< Heuristic function */
};
typedef struct _plan_search_lazy_t plan_search_lazy_t;

/**
 * Creates a new instance of the Lazy Best First Search algorithm.
 */
plan_search_t *planSearchLazyNew(const plan_search_lazy_params_t *params);

#endif /* __PLAN_SEARCH_LAZY_H__ */
