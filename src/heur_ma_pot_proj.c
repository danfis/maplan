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
    int agent_id;            /*!< ID of the current agent */
    int agent_size;          /*!< Number of agents in cluster */
    int pending;             /*!< Number of pending requests */
    plan_state_t *state;     /*!< Pre-allocated state */
    plan_state_t *state_req; /*!< Pre-allocated state for requests */
    double heur_val;         /*!< Cached best heur value for the current state. */
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

static void requestStateHeur(plan_heur_ma_pot_proj_t *heur,
                             plan_ma_comm_t *comm,
                             const plan_state_t *state);
static void responseStateHeur(plan_heur_ma_pot_proj_t *heur,
                              plan_ma_comm_t *comm,
                              int target_agent,
                              double hval);

plan_heur_t *planHeurMAPotProjNew(const plan_problem_t *p, unsigned flags)
{
    plan_heur_ma_pot_proj_t *h;

    h = BOR_ALLOC(plan_heur_ma_pot_proj_t);
    bzero(h, sizeof(*h));
    h->flags = flags;
    _planHeurInit(&h->heur, heurDel, NULL, NULL);
    _planHeurMAInit(&h->heur, NULL, heurHeurNode, heurUpdate, heurRequest);

    h->state = planStateNew(p->state_pool->num_vars);
    h->state_req = planStateNew(p->state_pool->num_vars);
    planStatePoolGetState(p->state_pool, p->initial_state, h->state);
    planPotInit(&h->pot, p->var, p->var_size, p->goal,
                p->proj_op, p->proj_op_size, h->state, flags, 0);
    planPotCompute(&h->pot);

    h->agent_id = p->agent_id;
    h->agent_size = p->num_agents;

    return &h->heur;
}

static void heurDel(plan_heur_t *heur)
{
    plan_heur_ma_pot_proj_t *h = HEUR(heur);
    planStateDel(h->state);
    planStateDel(h->state_req);
    planPotFree(&h->pot);
    _planHeurFree(&h->heur);
    BOR_FREE(h);
}

static int heurHeurNode(plan_heur_t *heur,
                        plan_ma_comm_t *comm,
                        plan_state_id_t state_id,
                        plan_search_t *search,
                        plan_heur_res_t *res)
{
    plan_heur_ma_pot_proj_t *h = HEUR(heur);

    planStatePoolGetState(search->state_pool, state_id, h->state);
    h->heur_val = planPotStatePot(&h->pot, h->state);

    h->pending = h->agent_size - 1;
    requestStateHeur(h, comm, h->state);
    return -1;
}

static int heurUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    plan_heur_ma_pot_proj_t *h = HEUR(heur);
    double hval;

    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || planMAMsgSubType(msg) != PLAN_MA_MSG_HEUR_POT_RESPONSE){
        fprintf(stderr, "[%d] Heur-pot-proj Error: Invalid type of"
                        " response message!\n", comm->node_id);
        return -1;
    }

    hval = planMAMsgPotInitHeur(msg);
    h->heur_val = BOR_MAX(h->heur_val, hval);
    if (--h->pending == 0){
        res->heur = h->heur_val;
        res->heur = BOR_MAX(0, res->heur);
        return 0;
    }

    return -1;
}

static void heurRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
    plan_heur_ma_pot_proj_t *h = HEUR(heur);
    double hval;

    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || planMAMsgSubType(msg) != PLAN_MA_MSG_HEUR_POT_REQUEST){
        fprintf(stderr, "[%d] Heur-pot-proj Error: Invalid type of"
                        " request message!\n", comm->node_id);
        return;
    }

    planMAStateGetFromMAMsg(h->heur.ma_state, msg, h->state_req);
    hval = planPotStatePot(&h->pot, h->state_req);
    responseStateHeur(h, comm, planMAMsgAgent(msg), hval);
}

static void requestStateHeur(plan_heur_ma_pot_proj_t *heur,
                             plan_ma_comm_t *comm,
                             const plan_state_t *state)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_REQUEST,
                       planMACommId(comm));
    planMAStateSetMAMsg2(heur->heur.ma_state, state, msg);
    planMACommSendToAll(comm, msg);
    planMAMsgDel(msg);
}

static void responseStateHeur(plan_heur_ma_pot_proj_t *heur,
                              plan_ma_comm_t *comm,
                              int target_agent,
                              double hval)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_RESPONSE,
                       planMACommId(comm));
    planMAMsgSetPotInitHeur(msg, hval);
    planMACommSendToNode(comm, target_agent, msg);
    planMAMsgDel(msg);
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
