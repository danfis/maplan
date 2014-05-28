#include <boruvka/alloc.h>

#include "plan/operator.h"

void planOperatorInit(plan_operator_t *op, plan_state_pool_t *state_pool)
{
    op->state_pool = state_pool;
    op->pre = planPartStateNew(state_pool);
    op->eff = planPartStateNew(state_pool);
    op->name = NULL;
    op->cost = 0;
}

void planOperatorFree(plan_operator_t *op)
{
    if (op->name)
        BOR_FREE(op->name);
    planPartStateDel(op->state_pool, op->pre);
    planPartStateDel(op->state_pool, op->eff);
}


void planOperatorSetPrecondition(plan_operator_t *op,
                                 unsigned var, unsigned val)
{
    planPartStateSet(op->pre, var, val);
}

void planOperatorSetEffect(plan_operator_t *op,
                           unsigned var, unsigned val)
{
    planPartStateSet(op->eff, var, val);
}

void planOperatorSetName(plan_operator_t *op, const char *name)
{
    const char *c;
    size_t i;

    if (op->name)
        BOR_FREE(op->name);

    op->name = BOR_ALLOC_ARR(char, strlen(name) + 1);
    for (i = 0, c = name; c && *c; ++c, ++i){
        op->name[i] = *c;
    }
    op->name[i] = 0;
}

void planOperatorSetCost(plan_operator_t *op, unsigned cost)
{
    op->cost = cost;
}

