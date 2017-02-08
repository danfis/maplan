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
#include <plan/config.h>
#include <plan/heur.h>

#ifdef PLAN_LP
#include "plan/pot.h"

struct _plan_heur_potential_t {
    plan_heur_t heur;
    plan_pot_t pot;
    plan_lp_t *lp;
};
typedef struct _plan_heur_potential_t plan_heur_potential_t;
#define HEUR(parent) bor_container_of((parent), plan_heur_potential_t, heur)

static void heurPotentialDel(plan_heur_t *_heur);
static void heurPotential(plan_heur_t *_heur, const plan_state_t *state,
                          plan_heur_res_t *res);

plan_heur_t *planHeurPotentialNew(const plan_problem_t *p,
                                  const plan_state_t *init_state,
                                  unsigned flags)
{
    plan_heur_potential_t *heur;

    heur = BOR_ALLOC(plan_heur_potential_t);
    bzero(heur, sizeof(*heur));
    _planHeurInit(&heur->heur, heurPotentialDel, heurPotential, NULL);

    planPotInit(&heur->pot, p->var, p->var_size, p->goal,
                p->op, p->op_size, init_state, flags, 0);
    planPotCompute(&heur->pot);

    return &heur->heur;
}

static void heurPotentialDel(plan_heur_t *_heur)
{
    plan_heur_potential_t *h = HEUR(_heur);

    planPotFree(&h->pot);
    _planHeurFree(&h->heur);
    BOR_FREE(h);
}

static void heurPotential(plan_heur_t *_heur, const plan_state_t *state,
                          plan_heur_res_t *res)
{
    plan_heur_potential_t *h = HEUR(_heur);

    res->heur = planPotStatePot(&h->pot, state);
    res->heur = BOR_MAX(0, res->heur);
}

#else /* PLAN_LP */

plan_heur_t *planHeurPotentialNew(const plan_problem_t *p,
                                  const plan_state_t *init_state,
                                  unsigned flags)
{
    fprintf(stderr, "Fatal Error: heur-potential needs some LP library!\n");
    return NULL;
}
#endif /* PLAN_LP */
