#ifndef __PLAN_SEARCH_H__
#define __PLAN_SEARCH_H__

#include <plan/problem.h>
#include <plan/statespace.h>
#include <plan/heur.h>
#include <plan/list_lazy.h>
#include <plan/path.h>

#define PLAN_SEARCH_CONT      0
#define PLAN_SEARCH_FOUND     1
#define PLAN_SEARCH_DEAD_END -1
#define PLAN_SEARCH_ABORT    -2

/**
 * Struct for statistics from search.
 */
struct _plan_search_stat_t {
    float elapsed_time;
    long steps;
    long evaluated_states;
    long expanded_states;
    long generated_states;
    long peak_memory;
    int found_solution;
    int dead_end;
};
typedef struct _plan_search_stat_t plan_search_stat_t;

/**
 * Initializes stat struct.
 */
void planSearchStatInit(plan_search_stat_t *stat);

/**
 * Updates .peak_memory value of stat structure.
 */
void planSearchStatUpdatePeakMemory(plan_search_stat_t *stat);

_bor_inline void planSearchStatIncEvaluatedStates(plan_search_stat_t *stat);
_bor_inline void planSearchStatIncExpandedStates(plan_search_stat_t *stat);
_bor_inline void planSearchStatIncGeneratedStates(plan_search_stat_t *stat);
_bor_inline void planSearchStatSetFoundSolution(plan_search_stat_t *stat);
_bor_inline void planSearchStatSetDeadEnd(plan_search_stat_t *stat);


/**
 * Callback for progress monitoring.
 * The function should return PLAN_SEARCH_CONT if the process should
 * continue after this callback, or PLAN_SEARCH_ABORT if the process
 * should be stopped.
 */
typedef int (*plan_search_progress_fn)(const plan_search_stat_t *stat);

/**
 * Common parameters for all search algorithms.
 */
struct _plan_search_params_t {
    plan_search_progress_fn progress_fn; /*!< Callback for monitoring */
    long progress_freq;                  /*!< Frequence of calling
                                              .progress_fn as number of steps. */

    plan_problem_t *prob; /*!< Problem definition */
};
typedef struct _plan_search_params_t plan_search_params_t;

void planSearchParamsInit(plan_search_params_t *params);

typedef void (*plan_search_del_fn)(void *);
typedef int (*plan_search_init_fn)(void *);
typedef int (*plan_search_step_fn)(void *);

/**
 * Common base struct for all search algorithms.
 */
struct _plan_search_t {
    plan_search_del_fn del_fn;
    plan_search_init_fn init_fn;
    plan_search_step_fn step_fn;

    plan_search_params_t params;
    plan_search_stat_t stat;

    plan_state_space_t *state_space;
    plan_state_t *state;             /*!< Preallocated state */
    plan_operator_t **succ_op;       /*!< Preallocated array for successor
                                          operators. */
    plan_state_id_t goal_state;      /*!< The found state satisfying the goal */
};
typedef struct _plan_search_t plan_search_t;

/**
 * Deletes search object.
 */
void planSearchDel(plan_search_t *search);

/**
 * Searches for the path from the initial state to the goal as defined via
 * parameters.
 * Returns PLAN_SEARCH_FOUND if the solution was found and in this case the
 * path is returned via path argument.
 * If the plan was not found, PLAN_SEARCH_DEAD_END is returned.
 * If the search progress was aborted by the "progess" callback,
 * PLAN_SEARCH_ABORT is returned.
 */
int planSearchRun(plan_search_t *search, plan_path_t *path);



/**
 * Initializas the base search struct.
 */
void _planSearchInit(plan_search_t *search,
                     const plan_search_params_t *params,
                     plan_search_del_fn del_fn,
                     plan_search_init_fn init_fn,
                     plan_search_step_fn step_fn);

/**
 * Frees allocated resources.
 */
void _planSearchFree(plan_search_t *search);

/**
 * Returns value of heuristics for the given state.
 */
plan_cost_t _planSearchHeuristic(plan_search_t *search,
                                 plan_state_id_t state_id,
                                 plan_heur_t *heur);

/**
 * Adds state's successors to the lazy list with the specified cost.
 */
void _planSearchAddLazySuccessors(plan_search_t *search,
                                  plan_state_id_t state_id,
                                  plan_cost_t cost,
                                  plan_list_lazy_t *list);

/**
 * Let the common structure know that a dead end was reached.
 */
void _planSearchReachedDeadEnd(plan_search_t *search);

/**
 * Record the goal state.
 */
void _planSearchFoundSolution(plan_search_t *search,
                              plan_state_id_t state_id);


/**
 * Creates a new state by application of the operator on the parent_state.
 * Returns 0 if the corresponding node is in NEW state, -1 otherwise.
 * The resulting state and node is returned via output arguments.\
 */
int _planSearchNewState(plan_search_t *search,
                        plan_operator_t *operator,
                        plan_state_id_t parent_state,
                        plan_state_id_t *new_state_id,
                        plan_state_space_node_t **new_node);

/**** INLINES ****/
_bor_inline void planSearchStatIncEvaluatedStates(plan_search_stat_t *stat)
{
    ++stat->evaluated_states;
}

_bor_inline void planSearchStatIncExpandedStates(plan_search_stat_t *stat)
{
    ++stat->expanded_states;
}

_bor_inline void planSearchStatIncGeneratedStates(plan_search_stat_t *stat)
{
    ++stat->generated_states;
}

_bor_inline void planSearchStatSetFoundSolution(plan_search_stat_t *stat)
{
    stat->found_solution = 1;
}

_bor_inline void planSearchStatSetDeadEnd(plan_search_stat_t *stat)
{
    stat->dead_end = 1;
}

#endif /* __PLAN_SEARCH_H__ */
