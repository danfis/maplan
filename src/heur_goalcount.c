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

struct _plan_heur_goalcount_t {
    plan_heur_t heur;
    const plan_part_state_t *goal;
};
typedef struct _plan_heur_goalcount_t plan_heur_goalcount_t;

#define HEUR_FROM_PARENT(parent) \
    bor_container_of((parent), plan_heur_goalcount_t, heur)

static void planHeurGoalCount(plan_heur_t *heur, const plan_state_t *state,
                              plan_heur_res_t *res);
static void planHeurGoalCountDel(plan_heur_t *h);

plan_heur_t *planHeurGoalCountNew(const plan_part_state_t *goal)
{
    plan_heur_goalcount_t *h;
    h = BOR_ALLOC(plan_heur_goalcount_t);
    _planHeurInit(&h->heur,
                  planHeurGoalCountDel,
                  planHeurGoalCount, NULL);
    h->goal = goal;
    return &h->heur;
}

static void planHeurGoalCountDel(plan_heur_t *_h)
{
    plan_heur_goalcount_t *h = HEUR_FROM_PARENT(_h);
    _planHeurFree(&h->heur);
    BOR_FREE(h);
}

static void planHeurGoalCount(plan_heur_t *_h, const plan_state_t *state,
                              plan_heur_res_t *res)
{
    plan_heur_goalcount_t *h = HEUR_FROM_PARENT(_h);
    int i;
    plan_var_id_t var;
    plan_val_t val;
    plan_cost_t heur;

    heur = PLAN_COST_ZERO;
    PLAN_PART_STATE_FOR_EACH(h->goal, i, var, val){
        if (val != planStateGet(state, var))
            ++heur;
    }

    res->heur = heur;
}
