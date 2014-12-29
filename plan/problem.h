#ifndef __PLAN_PROBLEM_H__
#define __PLAN_PROBLEM_H__

#include <plan/var.h>
#include <plan/state.h>
#include <plan/op.h>
#include <plan/succgen.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * If set, causal graph will be used for fine tuning problem definition.
 */
#define PLAN_PROBLEM_USE_CG 0x1

struct _plan_problem_private_val_t {
    int var;
    int val;
};
typedef struct _plan_problem_private_val_t plan_problem_private_val_t;

struct _plan_problem_t {
    plan_var_t *var;               /*!< Definitions of variables */
    int var_size;                  /*!< Number of variables */
    plan_state_pool_t *state_pool; /*!< Instance of state pool/registry */
    plan_state_id_t initial_state; /*!< State ID of the initial state */
    plan_part_state_t *goal;       /*!< Partial state representing goal */
    plan_op_t *op;                 /*!< Array of operators */
    int op_size;                   /*!< Number of operators */
    plan_succ_gen_t *succ_gen;     /*!< Successor generator */

    /** Fllowing data are available only in case of agent problem defintion: */
    char *agent_name;   /*!< Name of the corresponding agent */
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
 * Loads problem definition from protbuf format.
 * For flags see PLAN_PROBLEM_* macros.
 */
plan_problem_t *planProblemFromProto(const char *fn, int flags);

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
 * Loads agent problem definition from protbuf format.
 * For flags see PLAN_PROBLEM_* macros.
 */
plan_problem_agents_t *planProblemAgentsFromProto(const char *fn, int flags);

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
