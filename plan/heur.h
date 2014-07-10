#ifndef __PLAN_HEUR_H__
#define __PLAN_HEUR_H__

#include <plan/common.h>
#include <plan/problem.h>

/**
 * Heuristic Function API
 * =======================
 */

typedef void (*plan_heur_del_fn)(void *heur);
typedef plan_cost_t (*plan_heur_fn)(void *heur, const plan_state_t *state);

struct _plan_heur_t {
    plan_heur_del_fn del_fn;
    plan_heur_fn heur_fn;
};
typedef struct _plan_heur_t plan_heur_t;

/**
 * Creates a Goal Count Heuristic.
 * Note that it borrows the given pointer so be sure the heuristic is
 * deleted before the given partial state.
 */
plan_heur_t *planHeurGoalCountNew(const plan_part_state_t *goal);

/**
 * Creates an ADD version of relaxation heuristics.
 */
plan_heur_t *planHeurRelaxAddNew(const plan_var_t *var, int var_size,
                                 const plan_part_state_t *goal,
                                 const plan_operator_t *op, int op_size,
                                 const plan_succ_gen_t *succ_gen);

/**
 * Creates an MAX version of relaxation heuristics.
 */
plan_heur_t *planHeurRelaxMaxNew(const plan_var_t *var, int var_size,
                                 const plan_part_state_t *goal,
                                 const plan_operator_t *op, int op_size,
                                 const plan_succ_gen_t *succ_gen);

/**
 * Creates an FF version of relaxation heuristics.
 */
plan_heur_t *planHeurRelaxFFNew(const plan_var_t *var, int var_size,
                                const plan_part_state_t *goal,
                                const plan_operator_t *op, int op_size,
                                const plan_succ_gen_t *succ_gen);

/**
 * Deletes heuristics object.
 */
void planHeurDel(plan_heur_t *heur);

/**
 * Returns a heuristic value.
 */
plan_cost_t planHeur(plan_heur_t *heur, const plan_state_t *state);




/**
 * Initializes heuristics.
 * For internal use.
 */
void planHeurInit(plan_heur_t *heur,
                  plan_heur_del_fn del_fn,
                  plan_heur_fn heur_fn);

/**
 * Frees allocated resources.
 * For internal use.
 */
void planHeurFree(plan_heur_t *heur);

#endif /* __PLAN_HEUR_H__ */
