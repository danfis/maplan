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

static void addMutexConstr(plan_lp_t *lp,
                           const plan_fact_id_t *fact_id,
                           const plan_mutex_group_t *m)
{
    double rhs = 1.;
    char sense = 'G';
    int fid, i, mi, row;

    if (m->fact_size == 0)
        return;

    row = planLPNumRows(lp);
    planLPAddRows(lp, 1, &rhs, &sense);
    fid = planFactIdVar(fact_id, m->fact[0].var, m->fact[0].val);
    for (i = 0, mi = 0; i < fact_id->fact_size; ++i){
        if (fid == i){
            fid = fact_id->fact_size + 1;
            if (++mi < m->fact_size)
                fid = planFactIdVar(fact_id, m->fact[mi].var, m->fact[mi].val);
            continue;
        }
        planLPSetCoef(lp, row, i, 1.);
    }
}

static void addFAMutex(plan_mutex_group_set_t *ms,
                       const plan_fact_id_t *fact_id,
                       const double *obj)
{
    plan_mutex_group_t *m;
    plan_var_id_t var;
    plan_val_t val;
    int i;

    m = planMutexGroupSetAdd(ms);
    for (i = 0; i < fact_id->fact_size; ++i){
        if (obj[i] > 0.5){
            planFactIdVarFromFact(fact_id, i, &var, &val);
            planMutexGroupAdd(m, var, val);
        }
    }
}

void planFAMutexFind(const plan_problem_t *p, const plan_state_t *state,
                     plan_mutex_group_set_t *ms)
{
    plan_fact_id_t fact_id;
    plan_lp_t *lp;
    int i;
    double val, *obj;

    planFactIdInit(&fact_id, p->var, p->var_size, PLAN_FACT_ID_REVERSE_MAP);
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
    for (i = 0; i < ms->group_size; ++i)
        addMutexConstr(lp, &fact_id, ms->group + i);

    obj = BOR_ALLOC_ARR(double, fact_id.fact_size);
    while (planLPSolve(lp, &val, obj) == 0){
        addFAMutex(ms, &fact_id, obj);
        addMutexConstr(lp, &fact_id, ms->group + ms->group_size - 1);
    }
    BOR_FREE(obj);
    planLPDel(lp);
    planFactIdFree(&fact_id);
}
