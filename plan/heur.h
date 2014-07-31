#ifndef __PLAN_HEUR_H__
#define __PLAN_HEUR_H__

#include <plan/common.h>
#include <plan/problem.h>

/**
 * Heuristic Function API
 * =======================
 */

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
    plan_operator_t **pref_op; /*!< Input/Output list of operators */
    int pref_op_size;          /*!< Size of .pref_op[] */
    int pref_size;             /*!< Number of preferred operators */
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
 * Function that returns heuristic value (see planHeur() function below).
 */
typedef void (*plan_heur_fn)(plan_heur_t *heur, const plan_state_t *state,
                             plan_heur_res_t *res);

struct _plan_heur_t {
    plan_heur_del_fn del_fn;
    plan_heur_fn heur_fn;
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
plan_heur_t *planHeurRelaxAddNew(const plan_var_t *var, int var_size,
                                 const plan_part_state_t *goal,
                                 const plan_operator_t *op, int op_size,
                                 const plan_succ_gen_t *succ_gen);

/**
 * Creates an MAX version of relaxation heuristics.
 * If succ_gen is NULL, a new successor generator is created internally
 * from the given operators.
 */
plan_heur_t *planHeurRelaxMaxNew(const plan_var_t *var, int var_size,
                                 const plan_part_state_t *goal,
                                 const plan_operator_t *op, int op_size,
                                 const plan_succ_gen_t *succ_gen);

/**
 * Creates an FF version of relaxation heuristics.
 * If succ_gen is NULL, a new successor generator is created internally
 * from the given operators.
 */
plan_heur_t *planHeurRelaxFFNew(const plan_var_t *var, int var_size,
                                const plan_part_state_t *goal,
                                const plan_operator_t *op, int op_size,
                                const plan_succ_gen_t *succ_gen);

/**
 * Creates an LM-Cut heuristics.
 * If succ_gen is NULL, a new successor generator is created internally
 * from the given operators.
 */
plan_heur_t *planHeurLMCutNew(const plan_var_t *var, int var_size,
                              const plan_part_state_t *goal,
                              const plan_operator_t *op, int op_size,
                              const plan_succ_gen_t *succ_gen);

/**
 * Deletes heuristics object.
 */
void planHeurDel(plan_heur_t *heur);

/**
 * Compute heuristic from the specified state to the goal state.
 * See documentation to the plan_heur_res_t how the result structure is
 * filled.
 */
void planHeur(plan_heur_t *heur, const plan_state_t *state,
              plan_heur_res_t *res);




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


/**** INLINES: ****/
_bor_inline void planHeurResInit(plan_heur_res_t *res)
{
    bzero(res, sizeof(*res));
}

#endif /* __PLAN_HEUR_H__ */
