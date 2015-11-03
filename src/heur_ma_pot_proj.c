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
#include <boruvka/rand.h>
#include "plan/search.h"
#include "plan/heur.h"
#include "plan/pot.h"

#ifdef PLAN_LP

struct _plan_heur_ma_pot_proj_t {
    plan_heur_t heur;
    plan_pot_t pot;

    unsigned flags;
    int agent_id;             /*!< ID of the current agent */
    int agent_size;           /*!< Number of agents in cluster */
    int pending;              /*!< Number of pending requests */
};
typedef struct _plan_heur_ma_pot_proj_t plan_heur_ma_pot_proj_t;
#define HEUR(parent) \
    bor_container_of((parent), plan_heur_ma_pot_proj_t, heur)

static void heurDel(plan_heur_t *heur);
static int heurHeurNode(plan_heur_t *heur,
                        plan_ma_comm_t *comm,
                        plan_state_id_t state_id,
                        plan_search_t *search,
                        plan_heur_res_t *res);
static int heurUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res);
static void heurRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg);

plan_heur_t *planHeurMAPotProjNew(const plan_problem_t *p, unsigned flags)
{
    plan_heur_ma_pot_proj_t *h;
    PLAN_STATE_STACK(state, p->state_pool->num_vars);

    h = BOR_ALLOC(plan_heur_ma_pot_proj_t);
    bzero(h, sizeof(*h));
    h->flags = flags;
    _planHeurInit(&h->heur, heurDel, NULL, NULL);
    _planHeurMAInit(&h->heur, NULL, heurHeurNode, heurUpdate, heurRequest);

    planStatePoolGetState(p->state_pool, p->initial_state, &state);
    planPotInit(&h->pot, p->var, p->var_size, p->goal,
                p->proj_op, p->proj_op_size, &state, flags, 0);

    h->agent_id = p->agent_id;
    h->agent_size = p->num_agents;

    return &h->heur;
}

static void heurDel(plan_heur_t *heur)
{
    plan_heur_ma_pot_proj_t *h = HEUR(heur);
    // TODO
    BOR_FREE(h);
}

static int heurHeurNode(plan_heur_t *heur,
                        plan_ma_comm_t *comm,
                        plan_state_id_t state_id,
                        plan_search_t *search,
                        plan_heur_res_t *res)
{
    res->heur = 0;
    return 0;
}

static int heurUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    return 0;
}

static void heurRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
}

#else /* PLAN_LP */

plan_heur_t *planHeurMAPotProjNew(const plan_problem_t *p, unsigned flags)
{
    fprintf(stderr, "Error: Cannot create Potential heuristic because no"
                    " LP-solver was available during compilation!\n");
    fflush(stderr);
    return NULL;
}

#endif /* PLAN_LP */
