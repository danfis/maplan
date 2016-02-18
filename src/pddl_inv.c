/***
 * maplan
 * -------
 * Copyright (c)2016 Daniel Fiser <danfis@danfis.cz>,
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
#include <boruvka/hfunc.h>
#include "plan/pddl_inv.h"
#include "plan/lp.h"

static void addInvFromLPSol(bor_list_t *list, int fact_size, const double *sol);

void planPDDLInvFreeList(bor_list_t *list)
{
    bor_list_t *item, *tmp;
    plan_pddl_inv_t *inv;

    BOR_LIST_FOR_EACH_SAFE(list, item, tmp){
        inv = BOR_LIST_ENTRY(item, plan_pddl_inv_t, list);
        borListDel(&inv->list);
        if (inv->fact)
            BOR_FREE(inv->fact);
        BOR_FREE(inv);
    }
}

int planPDDLInvFind(const plan_pddl_ground_t *g, bor_list_t *inv)
{
    plan_lp_t *lp;
    const plan_pddl_ground_action_t *action;
    const plan_pddl_ground_facts_t *add, *del, *pre;
    int i, row, j, k, inv_size;
    double z, *sol, one = 1.;
    char sense = 'G';

    // Initialize linear program
    lp = planLPNew(g->action_pool.size + 1, g->fact_pool.size, PLAN_LP_MAX);

    // Set objective function and all variables to binary
    for (i = 0; i < g->fact_pool.size; ++i){
        planLPSetVarBinary(lp, i);
        planLPSetObj(lp, i, 1.);
    }

    // Add constraint for each action
    for (row = 0; row < g->action_pool.size; ++row){
        action = planPDDLGroundActionPoolGet(&g->action_pool, row);
        add = &action->eff_add;
        del = &action->eff_del;
        pre = &action->pre;

        for (j = 0; j < add->size; ++j)
            planLPSetCoef(lp, row, add->fact[j], 1.);

        // (facts in del and pre are always sorted)
        for (j = 0, k = 0; j < del->size && k < pre->size;){
            if (del->fact[j] == pre->fact[k]){
                planLPSetCoef(lp, row, del->fact[j], -1.);
                ++j;
                ++k;
            }else if (del->fact[j] < pre->fact[k]){
                ++j;
            }else{ // del->fact[j] > pre->fact[k]
                ++k;
            }
        }
        planLPSetRHS(lp, row, 0., 'L');

        // Refuse to infer invariants for problems with conditional effects
        if (action->cond_eff.size > 0){
            planLPDel(lp);
            return 0;
        }
    }

    // Add constraint for the initial state
    for (i = 0; i < g->init.size; ++i)
        planLPSetCoef(lp, row, g->init.fact[i], 1.);
    planLPSetRHS(lp, row, 1., 'E');

    // Find all invariants in cycle
    sol = BOR_ALLOC_ARR(double, g->fact_pool.size);
    inv_size = 0;
    while (planLPSolve(lp, &z, sol) == 0 && z > 1.5){
        // Add invariant to the output list
        addInvFromLPSol(inv, g->fact_pool.size, sol);
        ++inv_size;

        // Add constraint that removes the last invariant from the next
        // solution
        row = planLPNumRows(lp);
        planLPAddRows(lp, 1, &one, &sense);
        for (i = 0; i < g->fact_pool.size; ++i){
            if (sol[i] < 0.5)
                planLPSetCoef(lp, row, i, 1.);
        }
    }

    BOR_FREE(sol);
    planLPDel(lp);

    return inv_size;
}

static void addInvFromLPSol(bor_list_t *list, int fact_size, const double *sol)
{
    int i, ins, size;
    plan_pddl_inv_t *inv;

    size = 0;
    for (i = 0; i < fact_size; ++i){
        if (sol[i] > 0.5)
            ++size;
    }

    inv = BOR_ALLOC(plan_pddl_inv_t);
    inv->fact = BOR_ALLOC_ARR(int, size);
    inv->size = size;
    borListInit(&inv->list);

    for (i = 0, ins = 0; i < fact_size; ++i){
        if (sol[i] > 0.5)
            inv->fact[ins++] = i;
    }
    borListAppend(list, &inv->list);
}
