/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#include <boruvka/alloc.h>

#include "plan/heur.h"
#include "fact_op_cross_ref.h"

static void crossRefOpFact(int op_id, int fact_id,
                           plan_arr_int_t *op, plan_arr_int_t *fact)
{
    planArrIntAdd(op + op_id, fact_id);
    planArrIntAdd(fact + fact_id, op_id);
}

static void crossRefPartState(const plan_part_state_t *ps,
                              int op_id, const plan_fact_id_t *fid,
                              plan_arr_int_t *op, plan_arr_int_t *fact)
{
    int fact_id;

    PLAN_FACT_ID_FOR_EACH_PART_STATE(fid, ps, fact_id){
        crossRefOpFact(op_id, fact_id, op, fact);
    }
}

static void crossRefNoPre(plan_fact_op_cross_ref_t *cr, int op_id)
{
    crossRefOpFact(op_id, cr->fake_pre[0].fact_id, cr->op_pre, cr->fact_pre);
}

static void crossRefOp(plan_fact_op_cross_ref_t *cr,
                       int id, const plan_op_t *op)
{
    if (op->pre->vals_size > 0){
        crossRefPartState(op->pre, id, &cr->fact_id,
                          cr->op_pre, cr->fact_pre);
    }else{
        crossRefNoPre(cr, id);
    }

    if (op->eff->vals_size > 0){
        crossRefPartState(op->eff, id, &cr->fact_id,
                          cr->op_eff, cr->fact_eff);
    }
}

