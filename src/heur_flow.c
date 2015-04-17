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
#include <plan/heur.h>

/**
 * Main structure for Flow heuristic
 */
struct _plan_heur_flow_t {
    plan_heur_t heur;
};
typedef struct _plan_heur_flow_t plan_heur_flow_t;
#define HEUR(parent) \
    bor_container_of((parent), plan_heur_flow_t, heur)

static void heurFlowDel(plan_heur_t *_heur);
static void heurFlow(plan_heur_t *_heur, const plan_state_t *state,
                     plan_heur_res_t *res);

plan_heur_t *planHeurFlowNew(const plan_var_t *var, int var_size,
                            const plan_part_state_t *goal,
                            const plan_op_t *op, int op_size)
{
    plan_heur_flow_t *hflow;

    hflow = BOR_ALLOC(plan_heur_flow_t);
    _planHeurInit(&hflow->heur, heurFlowDel, heurFlow);
    return &hflow->heur;
}

static void heurFlowDel(plan_heur_t *_heur)
{
    plan_heur_flow_t *hflow = HEUR(_heur);

    _planHeurFree(&hflow->heur);
    BOR_FREE(hflow);
}

static void heurFlow(plan_heur_t *_heur, const plan_state_t *state,
                     plan_heur_res_t *res)
{
}
