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

#ifndef __PLAN_HEUR_H__
#define __PLAN_HEUR_H__

#include <plan/common.h>
#include <plan/problem.h>
#include <plan/ma_comm.h>
#include <plan/ma_state.h>
#include <plan/landmark.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Forward declaration */
struct _plan_search_t;

/**
 * Heuristic Function API
 * =======================
 */

/**
 * Set cost of all operators to one.
 */
#define PLAN_HEUR_OP_UNIT_COST 0x1u

/**
 * Add one to all operators' costs.
 */
#define PLAN_HEUR_OP_COST_PLUS_ONE 0x2u

/**
 * Use integer linear programming instead of LP for solving flow heuristic.
 */
#define PLAN_HEUR_FLOW_ILP 0x10u

/**
 * Use landmarks from lm-cut heuristic for constraints in the flow heuristic.
 */
#define PLAN_HEUR_FLOW_LANDMARKS_LM_CUT 0x20u

/**
 * Use fa-mutexes to tighten bounds.
 */
#define PLAN_HEUR_FLOW_FA_MUTEX 0x40u

/**
 * Sets the number of parallel threads that will be invoked by any CPLEX
 * parallel optimizer.
 * By default one thread is used.
 * Set num threads to -1 to switch to auto mode.
 */
#define PLAN_HEUR_FLOW_CPLEX_NUM_THREADS(num) \
    ((((unsigned)(num)) & 0x3fu) << 8u)

/**
 * Turns on all-syntactic-states version of potential heuristic.
 */
#define PLAN_HEUR_POT_ALL_SYNTACTIC_STATES 0x40u

/**
 * Turns off encryption of matrices for ma-pot heuristic.
 */
#define PLAN_HEUR_POT_ENCRYPTION_OFF 0x80u

/**
 * Print time of the computation of the potentials in the initial step.
 */
#define PLAN_HEUR_POT_PRINT_INIT_TIME 0x100u

#define PLAN_HEUR_H2 0x1000u

/** Forward declaration */
typedef struct _plan_heur_t plan_heur_t;

/**
 * Structure for results.
 */
struct _plan_heur_res_t {
    plan_cost_t heur; /*!< Computed heuristic value */

    /** Preferred operators:
     *  If .pref_op is set to non-NULL, the preferred operators will be
     *  also determined (if the algorithm knows how). The .pref_op is used
     *  as input/output array of operators and the caller should set it up
     *  together with .pref_op_size. The functin planHeur() then reorders
     *  operators in .pref_op[] so that the first ones are the preferred
     *  ones. The number of preferred operators is then stored in
     *  .pref_size member, i.e., first .pref_size operators in .pref_op[]
     *  array are the preferred opetors.
     *  Note 1: .pref_op[] should already contain applicable operators.
     *  Note 2: The pointers in .pref_op[] should be from the same array of
     *          operators as were used in building the heuristic object.
     */
    plan_op_t **pref_op; /*!< Input/Output list of operators */
    int pref_op_size;    /*!< Size of .pref_op[] */
    int pref_size;       /*!< Number of preferred operators */

    int save_landmarks; /*!< Set to true if landmarks should be returned.
                             Don't forget to use planLandmarkSetFree()
                             to free all memory allocated within .landmarks
                             member. */
    plan_landmark_set_t landmarks; /*!< Struct containing landmarks */

};
typedef struct _plan_heur_res_t plan_heur_res_t;

/**
 * Initializes plan_heur_res_t structure.
 */
_bor_inline void planHeurResInit(plan_heur_res_t *res);

/**
 * Destructor of the heuristic object.
 */
typedef void (*plan_heur_del_fn)(plan_heur_t *heur);

/**
 * Function that returns heuristic value (see planHeurState() function below).
 * This can be set to NULL if plan_heur_node_fn is set. This callback is
 * here primarly for a convenience in case the heuristic is based solely on
 * the state itself (probably most heuristics are).
 */
typedef void (*plan_heur_state_fn)(plan_heur_t *heur, const plan_state_t *state,
                                   plan_heur_res_t *res);

/**
 * Function that returns heuristic estimate based on ID of state node
 * (see planHeurNode() function below).
 * This method can be set to NULL if plan_search_state_fn is filled. In
 * that case, plan_heur_state_fn is automatically called instead.
 */
typedef void (*plan_heur_node_fn)(plan_heur_t *heur,
                                  plan_state_id_t state_id,
                                  struct _plan_search_t *search,
                                  plan_heur_res_t *res);

/**
 * Multi-agent version of plan_heur_state_fn
 */
