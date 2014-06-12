#ifndef __PLAN_SEARCH_EHC_H__
#define __PLAN_SEARCH_EHC_H__

#include <plan/search.h>
#include <plan/heur.h>
#include <plan/list_lazy.h>

/**
 * Enforced Hill Climbing Search
 * ==============================
 */

struct _plan_search_ehc_params_t {
    plan_search_params_t search; /*!< Common parameters */

    plan_heur_t *heur; /*!< Heuristic function that ought to be used */
};
typedef struct _plan_search_ehc_params_t plan_search_ehc_params_t;

void planSearchEHCParamsInit(plan_search_ehc_params_t *p);

struct _plan_search_ehc_t {
    plan_search_t search;

    plan_list_lazy_t *list;          /*!< List to keep track of the states */
    plan_heur_t *heur;               /*!< Heuristic function */
    plan_cost_t best_heur;           /*!< Value of the best heuristic
                                          value found so far */
};
typedef struct _plan_search_ehc_t plan_search_ehc_t;

/**
 * Creates a new instance of the Enforced Hill Climbing search algorithm.
 */
plan_search_t *planSearchEHCNew(const plan_search_ehc_params_t *params);

#endif /* __PLAN_SEARCH_EHC_H__ */
