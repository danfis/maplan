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

struct _plan_heur_relax_add_max_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;
    const plan_op_t *base_op;
};
typedef struct _plan_heur_relax_add_max_t plan_heur_relax_add_max_t;

#define HEUR(parent) bor_container_of(parent, plan_heur_relax_add_max_t, heur)

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_relax_add_max_t *heur = HEUR(_heur);
    _planHeurFree(&heur->heur);
    planHeurRelaxFree(&heur->relax);
    BOR_FREE(heur);
}

static void prefOps(plan_heur_relax_add_max_t *heur, plan_heur_res_t *res)
{
    plan_pref_op_selector_t sel;
    int i;

    // Compute relaxed plan
    planHeurRelaxMarkPlan(&heur->relax);

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
    plan_heur_relax_add_max_t *heur = HEUR(_heur);

    // Compute relaxation heuristic
    planHeurRelax(&heur->relax, state);

    // Pick up the value
    res->heur = heur->relax.fact[heur->relax.cref.goal_id].value;
    if (res->heur == PLAN_COST_MAX)
        res->heur = PLAN_HEUR_DEAD_END;

    // Select preferred operators if requested
    if (res->pref_op)
        prefOps(heur, res);
}

static plan_heur_t *heurNew(const plan_problem_t *p,
                            int relax_op, unsigned flags)
{
    plan_heur_relax_add_max_t *heur;

    heur = BOR_ALLOC(plan_heur_relax_add_max_t);
    heur->base_op = p->op;

    _planHeurInit(&heur->heur, heurDel, heurVal, NULL);
    planHeurRelaxInit(&heur->relax, relax_op,
                      p->var, p->var_size, p->goal, p->op, p->op_size, flags);

    return &heur->heur;
}

plan_heur_t *planHeurRelaxAddNew(const plan_problem_t *p, unsigned flags)
{
    return heurNew(p, PLAN_HEUR_RELAX_TYPE_ADD, flags);
}

plan_heur_t *planHeurRelaxMaxNew(const plan_problem_t *p, unsigned flags)
{
    return heurNew(p, PLAN_HEUR_RELAX_TYPE_MAX, flags);
}

plan_heur_t *planHeurH2MaxNew(const plan_problem_t *p, unsigned flags)
{
    return heurNew(p, PLAN_HEUR_RELAX_TYPE_MAX, flags | PLAN_HEUR_H2);
}
