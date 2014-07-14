#ifndef __PLAN_SEARCH_H__
#define __PLAN_SEARCH_H__

#include <plan/problem.h>
#include <plan/statespace.h>
#include <plan/heur.h>
#include <plan/list_lazy.h>
#include <plan/path.h>

/** Forward declaration */
typedef struct _plan_search_t plan_search_t;

/**
 * Search Algorithms
 * ==================
 */

/**
 * Status that search can (and should) continue. Mostly for internal use.
 */
#define PLAN_SEARCH_CONT       0

/**
 * Status signaling that a solution was found.
 */
#define PLAN_SEARCH_FOUND      1

/**
 * No solution was found.
 */
#define PLAN_SEARCH_NOT_FOUND -1

/**
 * The search process was aborted from outside, e.g., from progress
 * callback.
 */
#define PLAN_SEARCH_ABORT     -2

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
    int found;
    int not_found;
};
typedef struct _plan_search_stat_t plan_search_stat_t;

/**
 * Initializes stat struct.
 */
void planSearchStatInit(plan_search_stat_t *stat);


/**
 * Structure with record of changes made during one step of search
 * algorithm (see planSearchStep()).
 */
struct _plan_search_step_change_t {
    plan_state_space_node_t **closed_node; /*!< Closed nodes */
    int closed_node_size;                  /*!< Number of closed nodes */
    int closed_node_alloc;                 /*!< Allocated space */
};
typedef struct _plan_search_step_change_t plan_search_step_change_t;

/**
 * Initializes plan_search_step_change_t structure.
 */
void planSearchStepChangeInit(plan_search_step_change_t *step_change);

/**
 * Frees all resources allocated within plan_search_step_change_t
 * structure.
 */
void planSearchStepChangeFree(plan_search_step_change_t *step_change);

/**
 * Reset record of the structure.
 */
void planSearchStepChangeReset(plan_search_step_change_t *step_change);

/**
 * Add record about one closed node.
 */
void planSearchStepChangeAddClosedNode(plan_search_step_change_t *step_change,
                                       plan_state_space_node_t *node);



/**
 * Callback for progress monitoring.
 * The function should return PLAN_SEARCH_CONT if the process should
 * continue after this callback, or PLAN_SEARCH_ABORT if the process
 * should be stopped.
 */
typedef int (*plan_search_progress_fn)(const plan_search_stat_t *stat);


/**
 * Callback called each time a node is closed.
 */
typedef void (*plan_search_run_node_closed)(plan_state_space_node_t *node,
                                            void *data);

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


/**
 * Enforced Hill Climbing Search Algorithm
 * ----------------------------------------
 */
struct _plan_search_ehc_params_t {
    plan_search_params_t search; /*!< Common parameters */

    plan_heur_t *heur; /*!< Heuristic function that ought to be used */
    int heur_del;      /*!< True if .heur should be deleted in
                            planSearchDel() */
};
typedef struct _plan_search_ehc_params_t plan_search_ehc_params_t;

/**
 * Initializes parameters of EHC algorithm.
 */
void planSearchEHCParamsInit(plan_search_ehc_params_t *p);

/**
 * Creates a new instance of the Enforced Hill Climbing search algorithm.
 */
plan_search_t *planSearchEHCNew(const plan_search_ehc_params_t *params);



/**
 * Lazy Best First Search Algorithm
 * ---------------------------------
 */
struct _plan_search_lazy_params_t {
    plan_search_params_t search; /*!< Common parameters */

    plan_heur_t *heur;      /*!< Heuristic function that ought to be used */
    int heur_del;           /*!< True if .heur should be deleted in
                                 planSearchDel() */
    plan_list_lazy_t *list; /*!< Lazy list that will be used. */
    int list_del;           /*!< True if .list should be deleted in
                                 planSearchDel() */
};
typedef struct _plan_search_lazy_params_t plan_search_lazy_params_t;

/**
 * Initializes parameters of Lazy algorithm.
 */
void planSearchLazyParamsInit(plan_search_lazy_params_t *p);

/**
 * Creates a new instance of the Lazy Best First Search algorithm.
 */
plan_search_t *planSearchLazyNew(const plan_search_lazy_params_t *params);



/**
 * Common Functions
 * -----------------
 */

/**
 * Deletes search object.
 */
void planSearchDel(plan_search_t *search);

/**
 * Searches for the path from the initial state to the goal as defined via
 * parameters.
 * Returns PLAN_SEARCH_FOUND if the solution was found and in this case the
 * path is returned via path argument.
 * If the plan was not found, PLAN_SEARCH_NOT_FOUND is returned.
 * If the search progress was aborted by the "progess" callback,
 * PLAN_SEARCH_ABORT is returned.
 */
int planSearchRun(plan_search_t *search, plan_path_t *path);

/**
 * Extracts path from initial state to the goal state (if was found) using
 * back-tracking from the goal state.
 */
void planSearchBackTrackPath(plan_search_t *search, plan_path_t *path);

/**
 * Simliar to planSearchBackTrackPath() but starting state for
 * back-tracking is specified.
 */
void planSearchBackTrackPathFrom(plan_search_t *search,
                                 plan_state_id_t from_state,
                                 plan_path_t *path);

/**
 * Performs initial step which should be insertion of the initial state.
 * Returns PLAN_SEARCH_CONT or PLAN_SEARCH_FOUND.
 */
