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

static int nextOp(plan_fact_op_cross_ref_t *cr, int prob_op_id)
{
    int op_id;

    if (cr->op_size == cr->op_alloc){
        if (cr->op_alloc == 0)
            cr->op_alloc = 8;
        cr->op_alloc *= 2;
        cr->op_pre = BOR_REALLOC_ARR(cr->op_pre, plan_arr_int_t, cr->op_alloc);
        cr->op_eff = BOR_REALLOC_ARR(cr->op_eff, plan_arr_int_t, cr->op_alloc);
        cr->op_id  = BOR_REALLOC_ARR(cr->op_id, int, cr->op_alloc);
    }

    op_id = cr->op_size++;
    bzero(cr->op_pre + op_id, sizeof(plan_arr_int_t));
    bzero(cr->op_eff + op_id, sizeof(plan_arr_int_t));
    cr->op_id[op_id] = prob_op_id;
    return op_id;
}

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

static void crossRefPartStateCombs2(plan_fact_op_cross_ref_t *cr, int op_id,
                                    int fact_id, const plan_part_state_t *ps,
                                    plan_arr_int_t *op, plan_arr_int_t *fact)
{
    int i, fact_id2, fact2;

    for (i = 0; i < ps->vals_size; ++i){
        fact_id2 = planFactIdVar(&cr->fact_id,
                                 ps->vals[i].var, ps->vals[i].val);
        fact2 = planFactIdFact2(&cr->fact_id, fact_id, fact_id2);
        crossRefOpFact(op_id, fact2, op, fact);
    }
}

static void expandOpH2(plan_fact_op_cross_ref_t *cr,
                       const plan_op_t *op, int id)
{
    const plan_part_state_t *pre = op->pre;
    const plan_part_state_t *eff = op->eff;
    int prei, effi, fact_id;

    // Skip operators without effects
    if (op->eff->vals_size == 0)
        return;

    // Find preconditions that are not changed by the effect and add
    // combinations of this precondition with all effects
    for (prei = effi = 0; prei < pre->vals_size && effi < eff->vals_size;){
        if (pre->vals[prei].var == eff->vals[effi].var){
            ++prei;
            ++effi;

        }else if (pre->vals[prei].var < eff->vals[effi].var){
            fact_id = planFactIdVar(&cr->fact_id, pre->vals[prei].var,
                                                  pre->vals[prei].val);
            crossRefPartStateCombs2(cr, id, fact_id, eff,
                                    cr->op_eff, cr->fact_eff);
            ++prei;

        }else{
            ++effi;
        }
    }

    for (; prei < pre->vals_size; ++prei){
        fact_id = planFactIdVar(&cr->fact_id, pre->vals[prei].var,
                                              pre->vals[prei].val);
        crossRefPartStateCombs2(cr, id, fact_id, eff,
                                cr->op_eff, cr->fact_eff);
    }
}

static void crossRefOp(plan_fact_op_cross_ref_t *cr,
                       int op_id, const plan_op_t *op)
{
    int id = nextOp(cr, op_id);

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

    if (op->eff->vals_size > 0 && cr->flags & PLAN_HEUR_H2)
        expandOpH2(cr, op, id);
}

static void crossRefCondEff(plan_fact_op_cross_ref_t *cr,
                            int op_id, const plan_op_t *op,
                            const plan_op_cond_eff_t *cond_eff)
{
    int id = nextOp(cr, op_id);

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

static void crossRefH2Op(plan_fact_op_cross_ref_t *cr,
                         int op_id, const plan_op_t *op,
                         int fact_id)
{
    int id = nextOp(cr, op_id);
    int val;

    // Copy parent operator
    PLAN_ARR_INT_FOR_EACH(cr->op_pre + op_id, val)
        crossRefOpFact(id, val, cr->op_pre, cr->fact_pre);
    PLAN_ARR_INT_FOR_EACH(cr->op_eff + op_id, val)
        crossRefOpFact(id, val, cr->op_eff, cr->fact_eff);

    // Add preconditions
    crossRefOpFact(id, fact_id, cr->op_pre, cr->fact_pre);
    crossRefPartStateCombs2(cr, id, fact_id, op->pre,
                            cr->op_pre, cr->fact_pre);

    // Skip operators without effects
    if (op->eff->vals_size == 0)
        return;

    // Add effects
    crossRefPartStateCombs2(cr, id, fact_id, op->eff,
                            cr->op_eff, cr->fact_eff);
}

static void crossRefH2(plan_fact_op_cross_ref_t *cr,
                       int op_id, const plan_op_t *op,
                       const plan_var_t *pvar, int pvar_size)
{
    const plan_part_state_t *pre = op->pre;
    const plan_part_state_t *eff = op->eff;
    int prei, effi, var, val, fact_id;

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
            fact_id = planFactIdVar(&cr->fact_id, var, val);
            crossRefH2Op(cr, op_id, op, fact_id);
        }
    }
}

