#ifndef __PLAN_PREF_OP_SELECTOR_H__
#define __PLAN_PREF_OP_SELECTOR_H__

#include "plan/heur.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pref_op_selector_t {
    plan_heur_res_t *pref_ops; /*!< In/Out data structure */
    plan_op_t *base_op;  /*!< Base pointer to the source operator array. */
    plan_op_t **cur;     /*!< Cursor pointing to the next operator in
                              .pref_ops->op[] */
    plan_op_t **end;     /*!< Points after .pref_ops->op[] */
    plan_op_t **ins;     /*!< Next insert position for preferred operator. */
};
typedef struct _plan_pref_op_selector_t plan_pref_op_selector_t;

/**
 * Initializes selector of preferred operators.
 */
void planPrefOpSelectorInit(plan_pref_op_selector_t *sel,
                            plan_heur_res_t *heur_res,
                            const plan_op_t *base_op);

/**
 * Selects specified operators as preferred operator.
 */
void planPrefOpSelectorSelect(plan_pref_op_selector_t *sel, int op_id);

/**
 * Finalize selection and writes data to in/out plan_heur_res_t structure.
 */
void planPrefOpSelectorFinalize(plan_pref_op_selector_t *sel);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_PREF_OP_SELECTOR_H__ */
