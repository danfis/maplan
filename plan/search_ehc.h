#ifndef __PLAN_SEARCH_EHC_H__
#define __PLAN_SEARCH_EHC_H__

#include <plan/plan.h>
#include <plan/statespace_fifo.h>
#include <plan/succgen.h>
#include <plan/heuristic/goalcount.h>
#include <plan/path.h>

/**
 * Enforced Hill Climbing Search
 * ==============================
 */


struct _plan_search_ehc_t {
    plan_t *plan;
    plan_state_space_fifo_t *state_space;
    plan_heuristic_goalcount_t *heur;
    plan_state_t *state;
    plan_succ_gen_t *succ_gen;
    plan_operator_t **succ_op;
    unsigned best_heur;
    plan_state_id_t goal_state;
};
typedef struct _plan_search_ehc_t plan_search_ehc_t;

/**
 * Creates a new instance of the Enforced Hill Climbing search algorithm.
 */
plan_search_ehc_t *planSearchEHCNew(plan_t *plan);

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
