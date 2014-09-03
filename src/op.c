#include <boruvka/alloc.h>

#include "plan/op.h"

/** Initializes and frees conditional effect */
static void planOpCondEffInit(plan_op_cond_eff_t *ceff,
                              plan_state_pool_t *state_pool);
static void planOpCondEffFree(plan_op_cond_eff_t *ceff);
/** Copies conditional effect from src to dst */
static void planOpCondEffCopy(plan_state_pool_t *state_pool,
                              plan_op_cond_eff_t *dst,
                              const plan_op_cond_eff_t *src);
/** Bit operator: a = a | b */
_bor_inline void bitOr(void *a, const void *b, int size);

void planOpInit(plan_op_t *op, plan_state_pool_t *state_pool)
{
    bzero(op, sizeof(*op));
    op->state_pool = state_pool;
    op->pre = planPartStateNew(state_pool);
    op->eff = planPartStateNew(state_pool);
    op->cost = PLAN_COST_ZERO;
}

void planOpFree(plan_op_t *op)
{
    int i;

    if (op->name)
        BOR_FREE(op->name);
    planPartStateDel(op->pre);
    planPartStateDel(op->eff);

    for (i = 0; i < op->cond_eff_size; ++i){
        planOpCondEffFree(op->cond_eff + i);
    }
    if (op->cond_eff)
        BOR_FREE(op->cond_eff);
}

void planOpCopy(plan_op_t *dst, const plan_op_t *src)
{
    int i;

    planPartStateCopy(dst->pre, src->pre);
    planPartStateCopy(dst->eff, src->eff);

    if (src->cond_eff_size > 0){
        dst->cond_eff_size = src->cond_eff_size;
        dst->cond_eff = BOR_ALLOC_ARR(plan_op_cond_eff_t,
                                      dst->cond_eff_size);
        for (i = 0; i < dst->cond_eff_size; ++i){
            planOpCondEffInit(dst->cond_eff + i, dst->state_pool);
            planOpCondEffCopy(dst->state_pool,
                              dst->cond_eff + i, src->cond_eff + i);
        }
    }

    dst->cost = src->cost;
    dst->name = strdup(src->name);
}

static void planOpCondEffInit(plan_op_cond_eff_t *ceff,
                              plan_state_pool_t *state_pool)
{
    ceff->pre = planPartStateNew(state_pool);
    ceff->eff = planPartStateNew(state_pool);
}

void planOpSetName(plan_op_t *op, const char *name)
{
    if (op->name)
        BOR_FREE(op->name);
    op->name = strdup(name);
}

int planOpAddCondEff(plan_op_t *op)
{
    ++op->cond_eff_size;
    op->cond_eff = BOR_REALLOC_ARR(op->cond_eff, plan_op_cond_eff_t,
                                   op->cond_eff_size);
    planOpCondEffInit(op->cond_eff + op->cond_eff_size - 1,
                            op->state_pool);
    return op->cond_eff_size - 1;
}

void planOpCondEffSetPre(plan_op_t *op, int cond_eff,
                         plan_var_id_t var, plan_val_t val)
{
    planPartStateSet(op->cond_eff[cond_eff].pre, var, val);
}

void planOpCondEffSetEff(plan_op_t *op, int cond_eff,
                         plan_var_id_t var, plan_val_t val)
{
    planPartStateSet(op->cond_eff[cond_eff].eff, var, val);
}

void planOpDelLastCondEff(plan_op_t *op)
{
    planOpCondEffFree(op->cond_eff + op->cond_eff_size - 1);
    --op->cond_eff_size;
}