static void crossRefOps(plan_fact_op_cross_ref_t *cr,
                        const plan_op_t *op, int op_size,
                        const plan_var_t *var, int var_size)
{
    int i, j, have_cond_eff = 0;

    for (i = 0; i < op_size; ++i){
        crossRefOp(cr, i, op + i);
        have_cond_eff |= op[i].cond_eff_size;
    }

    if (have_cond_eff){
        // Disable H2 for conditional effects for now
        if (cr->flags & PLAN_HEUR_H2){
            fprintf(stderr, "ERROR: H2 does not work with conditinal effects"
                    " for now!\n");
            exit(-1);
        }

        for (i = 0; i < op_size; ++i){
            for (j = 0; j < op[i].cond_eff_size; ++j){
                crossRefCondEff(cr, i, op + i, op[i].cond_eff + j);
            }
        }
    }

    if (cr->flags & PLAN_HEUR_H2){
        for (i = 0; i < op_size; ++i){
            crossRefH2(cr, i, op + i, var, var_size);
        }
    }

    /*
    for (i = 0; i < cr->op_size; ++i){
        fprintf(stderr, "op[% 3d] pre:", i);
        for (j = 0; j < cr->op_pre[i].size; ++j){
            fprintf(stderr, " %d", cr->op_pre[i].arr[j]);
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "        eff:");
        for (j = 0; j < cr->op_eff[i].size; ++j){
            fprintf(stderr, " %d", cr->op_eff[i].arr[j]);
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "        op_id: %d\n", cr->op_id[i]);
    }

    for (i = 0; i < cr->fact_size; ++i){
        fprintf(stderr, "fact[% 3d] pre:", i);
        for (j = 0; j < cr->fact_pre[i].size; ++j){
            fprintf(stderr, " %d", cr->fact_pre[i].arr[j]);
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "          eff:");
        for (j = 0; j < cr->fact_eff[i].size; ++j){
            fprintf(stderr, " %d", cr->fact_eff[i].arr[j]);
        }
        fprintf(stderr, "\n");
    }
    */
}

static void addGoalOp(plan_fact_op_cross_ref_t *cr,
                      const plan_part_state_t *goal)
{
    cr->goal_op_id = nextOp(cr, -1);

    crossRefPartState(goal, cr->goal_op_id, &cr->fact_id,
                      cr->op_pre, cr->fact_pre);

    planArrIntAdd(cr->op_eff + cr->goal_op_id, cr->goal_id);
    planArrIntAdd(cr->fact_eff + cr->goal_id, cr->goal_op_id);
}


void planFactOpCrossRefInit(plan_fact_op_cross_ref_t *cr,
                            const plan_var_t *var, int var_size,
                            const plan_part_state_t *goal,
                            const plan_op_t *op, int op_size,
                            unsigned flags)
{
    unsigned fact_id_flags = 0;

    bzero(cr, sizeof(*cr));

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

    // Allocate arrays for facts
    cr->fact_pre = BOR_CALLOC_ARR(plan_arr_int_t, cr->fact_size);
    cr->fact_eff = BOR_CALLOC_ARR(plan_arr_int_t, cr->fact_size);

    // Allocated initial space for operators
    cr->op_alloc = op_size;
    cr->op_pre = BOR_ALLOC_ARR(plan_arr_int_t, cr->op_alloc);
    cr->op_eff = BOR_ALLOC_ARR(plan_arr_int_t, cr->op_alloc);
    cr->op_id  = BOR_ALLOC_ARR(int, cr->op_alloc);

    // Compute cross reference tables for operators
    crossRefOps(cr, op, op_size, var, var_size);

    // Adds operator reaching artificial goal
    addGoalOp(cr, goal);
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