_bor_inline int planSearchInitStep(plan_search_t *search);

/**
 * Performs one step in search.
 * Returns one of PLAN_SEARCH_{CONT,FOUND,NOT_FOUND,ABORT}.
 * If {change} is non-NULL it is filled with record of changes made during
 * that step.
 */
_bor_inline int planSearchStep(plan_search_t *search,
                               plan_search_step_change_t *change);

/**
 * Request for injection of the state into open-list.
 * Returns 0 if the state was injected, -1 otherwise.
 */
_bor_inline int planSearchInjectState(plan_search_t *search,
                                      plan_state_id_t state_id,
                                      plan_cost_t cost,
                                      plan_cost_t heuristic);

/**
 * Internals
 * ----------
 */

/**
 * Initializes common parameters.
 * This should be called from all *ParamsInit() functions of particular
 * algorithms.
 */
void planSearchParamsInit(plan_search_params_t *params);

/**
 * Updates .peak_memory value of stat structure.
 */
void planSearchStatUpdatePeakMemory(plan_search_stat_t *stat);

/**
 * Increments number of evaluated states by one.
 */
_bor_inline void planSearchStatIncEvaluatedStates(plan_search_stat_t *stat);

/**
 * Increments number of expanded states by one.
 */
_bor_inline void planSearchStatIncExpandedStates(plan_search_stat_t *stat);

/**
 * Increments number of generated states by one.
 */
_bor_inline void planSearchStatIncGeneratedStates(plan_search_stat_t *stat);

/**
 * Set "found" flag which means that solution was found.
 */
_bor_inline void planSearchStatSetFoundSolution(plan_search_stat_t *stat);

/**
 * Sets "not_found" flag meaning no solution was found.
 */
_bor_inline void planSearchStatSetNotFound(plan_search_stat_t *stat);


/**
 * Algorithm's method that frees all resources.
 */
typedef void (*plan_search_del_fn)(void *);

/**
 * Initialize algorithm -- first step of algorithm.
 */
typedef int (*plan_search_init_fn)(void *);

/**
 * Perform one step of algorithm.
 */
typedef int (*plan_search_step_fn)(void *, plan_search_step_change_t *change);

/**
 * Inject the given state into open-list and performs another needed
 * operations with the state.
 * Returns 0 on success.
 */
typedef int (*plan_search_inject_state_fn)(void *search,
                                           plan_state_id_t state_id,
                                           plan_cost_t cost,
                                           plan_cost_t heuristic);

/**
 * Common base struct for all search algorithms.
 */
struct _plan_search_t {
    plan_search_del_fn del_fn;
    plan_search_init_fn init_fn;
    plan_search_step_fn step_fn;
    plan_search_inject_state_fn inject_state_fn;

    plan_search_params_t params;
    plan_search_stat_t stat;

    plan_state_pool_t *state_pool;   /*!< State pool from params.prob */
    plan_state_space_t *state_space;
    plan_state_t *state;             /*!< Preallocated state */
    plan_operator_t **succ_op;       /*!< Preallocated array for successor
                                          operators. */
    plan_state_id_t goal_state;      /*!< The found state satisfying the goal */
};



/**
 * Initializas the base search struct.
 */
void _planSearchInit(plan_search_t *search,
                     const plan_search_params_t *params,
                     plan_search_del_fn del_fn,
                     plan_search_init_fn init_fn,
                     plan_search_step_fn step_fn,
                     plan_search_inject_state_fn inject_state_fn);

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
 * Generalization for lazy search algorithms.
 * Injects given state into open-list if the node wasn't discovered yet.
 */
int _planSearchLazyInjectState(plan_search_t *search,
                               plan_heur_t *heur,
                               plan_list_lazy_t *list,
                               plan_state_id_t state_id,
                               plan_cost_t cost, plan_cost_t heur_val);

/**
 * Let the common structure know that a dead end was reached.
 */
void _planSearchReachedDeadEnd(plan_search_t *search);

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

/**
 * Open and close the state in one step.
 */
plan_state_space_node_t *_planSearchNodeOpenClose(plan_search_t *search,
                                                  plan_state_id_t state,
                                                  plan_state_id_t parent_state,
                                                  plan_operator_t *parent_op,
                                                  plan_cost_t cost,
                                                  plan_cost_t heur);

/**
 * Returns true if the given state is the goal state.
 * Also the goal state is recorded in stats and the goal state is
 * remembered.
 */
int _planSearchCheckGoal(plan_search_t *search, plan_state_id_t state_id);

/**** INLINES ****/
_bor_inline int planSearchInitStep(plan_search_t *search)
{
    return search->init_fn(search);
}

_bor_inline int planSearchStep(plan_search_t *search,
                               plan_search_step_change_t *change)
{
    return search->step_fn(search, change);
}

_bor_inline int planSearchInjectState(plan_search_t *search,
                                      plan_state_id_t state_id,
                                      plan_cost_t cost,
                                      plan_cost_t heuristic)
{
    if (!search->inject_state_fn)
        return -1;
    return search->inject_state_fn(search, state_id, cost, heuristic);
}



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
    stat->found = 1;
}

_bor_inline void planSearchStatSetNotFound(plan_search_stat_t *stat)
{
    stat->not_found = 1;
}

#endif /* __PLAN_SEARCH_H__ */
