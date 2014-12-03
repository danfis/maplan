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

    op[op_id].size = planPartStateSize(ps);
    if (ps2)
        op[op_id].size = planPartStateSize(ps2);
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
    crossRefOpFact(op_id, 0, cr->no_pre_id, cr->op_pre, cr->fact_pre);
}

static void crossRefOp(plan_fact_op_cross_ref_t *cr,
                       int id, const plan_op_t *op)
{
    if (planPartStateSize(op->pre) > 0){
        crossRefPartState(op->pre, NULL, id, &cr->fact_id,
                          cr->op_pre, cr->fact_pre);
    }else{
        crossRefNoPre(cr, id);
    }

    if (planPartStateSize(op->eff) > 0){
        crossRefPartState(op->eff, NULL, id, &cr->fact_id,
                          cr->op_eff, cr->fact_eff);
    }
}

static void crossRefCondEff(plan_fact_op_cross_ref_t *cr, int id,
                            const plan_op_t *op,
                            const plan_op_cond_eff_t *cond_eff)
{
    if (planPartStateSize(op->pre) > 0
            || planPartStateSize(cond_eff->pre) > 0){
        crossRefPartState(op->pre, cond_eff->pre, id, &cr->fact_id,
                          cr->op_pre, cr->fact_pre);
    }else{
        crossRefNoPre(cr, id);
    }

    if (planPartStateSize(op->eff) > 0
            || planPartStateSize(cond_eff->eff) > 0){
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

void planFactOpCrossRefInit(plan_fact_op_cross_ref_t *cr,
                            const plan_var_t *var, int var_size,
                            const plan_part_state_t *goal,
                            const plan_op_t *op, int op_size)
{
    int i;

    planFactIdInit(&cr->fact_id, var, var_size);

    // Determine number of facts and add artificial facts
    cr->fact_size = cr->fact_id.fact_size;
    cr->goal_id = cr->fact_size++;
    cr->no_pre_id = cr->fact_size++;

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

    planFactIdFree(&cr->fact_id);
}
