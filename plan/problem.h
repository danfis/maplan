#ifndef __PLAN_PROBLEM_H__
#define __PLAN_PROBLEM_H__

#include <plan/var.h>
#include <plan/state.h>
#include <plan/operator.h>

struct _plan_problem_t {
    plan_var_t *var;               /*!< Definitions of variables */
    int var_size;                  /*!< Number of variables */
    plan_state_pool_t *state_pool; /*!< Instance of state pool/registry */
    plan_state_id_t initial_state; /*!< State ID of the initial state */
    plan_part_state_t *goal;       /*!< Partial state representing goal */
    plan_operator_t *op;           /*!< Array of operators */
    int op_size;                   /*!< Number of operators */
};
typedef struct _plan_problem_t plan_problem_t;


/**
 * Loads planning problem from the json file.
 */
plan_problem_t *planProblemFromJson(const char *fn);

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
