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

#ifndef __PLAN_PROBLEM_H__
#define __PLAN_PROBLEM_H__

#include <plan/var.h>
#include <plan/state.h>
#include <plan/op.h>
#include <plan/succ_gen.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * If set, causal graph will be used for fine tuning problem definition.
 */
#define PLAN_PROBLEM_USE_CG 0x1u

/**
 * Turns on removing duplicate operators.
 */
#define PLAN_PROBLEM_PRUNE_DUPLICATES 0x2u

/**
 * Set cost of all operators to one.
 */
#define PLAN_PROBLEM_OP_UNIT_COST 0x4u

/**
 * Enable state privacy in ma mode.
 */
#define PLAN_PROBLEM_MA_STATE_PRIVACY 0x8u

/**
 * Prepare problem on cluster of a specified number of agents.
 */
#define PLAN_PROBLEM_NUM_AGENTS(num) \
    ((unsigned)(((unsigned char)(num)) << 4))

struct _plan_problem_private_val_t {
    int var;
    int val;
};
typedef struct _plan_problem_private_val_t plan_problem_private_val_t;

struct _plan_problem_t {
    plan_var_t *var;               /*!< Definitions of variables */
    int var_size;                  /*!< Number of variables */
    int ma_privacy_var;            /*!< ID of the variable used for state
                                        privacy in ma mode, or -1 if the
                                        feature is not enabled */
    plan_state_pool_t *state_pool; /*!< Instance of state pool/registry */
    plan_state_id_t initial_state; /*!< State ID of the initial state */
    plan_part_state_t *goal;       /*!< Partial state representing goal */
    plan_op_t *op;                 /*!< Array of operators */
    int op_size;                   /*!< Number of operators */
    plan_succ_gen_t *succ_gen;     /*!< Successor generator */
    int duplicate_ops_removed;     /*!< Number of duplicate operators that
                                        were removed */

    /** Fllowing data are available only in case of agent problem defintion: */
    char *agent_name;   /*!< Name of the corresponding agent */
    int agent_id;       /*!< ID of the agent enumerated from 0 */
    int num_agents;     /*!< Number of agents in cluster */
    plan_op_t *proj_op; /*!< Projected operators */
    int proj_op_size;   /*!< Number of projected operators */
    plan_problem_private_val_t *private_val; /*!< List of private values */
    int private_val_size;
};
typedef struct _plan_problem_t plan_problem_t;

struct _plan_problem_agents_t {
    plan_problem_t glob;   /*!< Global definition w/o agents */
    plan_problem_t *agent; /*!< Definition for each agent */
    int agent_size;        /*!< Number of agents */
};
typedef struct _plan_problem_agents_t plan_problem_agents_t;

/**
 * Creates a problem structure from pddl domain and problem files.
 */
plan_problem_t *planProblemFromPDDL(const char *domain_pddl,
                                    const char *problem_pddl,
                                    unsigned flags);
/**
 * Loads planning problem from the fast-downward file.
 */
plan_problem_t *planProblemFromFD(const char *fn);

/**
 * Loads problem definition from protbuf format.
 * For flags see PLAN_PROBLEM_* macros.
 */
plan_problem_t *planProblemFromProto(const char *fn, unsigned flags);

/**
 * Creates an exact copy of the problem object.
 */
plan_problem_t *planProblemClone(const plan_problem_t *p);

/**
 * Free all allocated resources.
 */
void planProblemDel(plan_problem_t *problem);

/**
 * Initializes empty problem definition.
 */
void planProblemInit(plan_problem_t *prob);

/**
 * Free allocated resources "in place"
 */
void planProblemFree(plan_problem_t *prob);

/**
 * Copies problem object from src to dst.
 */
void planProblemCopy(plan_problem_t *dst, const plan_problem_t *src);

/**
 * Loads unfactore multi-agent problem from PDDL files.
 */
plan_problem_agents_t *planProblemUnfactorFromPDDL(const char *domain_pddl,
                                                   const char *problem_pddl,
                                                   unsigned flags);

/**
 * Loads agent problem definition from protbuf format.
 * For flags see PLAN_PROBLEM_* macros.
 */
plan_problem_agents_t *planProblemAgentsFromProto(const char *fn, unsigned flags);

/**
 * Creates an exact copy of the problem definition object.
 */
plan_problem_agents_t *planProblemAgentsClone(const plan_problem_agents_t *a);

/**
 * Deletes agent defitions.
 */
void planProblemAgentsDel(plan_problem_agents_t *agents);

/**
 * Prints problem definition as graph in DOT format.
 */
void planProblemAgentsDotGraph(const plan_problem_agents_t *agents,
                               FILE *fout);

/**
 * Returns true if the given state is the goal.
 */
_bor_inline int planProblemCheckGoal(plan_problem_t *p,
                                     plan_state_id_t state_id);

/**
 * Pack part-states and operators.
 */
void planProblemPack(plan_problem_t *p);

/**
 * Same as planProblemPack() but for plan_problem_agents_t
 */
void planProblemAgentsPack(plan_problem_agents_t *p);

/**
 * Creates an array of operators that are private projections of the given
 * operators.
 */
void planProblemCreatePrivateProjOps(const plan_op_t *op, int op_size,
                                     const plan_var_t *var, int var_size,
                                     plan_op_t **op_out, int *op_out_size);

/**
 * Destroys an array of operators.
 */
void planProblemDestroyOps(plan_op_t *op, int op_size);

/**
 * Create a shallow copy of a projection of a problem.
 */
void planProblemProj(plan_problem_t *proj, const plan_problem_t *p);

/**
 * Shallow copy containing global operators.
 */
void planProblemGlobOps(plan_problem_t *proj,
                        const plan_problem_t *base,
                        const plan_problem_t *glob);

/**
 * Frees global memory allocated by google protobuffers.
 * This is just wrapper for
 *      google::protobuf::ShutdownProtobufLibrary().
 */
void planShutdownProtobuf(void);

/**** INLINES ****/
_bor_inline int planProblemCheckGoal(plan_problem_t *p,
                                     plan_state_id_t state_id)
{
    return planStatePoolPartStateIsSubset(p->state_pool, p->goal, state_id);
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PROBLEM_H__ */
