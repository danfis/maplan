#ifndef __PLAN_SEARCH_LAZY_H__
#define __PLAN_SEARCH_LAZY_H__

#include <plan/problem.h>
#include <plan/statespace.h>
#include <plan/succgen.h>
#include <plan/heur.h>
#include <plan/path.h>
#include <plan/list_lazy_heap.h>

/**
 * Lazy Best First Search
 * =======================
 */


struct _plan_search_lazy_t {
    plan_problem_t *prob;            /*!< Structure with definition of
                                         the problem */
    plan_state_space_t *state_space;
    plan_list_lazy_heap_t *list;     /*!< List to keep track of the states */
    plan_heur_t *heur;               /*!< Heuristic function */
    plan_state_t *state;             /*!< Preallocated state structure */
    plan_succ_gen_t *succ_gen;       /*!< Successor operator generator */
    plan_operator_t **succ_op;       /*!< Preallocated array for successor
                                          operators. */
    plan_state_id_t goal_state;      /*!< The found state satisfying the goal */
};
typedef struct _plan_search_lazy_t plan_search_lazy_t;

/**
 * Creates a new instance of the Lazy Best First Search algorithm.
 */
plan_search_lazy_t *planSearchLazyNew(plan_problem_t *prob,
                                      plan_heur_t *heur);

/**
 * Frees all allocated resources.
 */
void planSearchLazyDel(plan_search_lazy_t *lazy);

/**
 * Searches for the path from the initial state to the goal.
 * Returns 0 is the search was successful, 1 if a dead end was reached and
 * -1 on error.
 * If a solution was found, it is returned if form of path via output
 * argument.
 */
int planSearchLazyRun(plan_search_lazy_t *lazy, plan_path_t *path);

#endif /* __PLAN_SEARCH_LAZY_H__ */

