#ifndef __PLAN_SEARCH_EHC_H__
#define __PLAN_SEARCH_EHC_H__

#include <boruvka/timer.h>
#include <plan/search.h>
#include <plan/problem.h>
#include <plan/statespace.h>
#include <plan/succgen.h>
#include <plan/heur.h>
#include <plan/path.h>
#include <plan/list_lazy.h>

/**
 * Enforced Hill Climbing Search
 * ==============================
 */

struct _plan_search_ehc_params_t {
    plan_search_progress_fn progress_fn;
    long progress_freq;

    plan_problem_t *prob;
    plan_heur_t *heur;
};
typedef struct _plan_search_ehc_params_t plan_search_ehc_params_t;

void planSearchEHCParamsInit(plan_search_ehc_params_t *p);

struct _plan_search_ehc_t {
    plan_search_ehc_params_t params;
    plan_search_stat_t stat;
    bor_timer_t timer;

    plan_problem_t *prob;            /*!< Structure with definition of
                                         the problem */
    plan_state_space_t *state_space;
    plan_list_lazy_t *list;          /*!< List to keep track of the states */
    plan_heur_t *heur;               /*!< Heuristic function */
    plan_state_t *state;             /*!< Preallocated state structure */
    plan_succ_gen_t *succ_gen;       /*!< Successor operator generator */
    plan_operator_t **succ_op;       /*!< Preallocated array for successor
                                          operators. */
    plan_cost_t best_heur;           /*!< Value of the best heuristic
                                          value found so far */
    plan_state_id_t goal_state;      /*!< The found state satisfying the goal */
};
typedef struct _plan_search_ehc_t plan_search_ehc_t;

/**
 * Creates a new instance of the Enforced Hill Climbing search algorithm.
 */
plan_search_ehc_t *planSearchEHCNew(const plan_search_ehc_params_t *params);

/**
 * Frees all allocated resources.
 */
void planSearchEHCDel(plan_search_ehc_t *ehc);

/**
 * Searches for the path from the initial state to the goal.
 * Returns 0 is the search was successful, 1 if a dead end was reached and
 * -1 on error.
 * If a solution was found, it is returned if form of path via output
 * argument.
 */
int planSearchEHCRun(plan_search_ehc_t *ehc, plan_path_t *path);


#endif /* __PLAN_SEARCH_EHC_H__ */
