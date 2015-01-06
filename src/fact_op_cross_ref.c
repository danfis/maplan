#include <boruvka/alloc.h>

#include "fact_op_cross_ref.h"

static void crossRefOpFact(int op_id, int ins, int fact_id,
                           plan_factarr_t *op, plan_oparr_t *fact)
{
    op[op_id].fact[ins] = fact_id;

    ++fact[fact_id].size;
    fact[fact_id].op = BOR_REALLOC_ARR(fact[fact_id].op, int,
                                       fact[fact_id].size);
    fact[fact_id].op[fact[fact_id].size - 1] = op_id;
}

static void crossRefPartState(const plan_part_state_t *ps,
                              const plan_part_state_t *ps2,
                              int op_id, const plan_fact_id_t *fid,
                              plan_factarr_t *op, plan_oparr_t *fact)
{
    int i, fact_id, ins;
    plan_var_id_t var;
    plan_val_t val;

    op[op_id].size = ps->vals_size;
    if (ps2)
        op[op_id].size += ps2->vals_size;
    op[op_id].fact = BOR_ALLOC_ARR(int, op[op_id].size);

    ins = 0;
    PLAN_PART_STATE_FOR_EACH(ps, i, var, val){
        fact_id = planFactId(fid, var, val);
        crossRefOpFact(op_id, ins++, fact_id, op, fact);
    }

    if (ps2){
        PLAN_PART_STATE_FOR_EACH(ps2, i, var, val){
            fact_id = planFactId(fid, var, val);
            crossRefOpFact(op_id, ins++, fact_id, op, fact);
        }
    }
}

static void crossRefNoPre(plan_fact_op_cross_ref_t *cr, int op_id)
{
    cr->op_pre[op_id].size = 1;
    cr->op_pre[op_id].fact = BOR_ALLOC_ARR(int, 1);
    crossRefOpFact(op_id, 0, cr->fake_pre[0].fact_id, cr->op_pre, cr->fact_pre);
}

static void crossRefOp(plan_fact_op_cross_ref_t *cr,
                       int id, const plan_op_t *op)
{
    if (op->pre->vals_size > 0){
        crossRefPartState(op->pre, NULL, id, &cr->fact_id,
                          cr->op_pre, cr->fact_pre);
    }else{
        crossRefNoPre(cr, id);
    }

    if (op->eff->vals_size > 0){
        crossRefPartState(op->eff, NULL, id, &cr->fact_id,
                          cr->op_eff, cr->fact_eff);
    }
}

static void crossRefCondEff(plan_fact_op_cross_ref_t *cr, int id,
                            const plan_op_t *op,
                            const plan_op_cond_eff_t *cond_eff)
{
    if (op->pre->vals_size > 0 || cond_eff->pre->vals_size > 0){
        crossRefPartState(op->pre, cond_eff->pre, id, &cr->fact_id,
                          cr->op_pre, cr->fact_pre);
    }else{
        crossRefNoPre(cr, id);
    }

    if (op->eff->vals_size > 0 || cond_eff->eff->vals_size > 0){
        crossRefPartState(op->eff, cond_eff->eff, id, &cr->fact_id,
                          cr->op_eff, cr->fact_eff);
    }
}

static void crossRefOps(plan_fact_op_cross_ref_t *cr,
                        const plan_op_t *op, int op_size)
{
    int i, j;
    int cond_eff_ins = op_size;

    for (i = 0; i < op_size; ++i){
        crossRefOp(cr, i, op + i);
        for (j = 0; j < op[i].cond_eff_size; ++j){
            crossRefCondEff(cr, cond_eff_ins++, op + i, op[i].cond_eff + j);
        }
    }
}

static void addGoalOp(plan_fact_op_cross_ref_t *cr,
                      const plan_part_state_t *goal)
{
    crossRefPartState(goal, NULL, cr->goal_op_id, &cr->fact_id,
                      cr->op_pre, cr->fact_pre);

    cr->op_eff[cr->goal_op_id].size = 1;
    cr->op_eff[cr->goal_op_id].fact = BOR_ALLOC_ARR(int, 1);
    cr->op_eff[cr->goal_op_id].fact[0] = cr->goal_id;
    cr->fact_eff[cr->goal_id].size = 1;
    cr->fact_eff[cr->goal_id].op = BOR_ALLOC_ARR(int, 1);
    cr->fact_eff[cr->goal_id].op[0] = cr->goal_op_id;
}

static void setOpId(plan_fact_op_cross_ref_t *cr,
                    const plan_op_t *op, int op_size)
{
    int i, j, cond_eff;

    cr->op_id = BOR_ALLOC_ARR(int, cr->op_size);
    cond_eff = op_size;
    for (i = 0; i < op_size; ++i){
        cr->op_id[i] = i;
        for (j = 0; j < op[i].cond_eff_size; ++j)
            cr->op_id[cond_eff++] = i;
    }
    cr->op_id[cr->goal_op_id] = -1;
}

