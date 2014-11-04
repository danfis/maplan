#ifndef __PLAN_SEARCH_H__
#define __PLAN_SEARCH_H__

#include <boruvka/timer.h>
#include <boruvka/fifo.h>

#include <plan/problem.h>
#include <plan/statespace.h>
#include <plan/heur.h>
#include <plan/list_lazy.h>
#include <plan/path.h>
#include <plan/ma_comm.h>
#include <plan/search_stat.h>
#include <plan/search_applicable_ops.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Forward declaration */
typedef struct _plan_search_t plan_search_t;

/**
 * Search Algorithms
 * ==================
 */


/**
 * Callback for progress monitoring.
 * The function should return PLAN_SEARCH_CONT if the process should
 * continue after this callback, or PLAN_SEARCH_ABORT if the process
 * should be stopped.
 */
typedef int (*plan_search_progress_fn)(const plan_search_stat_t *stat,
                                       void *userdata);

struct _plan_search_progress_t {
    plan_search_progress_fn fn; /*!< Callback for monitoring */
    long freq;  /*!< Frequence of calling .progress_fn as number of steps. */
    void *data; /*!< User-provided data */
};
typedef struct _plan_search_progress_t plan_search_progress_t;

/**
 * Common parameters for all search algorithms.
 */
struct _plan_search_params_t {
    plan_search_progress_t progress; /*!< Progress callback */

    plan_heur_t *heur; /*!< Heuristic function that ought to be used */
    int heur_del;      /*!< True if .heur should be deleted in
                            planSearchDel() */

    plan_problem_t *prob; /*!< Problem definition */
};
typedef struct _plan_search_params_t plan_search_params_t;


struct _plan_search_ma_params_t {
    plan_ma_comm_t *comm;
};
typedef struct _plan_search_ma_params_t plan_search_ma_params_t;

/**
 * Enforced Hill Climbing Search Algorithm
 * ----------------------------------------
 */
struct _plan_search_ehc_params_t {
    plan_search_params_t search; /*!< Common parameters */
    int use_preferred_ops; /*!< One of PLAN_SEARCH_PREFERRED_* constants */
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

    int use_preferred_ops;  /*!< One of PLAN_SEARCH_PREFERRED_* constants */
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
 * A* Search Algorithm
 * --------------------
 */
struct _plan_search_astar_params_t {
    plan_search_params_t search; /*!< Common parameters */

    int pathmax; /*!< Use pathmax correction */
};
typedef struct _plan_search_astar_params_t plan_search_astar_params_t;

/**
 * Initializes parameters of A* algorithm.
 */
void planSearchAStarParamsInit(plan_search_astar_params_t *p);

/**
 * Creates a new instance of the A* search algorithm.
 */
plan_search_t *planSearchAStarNew(const plan_search_astar_params_t *params);



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


typedef int (*plan_search_poststep_fn)(plan_search_t *search, void *userdata);

/**
 * Sets post-step callback, i.e., function that is called after each step
 * is performed.
 */
void planSearchSetPostStep(plan_search_t *search,
                           plan_search_poststep_fn fn, void *userdata);

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
 * Algorithm's method that frees all resources.
 */
typedef void (*plan_search_del_fn)(plan_search_t *);

/**
 * Initialize algorithm -- first step of algorithm.
 */
typedef int (*plan_search_init_step_fn)(plan_search_t *);

/**
 * Perform one step of algorithm.
 */
typedef int (*plan_search_step_fn)(plan_search_t *);

struct _plan_search_block_t {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int terminate;
};
typedef struct _plan_search_block_t plan_search_block_t;

/**
 * Common base struct for all search algorithms.
 */
struct _plan_search_t {
    plan_heur_t *heur;      /*!< Heuristic function */
    int heur_del;           /*!< True if .heur should be deleted */
    plan_state_id_t initial_state;
    plan_state_pool_t *state_pool;   /*!< State pool from params.prob */
    plan_state_space_t *state_space;
    const plan_succ_gen_t *succ_gen;
    const plan_part_state_t *goal;
    plan_search_progress_t progress;

    plan_search_del_fn del_fn;
    plan_search_init_step_fn init_step_fn;
    plan_search_step_fn step_fn;
    plan_search_poststep_fn poststep_fn;
    void *poststep_data;

    plan_state_t *state;             /*!< Preallocated state */
    plan_state_id_t state_id;        /*!< ID of .state -- used for caching*/
    plan_search_stat_t stat;
    plan_search_applicable_ops_t app_ops;

    int result; /*!< Result of planSearchRun() */
    plan_state_id_t goal_state; /*!< The found state satisfying the goal */
};



/**
 * Initializas the base search struct.
 */
void _planSearchInit(plan_search_t *search,
                     const plan_search_params_t *params,
                     plan_search_del_fn del_fn,
                     plan_search_init_step_fn init_step_fn,
                     plan_search_step_fn step_fn);

/**
 * Frees allocated resources.
 */
void _planSearchFree(plan_search_t *search);

/**
 * Finds applicable operators in the specified state and store the results
 * in searchc->applicable_ops.
 */
void _planSearchFindApplicableOps(plan_search_t *search,
                                  plan_state_id_t state_id);

/**
 * Returns PLAN_SEARCH_CONT if the heuristic value was computed.
 * Any other status should lead to immediate exit from the search algorithm
 * with the same status.
 * If preferred_ops is non-NULL, the function will find preferred
 * operators and set up the given struct accordingly.
 */
int _planSearchHeuristic(plan_search_t *search,
                         plan_state_id_t state_id,
                         plan_cost_t *heur_val,
                         plan_search_applicable_ops_t *preferred_ops);

/**
 * Returns true if the given state is the goal state.
 * Also the goal state is recorded in stats and the goal state is
 * remembered.
 */
int _planSearchCheckGoal(plan_search_t *search, plan_state_space_node_t *node);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_SEARCH_H__ */
