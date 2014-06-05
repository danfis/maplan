#ifndef __PLAN_OPERATOR_H__
#define __PLAN_OPERATOR_H__

#include <plan/state.h>

struct _plan_operator_t {
    plan_state_pool_t *state_pool;
    plan_part_state_t *pre; /*!< Precondition */
    plan_part_state_t *eff; /*!< Effect */
    unsigned cost;
    char *name;
};
typedef struct _plan_operator_t plan_operator_t;


void planOperatorInit(plan_operator_t *op, plan_state_pool_t *state_pool);
void planOperatorFree(plan_operator_t *op);

void planOperatorSetPrecondition(plan_operator_t *op,
                                 unsigned var, unsigned val);
void planOperatorSetEffect(plan_operator_t *op,
                           unsigned var, unsigned val);
void planOperatorSetName(plan_operator_t *op, const char *name);
void planOperatorSetCost(plan_operator_t *op, unsigned cost);

/**
 * Applies the operator on the given state and store the resulting state
 * into state pool it has reference to. The ID of the resulting state is
 * returned.
 */
plan_state_id_t planOperatorApply(plan_operator_t *op,
                                  plan_state_id_t state_id);

#endif /* __PLAN_OPERATOR_H__ */