static int sortIntCmp(const void *a, const void *b)
{
    int i1 = *(int *)a;
    int i2 = *(int *)b;
    return i1 - i2;
}

void planFactOpCrossRefInit(plan_fact_op_cross_ref_t *cr,
                            const plan_var_t *var, int var_size,
                            const plan_part_state_t *goal,
                            const plan_op_t *op, int op_size)
{
    int i;

    planFactIdInit(&cr->fact_id, var, var_size);

    // Allocate artificial precondition for operators without preconditions
    cr->fake_pre_size = 1;
    cr->fake_pre = BOR_ALLOC_ARR(plan_fake_pre_t, 1);

    // Determine number of facts and add artificial facts
    cr->fact_size = cr->fact_id.fact_size;
    cr->goal_id = cr->fact_size++;
    cr->fake_pre[0].fact_id = cr->fact_size++;
    cr->fake_pre[0].value = 0;

    // Compute final number of operators
    cr->op_size = op_size;
    for (i = 0; i < op_size; ++i){
        cr->op_size += op[i].cond_eff_size;
    }

    // Add artificial goal-reaching operator
    cr->goal_op_id = cr->op_size++;

    // Allocate arrays
    cr->fact_pre = BOR_CALLOC_ARR(plan_oparr_t, cr->fact_size);
    cr->fact_eff = BOR_CALLOC_ARR(plan_oparr_t, cr->fact_size);
    cr->op_pre = BOR_CALLOC_ARR(plan_factarr_t, cr->op_size);
    cr->op_eff = BOR_CALLOC_ARR(plan_factarr_t, cr->op_size);

    // Compute cross reference tables for operators
    crossRefOps(cr, op, op_size);

    // Adds operator reaching artificial goal
    addGoalOp(cr, goal);

    // Set up .op_id[] array
    setOpId(cr, op, op_size);
}

void planFactOpCrossRefFree(plan_fact_op_cross_ref_t *cr)
{
    int i;

    for (i = 0; i < cr->op_size; ++i){
        if (cr->op_pre[i].fact != NULL)
            BOR_FREE(cr->op_pre[i].fact);
        if (cr->op_eff[i].fact != NULL)
            BOR_FREE(cr->op_eff[i].fact);
    }
    BOR_FREE(cr->op_pre);
    BOR_FREE(cr->op_eff);

    for (i = 0; i < cr->fact_size; ++i){
        if (cr->fact_pre[i].op != NULL)
            BOR_FREE(cr->fact_pre[i].op);
        if (cr->fact_eff[i].op != NULL)
            BOR_FREE(cr->fact_eff[i].op);
    }
    BOR_FREE(cr->fact_pre);
    BOR_FREE(cr->fact_eff);
    if (cr->fake_pre)
        BOR_FREE(cr->fake_pre);
    BOR_FREE(cr->op_id);

    planFactIdFree(&cr->fact_id);
}

int planFactOpCrossRefAddFakePre(plan_fact_op_cross_ref_t *cr,
                                 plan_cost_t value)
{
    int fact_id;

    fact_id = cr->fact_size++;
    cr->fact_pre = BOR_REALLOC_ARR(cr->fact_pre, plan_oparr_t, cr->fact_size);
    cr->fact_eff = BOR_REALLOC_ARR(cr->fact_eff, plan_oparr_t, cr->fact_size);

    cr->fact_pre[fact_id].size = 0;
    cr->fact_pre[fact_id].op = NULL;
    cr->fact_eff[fact_id].size = 0;
    cr->fact_eff[fact_id].op = NULL;

    ++cr->fake_pre_size;
    cr->fake_pre = BOR_REALLOC_ARR(cr->fake_pre, plan_fake_pre_t,
                                   cr->fake_pre_size);
    cr->fake_pre[cr->fake_pre_size - 1].fact_id = fact_id;
    cr->fake_pre[cr->fake_pre_size - 1].value = value;

    return fact_id;
}

void planFactOpCrossRefSetFakePreValue(plan_fact_op_cross_ref_t *cr,
                                       int fact_id, plan_cost_t value)
{
    int i;

    for (i = 0; i < cr->fake_pre_size; ++i){
        if (cr->fake_pre[i].fact_id == fact_id)
            cr->fake_pre[i].value = value;
    }
}

void planFactOpCrossRefAddPre(plan_fact_op_cross_ref_t *cr,
                              int op_id, int fact_id)
{
    plan_oparr_t *oparr;
    plan_factarr_t *factarr;

    oparr = cr->fact_pre + fact_id;
    ++oparr->size;
    oparr->op = BOR_REALLOC_ARR(oparr->op, int, oparr->size);
    oparr->op[oparr->size - 1] = op_id;
    qsort(oparr->op, oparr->size, sizeof(int), sortIntCmp);

    factarr = cr->op_pre + op_id;
    ++factarr->size;
    factarr->fact = BOR_REALLOC_ARR(factarr->fact, int, factarr->size);
    factarr->fact[factarr->size - 1] = fact_id;
    qsort(factarr->fact, factarr->size, sizeof(int), sortIntCmp);
}
