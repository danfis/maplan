#include <boruvka/alloc.h>

#include "plan/operator.h"

void planOperatorCondEffInit(plan_operator_cond_eff_t *ceff,
                             plan_state_pool_t *state_pool)
{
    ceff->pre = planPartStateNew(state_pool);
    ceff->eff = planPartStateNew(state_pool);
}

void planOperatorCondEffFree(plan_operator_cond_eff_t *ceff)
{
    planPartStateDel(ceff->pre);
    planPartStateDel(ceff->eff);
}

void planOperatorCondEffCopy(plan_state_pool_t *state_pool,
                             plan_operator_cond_eff_t *dst,
                             const plan_operator_cond_eff_t *src)
{
    int i;
    plan_var_id_t var;
    plan_val_t val;

    PLAN_PART_STATE_FOR_EACH(src->pre, i, var, val){
        planPartStateSet(state_pool, dst->pre, var, val);
    }

    PLAN_PART_STATE_FOR_EACH(src->eff, i, var, val){
        planPartStateSet(state_pool, dst->eff, var, val);
    }
}

void planOperatorInit(plan_operator_t *op, plan_state_pool_t *state_pool)
{
    op->state_pool = state_pool;
    op->pre = planPartStateNew(state_pool);
    op->eff = planPartStateNew(state_pool);
    op->cond_eff = NULL;
    op->cond_eff_size = 0;
    op->name = NULL;
    op->cost = PLAN_COST_ZERO;
    op->is_private = 0;
}

void planOperatorFree(plan_operator_t *op)
{
    int i;

    if (op->name)
        BOR_FREE(op->name);
    planPartStateDel(op->pre);
    planPartStateDel(op->eff);

    for (i = 0; i < op->cond_eff_size; ++i){
        planOperatorCondEffFree(op->cond_eff + i);
    }
    if (op->cond_eff)
        BOR_FREE(op->cond_eff);
}

void planOperatorCopy(plan_operator_t *dst, const plan_operator_t *src)
{
    int i;
    plan_var_id_t var;
    plan_val_t val;

    PLAN_PART_STATE_FOR_EACH(src->pre, i, var, val){
        planPartStateSet(dst->state_pool, dst->pre, var, val);
    }

    PLAN_PART_STATE_FOR_EACH(src->eff, i, var, val){
        planPartStateSet(dst->state_pool, dst->eff, var, val);
    }

    if (src->cond_eff_size > 0){
        dst->cond_eff_size = src->cond_eff_size;
        dst->cond_eff = BOR_ALLOC_ARR(plan_operator_cond_eff_t,
                                      dst->cond_eff_size);
        for (i = 0; i < dst->cond_eff_size; ++i){
            planOperatorCondEffInit(dst->cond_eff + i, dst->state_pool);
            planOperatorCondEffCopy(dst->state_pool,
                                    dst->cond_eff + i,
                                    src->cond_eff + i);
        }
    }

    dst->cost = src->cost;
    dst->name = strdup(src->name);
    dst->is_private = src->is_private;
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

int planOperatorAddCondEff(plan_operator_t *op)
{
    ++op->cond_eff_size;
    op->cond_eff = BOR_REALLOC_ARR(op->cond_eff, plan_operator_cond_eff_t,
                                   op->cond_eff_size);
    planOperatorCondEffInit(op->cond_eff + op->cond_eff_size - 1,
                            op->state_pool);
    return op->cond_eff_size - 1;
}

void planOperatorCondEffPreSet(plan_operator_t *op, int cond_eff,
                               plan_var_id_t var, plan_val_t val)
{
    planPartStateSet(op->state_pool, op->cond_eff[cond_eff].pre, var, val);
}

void planOperatorCondEffEffSet(plan_operator_t *op, int cond_eff,
                               plan_var_id_t var, plan_val_t val)
{
    planPartStateSet(op->state_pool, op->cond_eff[cond_eff].eff, var, val);
}

plan_state_id_t planOperatorApply(const plan_operator_t *op,
                                  plan_state_id_t state_id)
{
    return planStatePoolApplyPartState(op->state_pool, op->eff, state_id);
}
