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
 * Structure used for obtaining preferred operators.
 * See planHeur() doc for explanation how to use it.
 */
struct _plan_heur_preferred_ops_t {
    plan_operator_t **op; /*!< Input/Output list of operators. */
    int op_size;          /*!< Number of operators in .op[] */
    int preferred_size;   /*!< Number of preferred operators, i.e., after
                               call of planHeur() first .preferred_size
                               operators in .op[] are the preferred one. */
};
typedef struct _plan_heur_preferred_ops_t plan_heur_preferred_ops_t;

/**
 * Destructor of the heuristic object.
 */
typedef void (*plan_heur_del_fn)(plan_heur_t *heur);

/**
 * Function that returns heuristic value (see planHeur() function below).
 */
typedef plan_cost_t (*plan_heur_fn)(plan_heur_t *heur, const plan_state_t *state,
                                    plan_heur_preferred_ops_t *preferred_ops);

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
 * Deletes heuristics object.
 */
void planHeurDel(plan_heur_t *heur);

/**
 * Returns a heuristic value.
 * If the {preferred_ops} argument is non-NULL the function will also
 * determine preferred operators (if it can).
 * The parameter {preferred_ops} is used as input/output parameter the
 * following way. The caller should set up .op and .op_size members of the
 * struct (for example using data returned from planSuccGenFind()). The
 * function then reorders operators in .op[] array and set .preferred_size
 * member in a way that the first .preffered_size operators are the
 * preferred operators.
 * Note 1: .op[] should already contain applicable * operators.
 * Note 2: The pointers in .op[] should be from the same array of operators
 * as were used in building heuristic object.
 */
plan_cost_t planHeur(plan_heur_t *heur, const plan_state_t *state,
                     plan_heur_preferred_ops_t *preferred_ops);




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
