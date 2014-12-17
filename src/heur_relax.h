#ifndef __PLAN_HEUR_RELAX_H__
#define __PLAN_HEUR_RELAX_H__

#include "fact_op_cross_ref.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PLAN_HEUR_RELAX_TYPE_ADD 0
#define PLAN_HEUR_RELAX_TYPE_MAX 1

struct _plan_heur_relax_op_t {
    int unsat;         /*!< Number of unsatisfied preconditions */
    plan_cost_t value; /*!< Value assigned to the operator */
    plan_cost_t cost;  /*!< Cost of the operator */
    int supp;          /*!< Supporter: The fact that enabled the operator */
};
typedef struct _plan_heur_relax_op_t plan_heur_relax_op_t;

struct _plan_heur_relax_fact_t {
    plan_cost_t value; /*!< Value assigned to the fact */
    int supp;          /*!< Supporter; The op that enabled the fact */
};
typedef struct _plan_heur_relax_fact_t plan_heur_relax_fact_t;

struct _plan_heur_relax_t {
    int type;
    plan_fact_op_cross_ref_t cref; /*!< Cross referenced ops and facts */
    plan_heur_relax_op_t *op;
    plan_heur_relax_op_t *op_init; /*!< Pre-initialization of .op[] array */
    plan_heur_relax_fact_t *fact;
    plan_heur_relax_fact_t *fact_init; /*!< Pre-init of .fact[] array */

    int *plan_fact;
    int *plan_op;
    int *goal_fact; /*!< Array with flags set to 1 on facts that are the
                         goal facts. This array is used in planHeurRelax2()
                         function. */
};
typedef struct _plan_heur_relax_t plan_heur_relax_t;

/**
 * Initialize relaxation heuristic.
 */
void planHeurRelaxInit(plan_heur_relax_t *relax, int type,
                       const plan_var_t *var, int var_size,
                       const plan_part_state_t *goal,
                       const plan_op_t *op, int op_size);

/**
 * Frees allocated resources.
 */
void planHeurRelaxFree(plan_heur_relax_t *relax);

/**
 * Runs relaxation from the specified state until goal is reached.
 */
void planHeurRelax(plan_heur_relax_t *relax, const plan_state_t *state);

/**
 * Runs relaxation heuristic to a specified goal instead of the one
 * specified during creation of the object.
 * The function returns heuristic value for the specified goal.
 */
plan_cost_t planHeurRelax2(plan_heur_relax_t *relax,
                           const plan_state_t *state,
                           const plan_part_state_t *goal);

/**
 * Incrementally update h^max values considering changed costs of the
 * speficied operators.
 */
void planHeurRelaxIncMax(plan_heur_relax_t *relax,
                         const int *op, int op_size);

/**
 * Marks facts and operators in relaxed plan in .plan_fact[] and
 * .plan_op[] arrays.
 */
void planHeurRelaxMarkPlan(plan_heur_relax_t *relax);

/**
 * Same as planHeurRelaxMarkPlan() but for the specified goal.
 */
void planHeurRelaxMarkPlan2(plan_heur_relax_t *relax,
                            const plan_part_state_t *goal);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_HEUR_RELAX_H__ */