typedef int (*plan_heur_ma_state_fn)(plan_heur_t *heur,
                                     plan_ma_comm_t *comm,
                                     const plan_state_t *state,
                                     plan_heur_res_t *res);

/**
 * Multi-agent version of plan_heur_node_fn
 */
typedef int (*plan_heur_ma_node_fn)(plan_heur_t *heur,
                                    plan_ma_comm_t *comm,
                                    plan_state_id_t state_id,
                                    struct _plan_search_t *search,
                                    plan_heur_res_t *res);

/**
 * Update function for multi-agent version of heuristic
 */
typedef int (*plan_heur_ma_update_fn)(plan_heur_t *heur,
                                      plan_ma_comm_t *comm,
                                      const plan_ma_msg_t *msg,
                                      plan_heur_res_t *res);

/**
 * Method for processing request from remote peer.
 */
typedef void (*plan_heur_ma_request_fn)(plan_heur_t *heur,
                                        plan_ma_comm_t *comm,
                                        const plan_ma_msg_t *msg);

struct _plan_heur_t {
    plan_heur_del_fn del_fn;
    plan_heur_state_fn heur_state_fn;
    plan_heur_node_fn heur_node_fn;
    plan_heur_ma_state_fn heur_ma_state_fn;
    plan_heur_ma_node_fn heur_ma_node_fn;
    plan_heur_ma_update_fn heur_ma_update_fn;
    plan_heur_ma_request_fn heur_ma_request_fn;

    int ma; /*!< Set to true if planHeurMA*() functions should be used
                 instead of planHeur() */
    int ma_agent_size;
    int ma_agent_id;
    plan_ma_state_t *ma_state;
};

/**
 * Creates a Goal Count Heuristic.
 * Note that it borrows the given pointer so be sure the heuristic is
 * deleted before the given partial state.
 */
plan_heur_t *planHeurGoalCountNew(const plan_part_state_t *goal);

/**
 * Creates an ADD version of relaxation heuristics.
 * If succ_gen is NULL, a new successor generator is created internally
 * from the given operators.
 */
plan_heur_t *planHeurRelaxAddNew(const plan_problem_t *p, unsigned flags);
plan_heur_t *planHeurAddNew(const plan_problem_t *prob, unsigned flags);

/**
 * Creates an MAX version of relaxation heuristics.
 * If succ_gen is NULL, a new successor generator is created internally
 * from the given operators.
 */
plan_heur_t *planHeurRelaxMaxNew(const plan_problem_t *p, unsigned flags);
plan_heur_t *planHeurMaxNew(const plan_problem_t *p, unsigned flags);

/**
 * Creates an FF version of relaxation heuristics.
 * If succ_gen is NULL, a new successor generator is created internally
 * from the given operators.
 */
plan_heur_t *planHeurRelaxFFNew(const plan_problem_t *p, unsigned flags);

/**
 * Creates an LM-Cut heuristics.
 */
plan_heur_t *planHeurRelaxLMCutNew(const plan_problem_t *p, unsigned flags);
plan_heur_t *planHeurLMCutNew(const plan_problem_t *p, unsigned flags);

/**
 * Incremental LM-Cut, the local version.
 */
plan_heur_t *planHeurRelaxLMCutIncLocalNew(const plan_problem_t *p,
                                           unsigned flags);
plan_heur_t *planHeurLMCutIncLocalNew(const plan_problem_t *p, unsigned f);

/**
 * Incremental LM-Cut, the version with cached landmarks.
 * Use PLAN_LANDMARK_CACHE_* flags if you want to change the default
 * behaviour of the landmark cache.
 */
plan_heur_t *planHeurRelaxLMCutIncCacheNew(const plan_problem_t *p,
                                           unsigned flags,
                                           unsigned cache_flags);
plan_heur_t *planHeurLMCutIncCacheNew(const plan_problem_t *p,
                                      unsigned flags, unsigned cache_flags);

/**
 * h^2 heuristic
 */
plan_heur_t *planHeurH2MaxNew(const plan_problem_t *p, unsigned flags);
plan_heur_t *planHeurMax2New(const plan_problem_t *p, unsigned flags);

/**
 * h^2 LM-Cut heuristic
 */
plan_heur_t *planHeurH2LMCutNew(const plan_problem_t *p, unsigned flags);
plan_heur_t *planHeurLMCut2New(const plan_problem_t *p, unsigned flags);


/**
 * Domain transition graph based heuristic.
 */
plan_heur_t *planHeurDTGNew(const plan_problem_t *p, unsigned flags);

/**
 * Flow based heuristics.
 * For flags see PLAN_HEUR_FLOW_* macros above.
 */
