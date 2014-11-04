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


/**
 * Callback called each time a node is closed.
 */
typedef void (*plan_search_run_node_closed)(plan_state_space_node_t *node,
                                            void *data);

/**
 * Common parameters for all search algorithms.
 */
struct _plan_search_params_t {
    plan_heur_t *heur; /*!< Heuristic function that ought to be used */
    int heur_del;      /*!< True if .heur should be deleted in
                            planSearchDel() */

    plan_search_progress_fn progress_fn; /*!< Callback for monitoring */
    long progress_freq;                  /*!< Frequence of calling
                                              .progress_fn as number of steps. */
    void *progress_data;

    plan_problem_t *prob; /*!< Problem definition */
    int ma_ack_solution; /*!< Set to true if you want ack'ed solutions in
                              multi-agent mode. */
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

/**
 * Runs search in multi-agent mode.
 */
int planSearchMARun(plan_search_t *search,
                    plan_search_ma_params_t *ma_params,
                    plan_path_t *path);

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
typedef int (*plan_search_init_fn)(plan_search_t *);

/**
 * Perform one step of algorithm.
 */
typedef int (*plan_search_step_fn)(plan_search_t *);

/**
 * Inject the given state into open-list and performs another needed
 * operations with the state.
 * Returns 0 on success.
 */
typedef int (*plan_search_inject_state_fn)(plan_search_t *search,
                                           plan_state_id_t state_id,
                                           plan_cost_t cost,
                                           plan_cost_t heuristic);

/**
 * Returns the lowest cost which is currently available among all open
 * nodes.
 */
typedef plan_cost_t (*plan_search_lowest_cost_fn)(plan_search_t *search);

struct _plan_search_ma_solution_verify_t {
    bor_fifo_t waitlist; /*!< Solution messages waiting for verification */
    int in_progress;     /*!< True if we are in the middle of erification
                              of a solution */
    plan_cost_t cost;    /*!< Cost of the solution */
    int state_id;        /*!< State ID of the currently verified solution */
    int token;           /*!< Token associated with the solution */
    int *mark;           /*!< Array of bool flags signaling the from the
                              SOLUTION_MARK message was received from the
                              corresponding agent. */
    int mark_size;
    int mark_remaining;  /*!< Number of peers for which SOLUTION_MARK was
                              not received yet */
    int ack_remaining;   /*!< Number of acks remaining */

    int invalid;         /*!< True if the verification failed */
    int reinsert;        /*!< True if the solution should be re-inserted */
};
typedef struct _plan_search_ma_solution_verify_t
    plan_search_ma_solution_verify_t;

struct _plan_search_ma_solution_ack_t {
    plan_ma_msg_t *solution_msg;
    plan_cost_t cost_lower_bound;
    int token;
    int *mark;
    int mark_remaining;
};
typedef struct _plan_search_ma_solution_ack_t plan_search_ma_solution_ack_t;

/**
 * Common base struct for all search algorithms.
 */
struct _plan_search_t {
    plan_heur_t *heur;      /*!< Heuristic function */
    int heur_del;           /*!< True if .heur should be deleted */

    plan_search_del_fn del_fn;
    plan_search_init_fn init_fn;
    plan_search_step_fn step_fn;
    plan_search_inject_state_fn inject_state_fn;
    plan_search_lowest_cost_fn lowest_cost_fn;

    plan_search_params_t params;
    plan_search_stat_t stat;

    plan_state_pool_t *state_pool;   /*!< State pool from params.prob */
    plan_state_space_t *state_space;
    plan_state_t *state;             /*!< Preallocated state */
    plan_state_id_t state_id;        /*!< ID of .state -- used for caching*/
    plan_state_id_t goal_state;      /*!< The found state satisfying the goal */
    plan_cost_t best_goal_cost;
    const plan_succ_gen_t *succ_gen;

    plan_search_applicable_ops_t app_ops;

    int ma;                  /*!< True if running in multi-agent mode */
    plan_ma_comm_t *ma_comm; /*!< Communication queue for MA search */
    int ma_comm_node_id;     /*!< ID of this node */
    int ma_comm_node_size;   /*!< Number of nodes in cluster */
    int ma_pub_state_reg;    /*!< ID of the registry that associates
                                  received public state with state-id. */
    int ma_terminated;       /*!< True if already terminated */
    plan_path_t *ma_path;    /*!< Output path for multi-agent mode */
    int ma_ack_solution;     /*!< True if solution should be ack'ed */

    /** Struct for verification of solutions */
    plan_search_ma_solution_verify_t ma_solution_verify;
    /** Solutions waiting for ACK */
    plan_search_ma_solution_ack_t *ma_solution_ack;
    int ma_solution_ack_size;
};



/**
 * Initializas the base search struct.
 */
void _planSearchInit(plan_search_t *search,
                     const plan_search_params_t *params,
                     plan_search_del_fn del_fn,
                     plan_search_init_fn init_fn,
                     plan_search_step_fn step_fn,
                     plan_search_inject_state_fn inject_state_fn,
                     plan_search_lowest_cost_fn lowest_cost_fn);

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
 * Adds state's successors to the lazy list with the specified cost.
 */
void _planSearchAddLazySuccessors(plan_search_t *search,
                                  plan_state_id_t state_id,
                                  plan_op_t **op, int op_size,
                                  plan_cost_t cost,
                                  plan_list_lazy_t *list);

/**
 * Generalization for lazy search algorithms.
 * Injects given state into open-list if the node wasn't discovered yet.
 */
int _planSearchLazyInjectState(plan_search_t *search,
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
                        plan_op_t *operator,
                        plan_state_id_t parent_state,
                        plan_state_id_t *new_state_id,
                        plan_state_space_node_t **new_node);

/**
 * Open and close the state in one step.
 */
plan_state_space_node_t *_planSearchNodeOpenClose(plan_search_t *search,
                                                  plan_state_id_t state,
                                                  plan_state_id_t parent_state,
                                                  plan_op_t *parent_op,
                                                  plan_cost_t cost,
                                                  plan_cost_t heur);

/**
 * Checks whether the node is public and if so, sends it to other agents.
 */
void _planSearchMASendState(plan_search_t *search,
                            plan_state_space_node_t *node);

/**
 * Updates part of statistics.
 */
void _planUpdateStat(plan_search_stat_t *stat,
                     long steps, bor_timer_t *timer);

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
