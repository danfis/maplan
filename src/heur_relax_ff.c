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
#include "heur_relax.h"
#include "pref_op_selector.h"

struct _plan_heur_relax_ff_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;
    const plan_op_t *base_op;
};
typedef struct _plan_heur_relax_ff_t plan_heur_relax_ff_t;

#define HEUR(parent) bor_container_of(parent, plan_heur_relax_ff_t, heur)

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_relax_ff_t *heur = HEUR(_heur);
    _planHeurFree(&heur->heur);
    planHeurRelaxFree(&heur->relax);
    BOR_FREE(heur);
}

static void prefOps(plan_heur_relax_ff_t *heur, plan_heur_res_t *res)
{
    plan_pref_op_selector_t sel;
    int i;

    // Select preferred operators as those that are in relaxed plan
    planPrefOpSelectorInit(&sel, res, heur->base_op);
    for (i = 0; i < heur->relax.cref.op_size; ++i){
        if (heur->relax.plan_op[i])
            planPrefOpSelectorSelect(&sel, i);
    }
    planPrefOpSelectorFinalize(&sel);
}

static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    plan_heur_relax_ff_t *heur = HEUR(_heur);
    int i;

    // Compute relaxation heuristic and relaxed plan
    planHeurRelax(&heur->relax, state);
    if (heur->relax.fact[heur->relax.cref.goal_id].value == PLAN_COST_MAX){
        res->heur = PLAN_HEUR_DEAD_END;
        return;
    }
    planHeurRelaxMarkPlan(&heur->relax);

    // Pick up the value
    res->heur = 0;
    for (i = 0; i < heur->relax.cref.op_size; ++i){
        if (heur->relax.plan_op[i])
            res->heur += heur->relax.op[i].cost;
    }

    // Select preferred operators if requested
    if (res->pref_op)
        prefOps(heur, res);
}

plan_heur_t *planHeurRelaxFFNew(const plan_problem_t *p, unsigned flags)
{
    plan_heur_relax_ff_t *heur;

    heur = BOR_ALLOC(plan_heur_relax_ff_t);
    heur->base_op = p->op;
    _planHeurInit(&heur->heur, heurDel, heurVal, NULL);
    planHeurRelaxInit(&heur->relax, PLAN_HEUR_RELAX_TYPE_ADD,
                      p->var, p->var_size, p->goal, p->op, p->op_size, flags);

    return &heur->heur;
}
