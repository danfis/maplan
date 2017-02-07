/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#ifndef __PLAN_SEARCH_H__
#define __PLAN_SEARCH_H__

#include <boruvka/timer.h>
#include <boruvka/fifo.h>

#include <plan/problem.h>
#include <plan/state_space.h>
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
 * Aborts search. This function can be called from other thread.
 */
void planSearchAbort(plan_search_t *search);

/**
 * Extracts path between initial state and the specified goal state.
 * Returns initial state ID where path was extracted from the goal state.
 */
plan_state_id_t planSearchExtractPath(const plan_search_t *search,
                                      plan_state_id_t goal_state,
                                      plan_path_t *path);

/**
 * Returns computed heuristic for the specified state.
 */
plan_cost_t planSearchStateHeur(const plan_search_t *search,
                                plan_state_id_t state_id);

/**
 * Returns cost of the node on top of the open-list.
 */
_bor_inline plan_cost_t planSearchTopNodeCost(const plan_search_t *search);

/**
 * (Re-)Inserts node to the open-list
 */
void planSearchInsertNode(plan_search_t *search,
                          plan_state_space_node_t *node);

/**
 * Callback that is called after each step.
 */
typedef int (*plan_search_poststep_fn)(plan_search_t *search, int res,
                                       void *userdata);

/**
 * Sets post-step callback, i.e., function that is called after each step
 * is performed.
 */
void planSearchSetPostStep(plan_search_t *search,
                           plan_search_poststep_fn fn, void *userdata);

/**
 * Callback function that is called on each expanded state.
 */
typedef void (*plan_search_expanded_node_fn)(plan_search_t *search,
                                             plan_state_space_node_t *node,
                                             void *userdata);

/**
 * Callback that is called for each expanded node (must be implemented in
 * particular search algorithm).
 */
void planSearchSetExpandedNode(plan_search_t *search,
                               plan_search_expanded_node_fn cb, void *ud);


/**
 * Callback for planSearchSetReachedGoal()
 */
typedef void (*plan_search_reached_goal_fn)(plan_search_t *search,
                                            plan_state_space_node_t *node,
                                            void *userdata);

/**
 * Sets callback that is called whenever the state satisficing goals is
 * reached.
 */
void planSearchSetReachedGoal(plan_search_t *search,
                              plan_search_reached_goal_fn cb,
                              void *userdata);


/**
 * Callback for planSearchSetMAHeur()
 */
typedef void (*plan_search_ma_heur_fn)(plan_search_t *search,
                                       plan_heur_t *heur,
                                       plan_state_id_t state_id,
                                       plan_heur_res_t *res,
                                       void *userdata);

/**
 * Sets callback that is called whenever a multi-agent heuristic should be
 * computed.
 */
void planSearchSetMAHeur(plan_search_t *search,
                         plan_search_ma_heur_fn cb, void *userdata);


/**
 * Returns a state corresponding to the provided state ID.
 * The return pointer references an internal storage of search object.
 */
const plan_state_t *planSearchLoadState(plan_search_t *search,
                                        plan_state_id_t state_id);

/**
 * Returns state space node.
 */
plan_state_space_node_t *planSearchLoadNode(plan_search_t *search,
                                            plan_state_id_t state_id);

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

/**
 * (Re-)Inserts node into open list.
 * This function should not re-compute heuristic or do any other operation
 * other than (re-)inserting into open-list.
 */
typedef void (*plan_search_insert_node_fn)(plan_search_t *,
                                           plan_state_space_node_t *node);

/**
 * Returns cost of the node on top of the open-list.
 */
typedef plan_cost_t (*plan_search_top_node_cost_fn)(const plan_search_t *s);

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
    int abort;
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
    plan_search_insert_node_fn insert_node_fn;
    plan_search_top_node_cost_fn top_node_cost_fn;
    plan_search_poststep_fn poststep_fn;
    void *poststep_data;
    plan_search_expanded_node_fn expanded_node_fn;
    void *expanded_node_data;
    plan_search_reached_goal_fn reached_goal_fn;
    void *reached_goal_data;
    plan_search_ma_heur_fn ma_heur_fn;
    void *ma_heur_data;

    plan_state_t *state;             /*!< Preallocated state */
    plan_state_id_t state_id;        /*!< ID of .state -- used for caching*/
    plan_search_stat_t stat;
    plan_search_applicable_ops_t app_ops;

    plan_state_id_t goal_state; /*!< The found state satisfying the goal */
};



/**
 * Initializas the base search struct.
 */
void _planSearchInit(plan_search_t *search,
                     const plan_search_params_t *params,
                     plan_search_del_fn del_fn,
                     plan_search_init_step_fn init_step_fn,
                     plan_search_step_fn step_fn,
                     plan_search_insert_node_fn insert_node_fn,
                     plan_search_top_node_cost_fn top_node_cost_fn);

/**
 * Frees allocated resources.
 */
void _planSearchFree(plan_search_t *search);

/**
 * Finds applicable operators in the specified state and store the results
 * in searchc->applicable_ops.
 * Returns 0 if the result was already cached and no search was performed,
 * and 1 if a new search was performed.
 */
int _planSearchFindApplicableOps(plan_search_t *search,
                                 plan_state_id_t state_id);

/**
 * Returns PLAN_SEARCH_CONT if the heuristic value was computed.
 * Any other status should lead to immediate exit from the search algorithm
 * with the same status.
 * If preferred_ops is non-NULL, the function will find preferred
 * operators and set up the given struct accordingly.
 */
int _planSearchHeur(plan_search_t *search,
                    plan_state_space_node_t *node,
                    plan_cost_t *heur_val,
                    plan_search_applicable_ops_t *preferred_ops);

/**
 * Returns true if the given state is the goal state.
 * Also the goal state is recorded in stats and the goal state is
 * remembered.
 */
int _planSearchCheckGoal(plan_search_t *search, plan_state_space_node_t *node);

/**
 * Apply operator to a specified state. A new state is created a heuristic
 * value is computed and the resulting state ID is returned.
 * Mainly for a testing.
 */
plan_state_id_t planSearchApplyOp(plan_search_t *search,
                                  plan_state_id_t state_id,
                                  plan_op_t *op);


/**** INLINES: ****/
_bor_inline void _planSearchExpandedNode(plan_search_t *search,
                                         plan_state_space_node_t *node)
{
    if (search->expanded_node_fn){
        search->expanded_node_fn(search, node, search->expanded_node_data);
    }
}

_bor_inline plan_cost_t planSearchTopNodeCost(const plan_search_t *search)
{
    if (search->top_node_cost_fn)
        return search->top_node_cost_fn(search);
    return PLAN_COST_MAX;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_SEARCH_H__ */