static void crossRefCondEff(plan_fact_op_cross_ref_t *cr, int id,
                            const plan_op_t *op,
                            const plan_op_cond_eff_t *cond_eff)
{
    if (op->pre->vals_size > 0 || cond_eff->pre->vals_size > 0){
        crossRefPartState(op->pre, id, &cr->fact_id,
                          cr->op_pre, cr->fact_pre);
        crossRefPartState(cond_eff->pre, id, &cr->fact_id,
                          cr->op_pre, cr->fact_pre);
    }else{
        crossRefNoPre(cr, id);
    }

    if (op->eff->vals_size > 0 || cond_eff->eff->vals_size > 0){
        crossRefPartState(op->eff, id, &cr->fact_id,
                          cr->op_eff, cr->fact_eff);
        crossRefPartState(cond_eff->eff, id, &cr->fact_id,
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
    crossRefPartState(goal, cr->goal_op_id, &cr->fact_id,
                      cr->op_pre, cr->fact_pre);

    planArrIntAdd(cr->op_eff + cr->goal_op_id, cr->goal_id);
    planArrIntAdd(cr->fact_eff + cr->goal_id, cr->goal_op_id);
}

static void addH2Op(plan_fact_op_cross_ref_t *cr,
                    int parent, const plan_op_t *op,
                    int add_fact, int *op_alloc)
{
    /*
    int op_id, i, fact_id, ins;

    if (*op_alloc == cr->op_size){
        *op_alloc *= 2;
        cr->op_pre = BOR_REALLOC_ARR(cr->op_pre, plan_arr_int_t, *op_alloc);
        cr->op_eff = BOR_REALLOC_ARR(cr->op_eff, plan_arr_int_t, *op_alloc);
    }
    op_id = cr->op_size++;
    fprintf(stderr, "addH2Op: %d <- %d\n", op_id, parent);

    // Allocate space for preconditions
    cr->op_pre[op_id].size  = cr->op_pre[parent].size;
    cr->op_pre[op_id].size += op->pre->vals_size + 1;
    cr->op_pre[op_id].fact = BOR_ALLOC_ARR(int, cr->op_pre[op_id].size);

    // Copy preconditions from the parent
    ins = 0;
    for (i = 0; i < cr->op_pre[parent].size; ++i)
        crossRefOpFact(op_id, ins++, cr->op_pre[parent].fact[i],
                       cr->op_pre, cr->fact_pre);

    // And add combinations with add_fact
    crossRefOpFact(op_id, ins++, add_fact, cr->op_pre, cr->fact_pre);
    fprintf(stderr, "%d -- %d\n", op->pre->vals_size,
            cr->op_pre[parent].size);
    fflush(stderr);
    for (i = 0; i < op->pre->vals_size; ++i){
        fact_id = planFactIdVar(&cr->fact_id, op->pre->vals[i].var,
                                              op->pre->vals[i].val);
        fact_id = planFactIdFact2(&cr->fact_id, fact_id, add_fact);
        crossRefOpFact(op_id, ins++, fact_id, cr->op_pre, cr->fact_pre);
    }

    // Allocate space for effects
    cr->op_eff[op_id].size  = cr->op_eff[parent].size;
    cr->op_eff[op_id].size += op->eff->vals_size;
    cr->op_eff[op_id].fact = BOR_ALLOC_ARR(int, cr->op_eff[op_id].size);

    // Copy effects from the parent
    ins = 0;
    for (i = 0; i < cr->op_eff[parent].size; ++i)
        crossRefOpFact(op_id, ins++, cr->op_eff[parent].fact[i],
                       cr->op_eff, cr->fact_eff);

    // And add combinations with add_fact
    for (i = 0; i < op->eff->vals_size; ++i){
        fact_id = planFactIdVar(&cr->fact_id, op->eff->vals[i].var,
                                              op->eff->vals[i].val);
        fact_id = planFactIdFact2(&cr->fact_id, fact_id, add_fact);
        crossRefOpFact(op_id, ins++, fact_id, cr->op_eff, cr->fact_eff);
    }
    */
}

static void addH2OpsFromOp(plan_fact_op_cross_ref_t *cr,
                           int op_id, const plan_op_t *op,
                           const plan_var_t *pvar, int pvar_size,
                           int *op_alloc)
{
    const plan_part_state_t *pre = op->pre;
    const plan_part_state_t *eff = op->eff;
    int prei, effi, var, val;

    // Skip operators without effects
    if (op->eff->vals_size == 0)
        return;

    for (prei = effi = var = 0; var < pvar_size; ++var){
        if (effi < eff->vals_size && var == eff->vals[effi].var){
            if (prei < pre->vals_size
                    && pre->vals[prei].var == eff->vals[effi].var){
                ++prei;
            }
            ++effi;
            continue;
        }

        if (prei < pre->vals_size && var == pre->vals[prei].var){
            ++prei;
            continue;
        }

        for (val = 0; val < pvar[var].range; ++val){
            addH2Op(cr, op_id, op + op_id,
                    planFactIdVar(&cr->fact_id, var, val),
                    op_alloc);
        }

        /*
        if (prei < pre->vals_size && var == pre->vals[prei].var)
            ++prei;
        */
    }
}

static void addH2Ops(plan_fact_op_cross_ref_t *cr,
                     const plan_op_t *op, int op_size,
                     const plan_var_t *var, int var_size)
{
    int op_alloc = cr->op_size;
    int op_id;

    for (op_id = 0; op_id < op_size; ++op_id){
        addH2OpsFromOp(cr, op_id, op + op_id, var, var_size, &op_alloc);
    }
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
                            const plan_op_t *op, int op_size,
                            unsigned flags)
{
    unsigned fact_id_flags = 0;
    int i;

    if (flags & PLAN_HEUR_H2)
        fact_id_flags = PLAN_FACT_ID_H2;

    cr->flags = flags;
    planFactIdInit(&cr->fact_id, var, var_size, fact_id_flags);

    // Allocate artificial precondition for operators without preconditions
    cr->fake_pre_size = 1;
    cr->fake_pre = BOR_ALLOC_ARR(plan_fake_pre_t, 1);

    // Determine number of facts and add artificial facts
    cr->fact_size = cr->fact_id.fact_size;
    cr->goal_id = cr->fact_size++;
    cr->fake_pre[0].fact_id = cr->fact_size++;
    cr->fake_pre[0].value = 0;

    // Compute number of operators including conditional effects
    cr->op_size = op_size;
    for (i = 0; i < op_size; ++i){
        cr->op_size += op[i].cond_eff_size;
    }

    // Disable H2 for conditional effects for now
    if (flags & PLAN_HEUR_H2 && cr->op_size != op_size){
        fprintf(stderr, "ERROR: H2 does not work with conditinal effects"
                        " for now!\n");
        exit(-1);
    }

    // Add artificial goal-reaching operator
    cr->goal_op_id = cr->op_size++;

    // Allocate arrays
    cr->fact_pre = BOR_CALLOC_ARR(plan_arr_int_t, cr->fact_size);
    cr->fact_eff = BOR_CALLOC_ARR(plan_arr_int_t, cr->fact_size);
    cr->op_pre = BOR_CALLOC_ARR(plan_arr_int_t, cr->op_size);
    cr->op_eff = BOR_CALLOC_ARR(plan_arr_int_t, cr->op_size);

    // Compute cross reference tables for operators
    crossRefOps(cr, op, op_size);

    // Adds operator reaching artificial goal
    addGoalOp(cr, goal);

    // Set up .op_id[] array
    setOpId(cr, op, op_size);

    // Add h^2 operators
    // TODO
    if (flags & PLAN_HEUR_H2)
        addH2Ops(cr, op, op_size, var, var_size);
}

void planFactOpCrossRefFree(plan_fact_op_cross_ref_t *cr)
{
    int i;

    for (i = 0; i < cr->op_size; ++i){
        planArrIntFree(cr->op_pre + i);
        planArrIntFree(cr->op_eff + i);
    }
    if (cr->op_pre)
        BOR_FREE(cr->op_pre);
    if (cr->op_eff)
        BOR_FREE(cr->op_eff);

    for (i = 0; i < cr->fact_size; ++i){
        planArrIntFree(cr->fact_pre + i);
        planArrIntFree(cr->fact_eff + i);
    }
    if (cr->fact_pre)
        BOR_FREE(cr->fact_pre);
    if (cr->fact_eff)
        BOR_FREE(cr->fact_eff);
    if (cr->fake_pre)
        BOR_FREE(cr->fake_pre);
    if (cr->op_id)
        BOR_FREE(cr->op_id);

    planFactIdFree(&cr->fact_id);
}

int planFactOpCrossRefAddFakePre(plan_fact_op_cross_ref_t *cr,
                                 plan_cost_t value)
{
    int fact_id;

    fact_id = cr->fact_size++;
    cr->fact_pre = BOR_REALLOC_ARR(cr->fact_pre, plan_arr_int_t, cr->fact_size);
    cr->fact_eff = BOR_REALLOC_ARR(cr->fact_eff, plan_arr_int_t, cr->fact_size);

    bzero(cr->fact_pre + fact_id, sizeof(*cr->fact_pre));
    bzero(cr->fact_eff + fact_id, sizeof(*cr->fact_eff));

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
    planArrIntAdd(cr->fact_pre + fact_id, op_id);
    planArrIntSort(cr->fact_pre + fact_id);

    planArrIntAdd(cr->op_pre + op_id, fact_id);
    planArrIntSort(cr->op_pre + op_id);
}
