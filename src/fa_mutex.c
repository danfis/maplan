/***
 * maplan
 * -------
 * Copyright (c)2017 Daniel Fiser <danfis@danfis.cz>,
 * AI Center, Department of Computer Science,
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
#include "plan/fa_mutex.h"
#include "plan/fact_id.h"
#include "plan/lp.h"

void planFAMutexSetInit(plan_fa_mutex_set_t *ms)
{
    bzero(ms, sizeof(*ms));
}

void planFAMutexSetFree(plan_fa_mutex_set_t *ms)
{
    int i;

    for (i = 0; i < ms->size; ++i)
        planArrIntFree(ms->fa_mutex + i);
    if (ms->fa_mutex)
        BOR_FREE(ms->fa_mutex);
}

static int cmpFAMutex(const void *a, const void *b)
{
    const plan_arr_int_t *m1 = a;
    const plan_arr_int_t *m2 = b;
    int i;

    for (i = 0; i < m1->size && i < m2->size; ++i){
        if (m1->arr[i] != m2->arr[i])
            return m1->arr[i] - m2->arr[i];
    }
    return m1->size - m2->size;
}

void planFAMutexSetSort(plan_fa_mutex_set_t *ms)
{
    qsort(ms->fa_mutex, ms->size, sizeof(plan_arr_int_t), cmpFAMutex);
}

static plan_arr_int_t *planFAMutexNext(plan_fa_mutex_set_t *ms)
{
    if (ms->size == ms->alloc){
        if (ms->alloc == 0)
            ms->alloc = 2;
        ms->alloc *= 2;
        ms->fa_mutex = BOR_REALLOC_ARR(ms->fa_mutex, plan_arr_int_t,
                                       ms->alloc);
        bzero(ms->fa_mutex + ms->size,
              sizeof(plan_arr_int_t) * (ms->alloc - ms->size));
    }
    return ms->fa_mutex + ms->size++;
}

void planFAMutexSetClone(plan_fa_mutex_set_t *dst,
                         const plan_fa_mutex_set_t *src)
{
    const plan_arr_int_t *asrc;
    plan_arr_int_t *adst;
    int i;

    planFAMutexSetInit(dst);
    if (src->size == 0)
        return;

    dst->alloc = dst->size = src->size;
    dst->fa_mutex = BOR_CALLOC_ARR(plan_arr_int_t, dst->alloc);
    for (i = 0; i < src->size; ++i){
        asrc = src->fa_mutex + i;
        adst = dst->fa_mutex + i;
        adst->alloc = adst->size = asrc->size;
        adst->arr = BOR_ALLOC_ARR(int, adst->alloc);
        memcpy(adst->arr, asrc->arr, sizeof(int) * asrc->size);
    }
}

void planFAMutexAddFromVars(plan_fa_mutex_set_t *ms,
                            const plan_var_t *var, int var_size)
{
    plan_fact_id_t fact_id;
    plan_arr_int_t *mutex;
    int vi, val, fid;

    planFactIdInit(&fact_id, var, var_size, 0);
    for (vi = 0; vi < var_size; ++vi){
        if (var[vi].ma_privacy)
            continue;
        mutex = planFAMutexNext(ms);
        for (val = 0; val < var[vi].range; ++val){
            fid = planFactIdVar(&fact_id, vi, val);
            planArrIntAdd(mutex, fid);
        }
    }
    planFactIdFree(&fact_id);
}

static void setStateConstr(plan_lp_t *lp, int row,
                           const plan_fact_id_t *fact_id,
                           const plan_problem_t *p,
                           const plan_state_t *state)
{
    int i, fid;

    for (i = 0; i < p->var_size; ++i){
        if (i == p->ma_privacy_var)
            continue;
        fid = planFactIdVar(fact_id, i, planStateGet(state, i));
        planLPSetCoef(lp, 0, fid, 1.);
    }
    planLPSetRHS(lp, 0, 1., 'L');
}

static void setOpConstr(plan_lp_t *lp, int row,
                        const plan_fact_id_t *fact_id,
                        const plan_op_t *op)
{
    const plan_part_state_t *pre = op->pre;
    const plan_part_state_t *eff = op->eff;
    int fid, prei, effi;

    for (prei = effi = 0; prei < pre->vals_size && effi < eff->vals_size;){
        if (pre->vals[prei].var == eff->vals[effi].var){
            fid = planFactIdVar(fact_id, pre->vals[prei].var,
                                         pre->vals[prei].val);
            planLPSetCoef(lp, row, fid, -1.);

            fid = planFactIdVar(fact_id, eff->vals[effi].var,
                                         eff->vals[effi].val);
            planLPSetCoef(lp, row, fid, 1.);

            ++prei;
            ++effi;

        }else if (pre->vals[prei].var < eff->vals[effi].var){
            // Skip prevails
            ++prei;

        }else{
            fid = planFactIdVar(fact_id, eff->vals[effi].var,
                                         eff->vals[effi].val);
            planLPSetCoef(lp, row, fid, 1.);
            ++effi;
        }
    }
    for (; effi < eff->vals_size; ++effi){
        fid = planFactIdVar(fact_id, eff->vals[effi].var,
                                     eff->vals[effi].val);
        planLPSetCoef(lp, row, fid, 1.);
    }
    planLPSetRHS(lp, row, 0., 'L');
}

static void addFAMutexConstr(plan_lp_t *lp, int fact_size,
                             const plan_arr_int_t *m)
{
    double rhs = 1.;
    char sense = 'G';
    int i, mi, row;

    row = planLPNumRows(lp);
    planLPAddRows(lp, 1, &rhs, &sense);
    for (i = 0, mi = 0; i < fact_size; ++i){
        if (mi < m->size && m->arr[mi] == i){
            ++mi;
            continue;
        }
        planLPSetCoef(lp, row, i, 1.);
    }
}

static void addFAMutex(plan_fa_mutex_set_t *ms,
                       int fact_size, const double *obj)
{
    plan_arr_int_t *mutex;
    int i;

    mutex = planFAMutexNext(ms);
    for (i = 0; i < fact_size; ++i){
        if (obj[i] > 0.5)
            planArrIntAdd(mutex, i);
    }
}

void planFAMutexFind(const plan_problem_t *p, const plan_state_t *state,
                     plan_fa_mutex_set_t *ms)
{
    plan_fact_id_t fact_id;
    plan_lp_t *lp;
    int i;
    double val, *obj;

    planFactIdInit(&fact_id, p->var, p->var_size, 0);
    lp = planLPNew(p->op_size + 1, fact_id.fact_size, PLAN_LP_MAX);

    // Set objective and all variables as binary
    for (i = 0; i < fact_id.fact_size; ++i){
        planLPSetObj(lp, i, 1.);
        planLPSetVarBinary(lp, i);
    }

    // Initial state constraint
    setStateConstr(lp, 0, &fact_id, p, state);

    // Operator constaints
    for (i = 0; i < p->op_size; ++i)
        setOpConstr(lp, i + 1, &fact_id, p->op + i);

    // Add constraints for each input mutex
    for (i = 0; i < ms->size; ++i)
        addFAMutexConstr(lp, fact_id.fact_size, ms->fa_mutex + i);

    obj = BOR_ALLOC_ARR(double, fact_id.fact_size);
    while (planLPSolve(lp, &val, obj) == 0){
        addFAMutex(ms, fact_id.fact_size, obj);
        addFAMutexConstr(lp, fact_id.fact_size, ms->fa_mutex + ms->size - 1);
    }
    BOR_FREE(obj);
    planLPDel(lp);
    planFactIdFree(&fact_id);
}
