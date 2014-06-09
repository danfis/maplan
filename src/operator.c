#include <boruvka/alloc.h>

#include "plan/operator.h"

void planOperatorInit(plan_operator_t *op, plan_state_pool_t *state_pool)
{
    op->state_pool = state_pool;
    op->pre = planPartStateNew(state_pool);
    op->eff = planPartStateNew(state_pool);
    op->name = NULL;
    op->cost = PLAN_COST_ZERO;
}

void planOperatorFree(plan_operator_t *op)
{
    if (op->name)
        BOR_FREE(op->name);
    planPartStateDel(op->state_pool, op->pre);
    planPartStateDel(op->state_pool, op->eff);
}


void planOperatorSetPrecondition(plan_operator_t *op,
                                 plan_var_id_t var,
                                 plan_val_t val)
{
    planPartStateSet(op->state_pool, op->pre, var, val);
}

void planOperatorSetEffect(plan_operator_t *op,
                           plan_var_id_t var,
                           plan_val_t val)
{
    planPartStateSet(op->state_pool, op->eff, var, val);
}

void planOperatorSetName(plan_operator_t *op, const char *name)
{
    const char *c;
    int i;

    if (op->name)
        BOR_FREE(op->name);

    op->name = BOR_ALLOC_ARR(char, strlen(name) + 1);
    for (i = 0, c = name; c && *c; ++c, ++i){
        op->name[i] = *c;
    }
    op->name[i] = 0;
}

void planOperatorSetCost(plan_operator_t *op, plan_cost_t cost)
{
    op->cost = cost;
}

plan_state_id_t planOperatorApply(plan_operator_t *op,
                                  plan_state_id_t state_id)
{
    return planStatePoolApplyPartState(op->state_pool, op->eff, state_id);
}
