#include <boruvka/alloc.h>

#include "plan/operator.h"

void planOperatorInit(plan_operator_t *op, plan_state_pool_t *state_pool)
{
    op->pre = planStatePoolNewPartState(state_pool);
    op->eff = planStatePoolNewPartState(state_pool);
    op->name = NULL;
    op->cost = 0;
}

void planOperatorFree(plan_operator_t *op)
{
    if (op->name)
        BOR_FREE(op->name);
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

void planOperatorSetName(plan_operator_t *op, const char *name)
{
    const char *c;
    size_t i;

    if (op->name)
        BOR_FREE(op->name);

    op->name = BOR_ALLOC_ARR(char, strlen(name));
    for (i = 0, c = name; c && *c; ++c, ++i){
        op->name[i] = *c;
    }
}

void planOperatorSetCost(plan_operator_t *op, unsigned cost)
{
    op->cost = cost;
}