plan_heur_t *planHeurFlowNew(const plan_problem_t *p, unsigned flags);

/**
 * Potential based heuristics.
 */
plan_heur_t *planHeurPotentialNew(const plan_problem_t *p,
                                  const plan_state_t *init_state,
                                  unsigned flags);

/**
 * Creates an multi-agent version of max heuristic.
 */
plan_heur_t *planHeurMARelaxMaxNew(const plan_problem_t *agent_def,
                                   unsigned flags);

/**
 * Creates an multi-agent version of FF heuristic.
 */
plan_heur_t *planHeurMARelaxFFNew(const plan_problem_t *agent_def);

/**
 * Multi-agent lm-cut heuristic.
 */
plan_heur_t *planHeurMALMCutNew(const plan_problem_t *agent_def);

/**
 * Multi-agent DTG heuristic.
 */
plan_heur_t *planHeurMADTGNew(const plan_problem_t *agent_def);

/**
 * Multi-agent potential heuristic.
 */
plan_heur_t *planHeurMAPotNew(const plan_problem_t *p, unsigned flags);

/**
 * Multi-agent potential heuristic on projections.
 */
plan_heur_t *planHeurMAPotProjNew(const plan_problem_t *p, unsigned flags);

/**
 * Deletes heuristics object.
 */
void planHeurDel(plan_heur_t *heur);

/**
 * Compute heuristic estimate for from the specified state node.
 * See documentation to the plan_heur_res_t how the result structure is
 * filled.
 */
void planHeurNode(plan_heur_t *heur, plan_state_id_t state_id,
                  struct _plan_search_t *search, plan_heur_res_t *res);

/**
 * Similar to planHeurNode() but only for the cases when the heuristic is
 * based solely on the state itself.
 * This function is here primarly for testing purposes, from within search
 * procedures planHeurNode() is called instead.
 */
void planHeurState(plan_heur_t *heur, const plan_state_t *state,
                   plan_heur_res_t *res);

/**
 * Initialization of heuristic in ma mode.
 * This is called from within ma-search object before first call of
 * planHeurMA().
 */
void planHeurMAInit(plan_heur_t *heur, int agent_size, int agent_id,
                    plan_ma_state_t *ma_state);

/**
 * Multi-agent version of planHeurNode(), this function can send some messages
 * to other peers. Returns 0 if heuristic value was found or -1 if
 * planHeurMAUpdate() should be consecutively called on all heur-response
 * messages.
 */
int planHeurMANode(plan_heur_t *heur,
                   plan_ma_comm_t *comm,
                   plan_state_id_t state_id,
                   struct _plan_search_t *search,
                   plan_heur_res_t *res);

/**
 * Similar to planHeurMANode() but only for the cases when the heuristic is
 * based solely on the state itself.
 * This function is here primarly for testing purposes, from within
 * search procedures planHeurMANode() is called.
 */
int planHeurMAState(plan_heur_t *heur,
                    plan_ma_comm_t *comm,
                    const plan_state_t *state,
                    plan_heur_res_t *res);

/**
 * If planHeurMA() returned non-zero value (reference ID), all receiving
 * messages with the same ID (as the returned value) should be passed into
 * this function until 0 is returned in which case the heuristic value was
 * computed.
 */
int planHeurMAUpdate(plan_heur_t *heur,
                     plan_ma_comm_t *comm,
                     const plan_ma_msg_t *msg,
                     plan_heur_res_t *res);

/**
 * Function that process multi-agent heur-request message.
 */
void planHeurMARequest(plan_heur_t *heur,
                       plan_ma_comm_t *comm,
                       const plan_ma_msg_t *msg);

/**
 * Initializes heuristics.
 * For internal use.
 */
void _planHeurInit(plan_heur_t *heur,
                   plan_heur_del_fn del_fn,
                   plan_heur_state_fn heur_state_fn,
                   plan_heur_node_fn heur_node_fn);

/**
 * Initializes multi-agent part of the heuristics.
 * This function must be called _after_ _planHeurInit().
 * For internal use.
 */
void _planHeurMAInit(plan_heur_t *heur,
                     plan_heur_ma_state_fn heur_ma_state_fn,
                     plan_heur_ma_node_fn heur_ma_node_fn,
                     plan_heur_ma_update_fn heur_ma_update_fn,
                     plan_heur_ma_request_fn heur_ma_request_fn);

/**
 * Frees allocated resources.
 * For internal use.
 */
void _planHeurFree(plan_heur_t *heur);


/**** INLINES: ****/
_bor_inline void planHeurResInit(plan_heur_res_t *res)
{
    bzero(res, sizeof(*res));
}


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_HEUR_H__ */
