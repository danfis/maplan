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

static void addInvFromLPSol(plan_pddl_inv_finder_t *invf,
                            const double *sol);
/** Creates a new invariant from the facts stored in binary array */
static plan_pddl_inv_t *invNew(const int *fact, int fact_size);
/** Frees allocated memory */
static void invDel(plan_pddl_inv_t *inv);

void planPDDLInvFinderInit(plan_pddl_inv_finder_t *invf,
                           const plan_pddl_ground_t *g)
{
    const plan_pddl_ground_action_t *action;
    const plan_pddl_fact_t *fact;
    int i, row, j, k, fact_id;

    bzero(invf, sizeof(*invf));
    borListInit(&invf->inv);
    invf->inv_size = 0;
    invf->g = g;
    invf->fact_size = g->fact_pool.size;

    // Initialize linear program
    invf->lp = planLPNew(g->action_pool.size + 1,
                         g->fact_pool.size, PLAN_LP_MAX);

    // Set objective function and all variables to binary
    for (i = 0; i < g->fact_pool.size; ++i){
        planLPSetVarBinary(invf->lp, i);
        planLPSetObj(invf->lp, i, 1.);
    }

    // Add constraint for each action
    for (row = 0; row < g->action_pool.size; ++row){
        action = planPDDLGroundActionPoolGet(&invf->g->action_pool, row);
        for (j = 0; j < action->eff_add.size; ++j)
            planLPSetCoef(invf->lp, row, action->eff_add.fact[j], 1.);
        for (j = 0; j < action->eff_del.size; ++j){
            for (k = 0; k < action->pre.size; ++k){
                if (action->eff_del.fact[j] == action->pre.fact[k]){
                    planLPSetCoef(invf->lp, row, action->eff_del.fact[j], -1.);
                    break;
                }
            }
        }
        planLPSetRHS(invf->lp, row, 0., 'L');
    }

    // Add constraint for the initial state
    for (i = 0; i < g->pddl->init_fact.size; ++i){
        fact_id = planPDDLFactPoolFind((plan_pddl_fact_pool_t *)&g->fact_pool,
                                       g->pddl->init_fact.fact + i);
        if (fact_id >= 0){
            planLPSetCoef(invf->lp, row, fact_id, 1.);
        }
    }
    planLPSetRHS(invf->lp, row, 1., 'E');

    /*
    for (i = 0; i < invf->g->fact_pool.size; ++i){
        fact = planPDDLFactPoolGet(&invf->g->fact_pool, i);
        fprintf(stderr, "f[%d] ", i);
        if (fact->neg)
            fprintf(stderr, "N");
        fprintf(stderr, "%s:", invf->g->pddl->predicate.pred[fact->pred].name);
        for (int j = 0; j < fact->arg_size; ++j)
            fprintf(stderr, " %s", invf->g->pddl->obj.obj[fact->arg[j]].name);
        fprintf(stderr, "\n");
    }
    for (i = 0; i < invf->g->action_pool.size; ++i){
        action = planPDDLGroundActionPoolGet(&invf->g->action_pool, i);
        fprintf(stderr, "a[%d]: %s\n", i, action->name);
    }
    */
}

void planPDDLInvFinderFree(plan_pddl_inv_finder_t *invf)
{
    plan_pddl_inv_t *inv;
    bor_list_t *item;

    if (invf->lp != NULL)
        planLPDel(invf->lp);

    while (!borListEmpty(&invf->inv)){
        item = borListNext(&invf->inv);
        borListDel(item);
        inv = BOR_LIST_ENTRY(item, plan_pddl_inv_t, list);
        invDel(inv);
    }
}

void planPDDLInvFinder(plan_pddl_inv_finder_t *invf)
{
    double z, *sol, one = 1.;
    char sense = 'G';
    int i, row;

    sol = BOR_ALLOC_ARR(double, invf->fact_size);

    while (planLPSolve(invf->lp, &z, sol) == 0 && z > 1.5){
        addInvFromLPSol(invf, sol);

        row = planLPNumRows(invf->lp);
        planLPAddRows(invf->lp, 1, &one, &sense);
        for (i = 0; i < invf->fact_size; ++i){
            if (sol[i] < 0.5){
                planLPSetCoef(invf->lp, row, i, 1.);
            }
        }
    }

    BOR_FREE(sol);
}


static void addInvFromLPSol(plan_pddl_inv_finder_t *invf,
                            const double *sol)
{
    int i, ins, size;
    plan_pddl_inv_t *inv;

    size = 0;
    for (i = 0; i < invf->fact_size; ++i){
        if (sol[i] > 0.5)
            ++size;
    }

    inv = BOR_ALLOC(plan_pddl_inv_t);
    inv->fact = BOR_ALLOC_ARR(int, size);
    inv->size = size;
    borListInit(&inv->list);

    for (i = 0, ins = 0; i < invf->fact_size; ++i){
        if (sol[i] > 0.5)
            inv->fact[ins++] = i;
    }
    borListAppend(&invf->inv, &inv->list);
    ++invf->inv_size;
}

/*** INV ***/
static plan_pddl_inv_t *invNew(const int *fact, int fact_size)
{
    plan_pddl_inv_t *inv;
    int i, size;

    for (size = 0, i = 0; i < fact_size; ++i){
        if (fact[i])
            ++size;
    }

    if (size == 0)
        return NULL;

    inv = BOR_ALLOC(plan_pddl_inv_t);
    inv->fact = BOR_ALLOC_ARR(int, size);
    inv->size = 0;
    borListInit(&inv->list);
    for (i = 0; i < fact_size; ++i){
        if (fact[i])
            inv->fact[inv->size++] = i;
    }

    return inv;
}

static void invDel(plan_pddl_inv_t *inv)
{
    if (inv->fact)
        BOR_FREE(inv->fact);
    BOR_FREE(inv);
}
/*** INV END ***/
