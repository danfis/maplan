#ifndef __PLAN_PROBLEM_H__
#define __PLAN_PROBLEM_H__

#include <plan/var.h>
#include <plan/state.h>
#include <plan/operator.h>
#include <plan/succgen.h>

struct _plan_problem_t {
    plan_var_t *var;               /*!< Definitions of variables */
    int var_size;                  /*!< Number of variables */
    plan_state_pool_t *state_pool; /*!< Instance of state pool/registry */
    plan_state_id_t initial_state; /*!< State ID of the initial state */
    plan_part_state_t *goal;       /*!< Partial state representing goal */
    plan_operator_t *op;           /*!< Array of operators */
    int op_size;                   /*!< Number of operators */
    plan_succ_gen_t *succ_gen;     /*!< Successor generator */
};
typedef struct _plan_problem_t plan_problem_t;

struct _plan_problem_agent_t {
    int id;                        /*!< ID of the agent */
    char *name;                    /*!< Name of the agent */
    plan_operator_t *projected_op; /*!< Array of projected operators */
    int projected_op_size;         /*!< Number of projected operators */

    plan_problem_t prob;           /*!< The rest of the problem definition */
};
typedef struct _plan_problem_agent_t plan_problem_agent_t;


/**
 * Loads planning problem from the json file.
 */
plan_problem_t *planProblemFromJson(const char *fn);

/**
 * Loads planning problem from the json file.
 */
plan_problem_t *planProblemFromFD(const char *fn);

/**
 * Load agent problem definitions from the specified file.
 * Returns 0 on success, -1 otherwise.
 */
int planProblemAgentFromFD(const char *fn,
                           plan_problem_agent_t **agents,
                           int *num_agents);
/**
 * Free all allocated resources.
 */
void planProblemDel(plan_problem_t *problem);

/**
 * Returns true if the given state is the goal.
 */
_bor_inline int planProblemCheckGoal(plan_problem_t *p,
                                     plan_state_id_t state_id);

/**
 * Dumps problem definition in text format.
 * For debugging purposes.
 */
void planProblemDump(const plan_problem_t *p, FILE *fout);

/**** INLINES ****/
_bor_inline int planProblemCheckGoal(plan_problem_t *p,
                                     plan_state_id_t state_id)
{
    return planStatePoolPartStateIsSubset(p->state_pool, p->goal, state_id);
}

#endif /* __PLAN_PROBLEM_H__ */
