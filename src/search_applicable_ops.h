#ifndef __PLAN_SEARCH_APPLICABLE_OPS_H__
#define __PLAN_SEARCH_APPLICABLE_OPS_H__

#include <plan/op.h>
#include <plan/succgen.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_search_applicable_ops_t {
    plan_op_t **op;        /*!< Array of applicable operators. This array
                                must be big enough to hold all operators. */
    int op_size;           /*!< Size of .op[] */
    int op_found;          /*!< Number of found applicable operators */
    int op_preferred;      /*!< Number of preferred operators (that are
                                stored at the beggining of .op[] array */
    plan_state_id_t state; /*!< State in which these operators are
                                applicable */
};
typedef struct _plan_search_applicable_ops_t plan_search_applicable_ops_t;

/**
 * Initializes structure.
 */
void planSearchApplicableOpsInit(plan_search_applicable_ops_t *app_ops,
                                 int op_size);

/**
 * Frees resources
 */
void planSearchApplicableOpsFree(plan_search_applicable_ops_t *app_ops);

/**
 * Fills structure with applicable operators.
 */
void planSearchApplicableOpsFind(plan_search_applicable_ops_t *app_ops,
                                 const plan_state_t *state,
                                 plan_state_id_t state_id,
                                 const plan_succ_gen_t *succ_gen);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_SEARCH_APPLICABLE_OPS_H__ */
