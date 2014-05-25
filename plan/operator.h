#ifndef __PLAN_OPERATOR_H__
#define __PLAN_OPERATOR_H__

#include <plan/state.h>

struct _plan_operator_t {
    plan_part_state_t pre; /*!< Precondition */
    plan_part_state_t eff; /*!< Effect */
};
typedef struct _plan_operator_t plan_operator_t;


void planOperatorInit(plan_operator_t *op, plan_state_pool_t *state_pool);
void planOperatorFree(plan_operator_t *op);

void planOperatorSetPrecondition(plan_operator_t *op,
                                 unsigned var, unsigned val);
void planOperatorSetEffect(plan_operator_t *op,
                           unsigned var, unsigned val);

#endif /* __PLAN_OPERATOR_H__ */
