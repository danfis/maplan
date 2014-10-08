#ifndef ___PLAN_PROBLEM_H__
#define ___PLAN_PROBLEM_H__

#include "plan/problem.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Structure for storing array fact IDs.
 */
struct _plan_problem_factarr_t {
    int *fact;
    int size;
};
typedef struct _plan_problem_factarr_t plan_problem_factarr_t;

/**
 * Structure for storing array operator IDs.
 */
struct _plan_problem_oparr_t {
    int *op;
    int size;
};
typedef struct _plan_problem_oparr_t plan_problem_oparr_t;

/**
 * Translator from variable-value pair to a fact ID.
 */
struct _plan_problem_fact_id_t {
    int **fact_id; /*!< Fact IDs indexed by var-ID and its value */
    int fact_size; /*!< Number of all facts */
    int var_size;  /*!< Number of variables */
};
typedef struct _plan_problem_fact_id_t plan_problem_fact_id_t;


/**
 * Initializes translator from var-val pair to fact ID.
 */
void planProblemFactIdInit(plan_problem_fact_id_t *factid,
                           const plan_var_t *var, int var_size);

/**
 * Frees resources associated with fact-id structure.
 */
void planProblemFactIdFree(plan_problem_fact_id_t *factid);

/**
 * Translates var-val pair to fact ID.
 */
_bor_inline int planProblemFactId(const plan_problem_fact_id_t *factid,
                                  plan_var_id_t var, plan_val_t val);

/**** INLINES: ****/
_bor_inline int planProblemFactId(const plan_problem_fact_id_t *factid,
                                  plan_var_id_t var, plan_val_t val)
{
    return factid->fact_id[var][val];
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ___PLAN_PROBLEM_H__ */
