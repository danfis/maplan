#ifndef __PLAN_HEUR_RELAX_H__
#define __PLAN_HEUR_RELAX_H__

#include <plan/state.h>
#include <plan/operator.h>


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

    int **val_id;     /*!< ID of each variable value (val_id[var][val]) */
    int val_size;     /*!< Number of all values */
    int **precond;
    int *precond_size;
    int *op_unsat;
    int *op_value;
};
typedef struct _plan_heur_relax_t plan_heur_relax_t;

/**
 * Creates a heuristic.
 * Note that it borrows the given pointer so be sure the heuristic is
 * deleted before the given partial state.
 */
plan_heur_relax_t *planHeurRelaxNew(const plan_operator_t *ops,
                                    size_t ops_size,
                                    const plan_var_t *var,
                                    size_t var_size,
                                    const plan_part_state_t *goal);


/**
 * Deletes the heuristic object.
 */
void planHeurRelaxDel(plan_heur_relax_t *h);

/**
 * Computes a heuristic value corresponding to the state.
 */
unsigned planHeurRelax(plan_heur_relax_t *h,
                       const plan_state_t *state);

#endif /* __PLAN_HEUR_RELAX_H__ */

