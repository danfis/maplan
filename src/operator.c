#include "plan/operator.h"

void planOperatorInit(plan_operator_t *op, plan_state_pool_t *state_pool)
{
    op->pre = planStatePoolNewPartState(state_pool);
    op->eff = planStatePoolNewPartState(state_pool);
}

void planOperatorFree(plan_operator_t *op)
{
}


void planOperatorSetPrecondition(plan_operator_t *op,
                                 unsigned var, unsigned val)
{
    planPartStateSet(&op->pre, var, val);
}

void planOperatorSetEffect(plan_operator_t *op,
                           unsigned var, unsigned val)
{
    planPartStateSet(&op->eff, var, val);
}
