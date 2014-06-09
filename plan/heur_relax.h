#ifndef __PLAN_HEUR_RELAX_H__
#define __PLAN_HEUR_RELAX_H__

#include <plan/problem.h>
#include <plan/heur.h>


/**
 * Relaxation Heuristics
 * ======================
 */
struct _plan_heur_relax_t {
    const plan_operator_t *ops;
    int ops_size;
    const plan_var_t *var;
    int var_size;
    const plan_part_state_t *goal;

    int **val_id;      /*!< ID of each variable value (val_id[var][val]) */
    int val_size;      /*!< Number of all values */
    int **precond;     /*!< Operator IDs indexed by precondition ID */
    int *precond_size; /*!< Size of each subarray */
    int *op_unsat;     /*!< Preinitialized counters of unsatisfied
                            preconditions per operator */
    int *op_value;     /*!< Preinitialized values of operators */

    int type;
};
typedef struct _plan_heur_relax_t plan_heur_relax_t;

/**
 * Creates an ADD version of relaxation heuristics.
 */
plan_heur_relax_t *planHeurRelaxAddNew(const plan_problem_t *prob);

/**
 * Creates an MAX version of relaxation heuristics.
 */
plan_heur_relax_t *planHeurRelaxMaxNew(const plan_problem_t *prob);

/**
 * Creates an FF version of relaxation heuristics.
 */
plan_heur_relax_t *planHeurRelaxFFNew(const plan_problem_t *prob);

/**
 * Deletes the heuristic object.
 */
void planHeurRelaxDel(plan_heur_relax_t *h);

/**
 * Computes a heuristic value corresponding to the state.
 */
plan_cost_t planHeurRelax(plan_heur_relax_t *h,
                          const plan_state_t *state);

#endif /* __PLAN_HEUR_RELAX_H__ */