void planOpCondEffSimplify(plan_op_t *op)
{
    plan_op_cond_eff_t *e1, *e2;
    int i, j, tmpi, cond_eff_size;
    plan_op_cond_eff_t *cond_eff;
    plan_var_id_t var;
    plan_val_t val;

    if (op->cond_eff_size == 0)
        return;

    cond_eff_size = op->cond_eff_size;

    // Try to merge conditional effects with same conditions
    for (i = 0; i < op->cond_eff_size - 1; ++i){
        e1 = op->cond_eff + i;
        if (e1->pre == NULL)
            continue;

        for (j = i + 1; j < op->cond_eff_size; ++j){
            e2 = op->cond_eff + j;
            if (e2->pre == NULL)
                continue;

            if (planPartStateEq(e2->pre, e1->pre)){
                // Extend effects of e1 by effects of e2
                PLAN_PART_STATE_FOR_EACH(e2->eff, tmpi, var, val){
                    planPartStateSet(e1->eff, var, val);
                }

                // and destroy e2
                planOpCondEffFree(e2);
                e2->pre = e2->eff = NULL;
                --cond_eff_size;
            }
        }
    }

    if (cond_eff_size < op->cond_eff_size){
        // If some cond effect was deleted the array must be reassigned
        cond_eff = BOR_ALLOC_ARR(plan_op_cond_eff_t, cond_eff_size);
        for (i = 0, j = 0; i < op->cond_eff_size; ++i){
            if (op->cond_eff[i].pre != NULL){
                cond_eff[j++] = op->cond_eff[i];
            }
        }

        BOR_FREE(op->cond_eff);
        op->cond_eff = cond_eff;
        op->cond_eff_size = cond_eff_size;
    }
}

plan_state_id_t planOpApply(const plan_op_t *op, plan_state_id_t state_id)
{
    if (op->cond_eff_size == 0){
        // Use faster branch for non-conditional effects
        return planStatePoolApplyPartState(op->state_pool, op->eff, state_id);

    }else{
        int size = planStatePackerBufSize(op->state_pool->packer);
        char maskbuf[size];
        char valbuf[size];
        int i;

        // Initialize mask and value by non-conditional effects.
        memcpy(maskbuf, op->eff->maskbuf, size);
        memcpy(valbuf, op->eff->valbuf, size);

        // Test conditional effects
        for (i = 0; i < op->cond_eff_size; ++i){
            if (planStatePoolPartStateIsSubset(op->state_pool,
                                               op->cond_eff[i].pre,
                                               state_id)){
                // If condition associated with the effect holds, extend
                // mask and value buffers accordingly.
                bitOr(maskbuf, op->cond_eff[i].eff->maskbuf, size);
                bitOr(valbuf, op->cond_eff[i].eff->valbuf, size);
            }
        }

        return planStatePoolApplyPartState2(op->state_pool, maskbuf, valbuf,
                                            state_id);
    }
}

void planOpExtraMAOpAddRecvAgent(plan_op_t *op, int agent_id)
{
    ++op->extra.ma_op.recv_agent_size;
    op->extra.ma_op.recv_agent = BOR_REALLOC_ARR(op->extra.ma_op.recv_agent, int,
                                                 op->extra.ma_op.recv_agent_size);
    op->extra.ma_op.recv_agent[op->extra.ma_op.recv_agent_size - 1] = agent_id;
}

void planOpExtraMAOpFree(plan_op_t *op)
{
    if (op->extra.ma_op.recv_agent)
        BOR_FREE(op->extra.ma_op.recv_agent);
}

static void planOpCondEffFree(plan_op_cond_eff_t *ceff)
{
    planPartStateDel(ceff->pre);
    planPartStateDel(ceff->eff);
}

static void planOpCondEffCopy(plan_state_pool_t *state_pool,
                              plan_op_cond_eff_t *dst,
                              const plan_op_cond_eff_t *src)
{
    planPartStateCopy(dst->pre, src->pre);
    planPartStateCopy(dst->eff, src->eff);
}

_bor_inline void bitOr(void *a, const void *b, int size)
{
    uint32_t *a32;
    const uint32_t *b32;
    uint8_t *a8;
    const uint8_t *b8;
    int size32, size8;

    size32 = size / 4;
    a32 = a;
    b32 = b;
    for (; size32 != 0; --size32, ++a32, ++b32){
        *a32 |= *b32;
    }

    size8 = size % 4;
    a8 = (uint8_t *)a32;
    b8 = (uint8_t *)b32;
    for (; size8 != 0; --size8, ++a8, ++b8){
        *a8 |= *b8;
    }
}
