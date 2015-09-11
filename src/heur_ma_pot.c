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
#include "plan/search.h"
#include "plan/heur.h"
#include "plan/pot.h"

struct _agent_data_t {
    plan_pot_agent_t pot;
    int lp_private_var_size;
    int pot_requested;    /*!< True if potentials were requested before
                               they were computed. */
    double *potval;
    int potval_size;
};
typedef struct _agent_data_t agent_data_t;

struct _plan_heur_ma_pot_t {
    plan_heur_t heur;
    plan_pot_t pot;

    int pending;
    int agent_id;
    int agent_size;
    agent_data_t *agent_data;
    int *fact_range;
    int fact_range_size;
    int fact_range_lcm;
    int lp_var_size;

    int init_heur;
    unsigned flags;

    plan_state_t *state;
    plan_state_t *state2;
};
typedef struct _plan_heur_ma_pot_t plan_heur_ma_pot_t;
#define HEUR(parent) \
    bor_container_of((parent), plan_heur_ma_pot_t, heur)

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
static int heurHeur(const plan_heur_ma_pot_t *h,
                    plan_state_id_t state_id,
                    const plan_search_t *search);

/** Adds range to the array */
static void addFactRange(plan_heur_ma_pot_t *h, int range);
/** Request potentials from the master agent */
static void requestPot(plan_ma_comm_t *comm);
/** Request basic info from all slave agents */
static void requestInfo(plan_ma_comm_t *comm);
/** Response basic info to the master agent */
static void responseInfo(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm);
/** Process info received from slave agent */
static void processInfo(plan_heur_ma_pot_t *h, const plan_ma_msg_t *msg);
/** Process all info when they are retrieved from all slave agents */
static void processAllInfo(plan_heur_ma_pot_t *h);
/** Reqest private LPs from slave agents */
static void requestLP(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm);
/** Response private LPs to the master agent */
static void responseLP(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm,
                       const plan_ma_msg_t *msg);
/** Process all private LPs etc and compute potentials */
static void processLP(plan_heur_ma_pot_t *h);
/** Sends potentials to the specified agent.
 *  If potentials are not computed yet, adds agent to a pending list. */
static void sendPot(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm, int agent_id);
/** Sends potentials to the pending agents that already requested */
static void sendPotToPending(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm);
/** Reads potentials from the messages and prepares them for computing
 *  heuristic values */
static void setPot(plan_heur_ma_pot_t *h, const plan_ma_msg_t *msg);

plan_heur_t *planHeurMAPotNew(const plan_problem_t *p, unsigned flags)
{
    plan_heur_ma_pot_t *h;
    PLAN_STATE_STACK(state, p->state_pool->num_vars);

    h = BOR_ALLOC(plan_heur_ma_pot_t);
    bzero(h, sizeof(*h));
    h->flags = flags;
    _planHeurInit(&h->heur, heurDel, NULL, NULL);
    _planHeurMAInit(&h->heur, NULL, heurHeurNode, heurUpdate, heurRequest);

    planStatePoolGetState(p->state_pool, p->initial_state, &state);
    planPotInit(&h->pot, p->var, p->var_size, p->goal,
                p->op, p->op_size, &state, flags, PLAN_POT_MA);

    h->agent_id = p->agent_id;
    h->agent_size = p->num_agents;
    h->agent_data = BOR_CALLOC_ARR(agent_data_t, h->agent_size);
    h->init_heur = -1;

    h->state = planStateNew(p->state_pool->num_vars);
    h->state2 = planStateNew(p->state_pool->num_vars);

    return &h->heur;
}

static void heurDel(plan_heur_t *heur)
{
    plan_heur_ma_pot_t *h = HEUR(heur);
    int i;

    for (i = 0; i < h->agent_size; ++i){
        planPotAgentFree(&h->agent_data[i].pot);
        if (h->agent_data[i].potval != NULL)
            BOR_FREE(h->agent_data[i].potval);
    }
    if (h->agent_data)
        BOR_FREE(h->agent_data);
    if (h->fact_range)
        BOR_FREE(h->fact_range);

    planStateDel(h->state);
    planStateDel(h->state2);
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
    plan_heur_ma_pot_t *h = HEUR(heur);

    planStatePoolGetState(search->state_pool, state_id, h->state);

    if (h->pot.pot == NULL){
        if (comm->node_id == 0){
            requestInfo(comm);
            h->pending = comm->node_size - 1;
        }else{
            requestPot(comm);
        }
        return -1;
    }

    res->heur = heurHeur(h, state_id, search);
    return 0;
}

static int heurUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    plan_heur_ma_pot_t *h = HEUR(heur);
    int type, subtype, agent_id;

    type = planMAMsgType(msg);
    subtype = planMAMsgSubType(msg);
    agent_id = planMAMsgAgent(msg);

    if (type == PLAN_MA_MSG_HEUR
            && subtype == PLAN_MA_MSG_HEUR_POT_RESPONSE){
        setPot(h, msg);
        res->heur = h->init_heur;
        return 0;

    }else if (type == PLAN_MA_MSG_HEUR
                && subtype == PLAN_MA_MSG_HEUR_POT_INFO_RESPONSE){
        processInfo(h, msg);
        --h->pending;
        if (h->pending == 0){
            processAllInfo(h);
            requestLP(h, comm);
            h->pending = comm->node_size - 1;
        }

    }else if (type == PLAN_MA_MSG_HEUR
                && subtype == PLAN_MA_MSG_HEUR_POT_LP_RESPONSE){
        planMAMsgPotAgent(msg, &h->agent_data[agent_id].pot);
        --h->pending;
        if (h->pending == 0){
            processLP(h);
            sendPotToPending(h, comm);
            res->heur = h->init_heur;
            return 0;
        }

    }else{
        fprintf(stderr, "[%d] Unexpected update message: %x/%x\n",
                comm->node_id, type, subtype);
        fflush(stderr);
    }

    return -1;
}

static void heurRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
    plan_heur_ma_pot_t *h = HEUR(heur);
    int type, subtype, agent_id;

    type = planMAMsgType(msg);
    subtype = planMAMsgSubType(msg);
    agent_id = planMAMsgAgent(msg);

    if (type == PLAN_MA_MSG_HEUR && subtype == PLAN_MA_MSG_HEUR_POT_REQUEST){
        sendPot(h, comm, agent_id);

    }else if (type == PLAN_MA_MSG_HEUR
                && subtype == PLAN_MA_MSG_HEUR_POT_INFO_REQUEST){
        responseInfo(h, comm);

    }else if (type == PLAN_MA_MSG_HEUR
                && subtype == PLAN_MA_MSG_HEUR_POT_LP_REQUEST){
        responseLP(h, comm, msg);

    }else{
        fprintf(stderr, "[%d] Unexpected request message: %x/%x\n",
                comm->node_id, type, subtype);
        fflush(stderr);
    }
}

static void addFactRange(plan_heur_ma_pot_t *h, int range)
{
    int i;

    for (i = 0; i < h->fact_range_size; ++i){
        if (h->fact_range[i] == range)
            return;
    }

    ++h->fact_range_size;
    h->fact_range = BOR_REALLOC_ARR(h->fact_range, int, h->fact_range_size);
    h->fact_range[h->fact_range_size - 1] = range;
}

static void requestPot(plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;
    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_REQUEST,
                       comm->node_id);
    planMACommSendToNode(comm, 0, msg);
    planMAMsgDel(msg);
}

static void requestInfo(plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;
    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_INFO_REQUEST,
                       comm->node_id);
    planMACommSendToAll(comm, msg);
    planMAMsgDel(msg);
}

static void responseInfo(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;
    int i, size;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_INFO_RESPONSE,
                       comm->node_id);

    for (i = 0; i < h->pot.var_size; ++i){
        if (i != h->pot.ma_privacy_var){
            planMAMsgAddPotFactRange(msg, h->pot.var[i].range);
        }
    }
    // TODO: secure
    size = h->pot.lp_var_size - h->pot.lp_var_private;
    planMAMsgSetPotLPPrivateVarSize(msg, size);
    planMACommSendToNode(comm, 0, msg);
    planMAMsgDel(msg);
}

static void processInfo(plan_heur_ma_pot_t *h, const plan_ma_msg_t *msg)
{
    int *fact_range, fact_range_size;
    int i, agent_id;
    agent_data_t *d;

    agent_id = planMAMsgAgent(msg);
    d = h->agent_data + agent_id;

    d->lp_private_var_size = planMAMsgPotLPPrivateVarSize(msg);

    fact_range_size = planMAMsgPotFactRangeSize(msg);
    if (fact_range_size > 0){
        fact_range = BOR_ALLOC_ARR(int, fact_range_size);
        planMAMsgPotFactRange(msg, fact_range);
        for (i = 0; i < fact_range_size; ++i)
            addFactRange(h, fact_range[i]);
        BOR_FREE(fact_range);
    }
}

static int gcd(int x, int y)
{
    if (x == 0)
        return y;

    while (y != 0){
        if (x > y) {
            x = x - y;
        }else{
            y = y - x;
        }
    }

    return x;
}

static int lcm(int x, int y)
{
    return (x * y) / gcd(x, y);
}

static void processAllInfo(plan_heur_ma_pot_t *h)
{
    int i;

    // Compute LCM of all ranges
    h->fact_range_lcm = 1;
    for (i = 0; i < h->pot.var_size; ++i){
        if (i != h->pot.ma_privacy_var)
            h->fact_range_lcm = lcm(h->fact_range_lcm, h->pot.var[i].range);
    }
    for (i = 0; i < h->fact_range_size; ++i)
        h->fact_range_lcm = lcm(h->fact_range_lcm, h->fact_range[i]);

    // Compute overall number of LP variables
    h->lp_var_size = h->pot.lp_var_size;
    for (i = 0; i < h->agent_size; ++i){
        if (i != h->agent_id)
            h->lp_var_size += h->agent_data[i].lp_private_var_size;
    }
}

static void requestLP(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_LP_REQUEST,
                       comm->node_id);
    planMAStateSetMAMsg2(h->heur.ma_state, h->state, msg);
    planMAMsgSetPotFactRangeLCM(msg, h->fact_range_lcm);
    planMAMsgSetPotLPVarSize(msg, h->lp_var_size);
    planMACommSendToAll(comm, msg);
    planMAMsgDel(msg);
}

static void responseLP(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm,
                       const plan_ma_msg_t *msg)
{
    plan_pot_agent_t pot;
    PLAN_STATE_STACK(state, h->pot.var_size);
    plan_ma_msg_t *mout;

    // TODO: all-synt-states
    // TODO: secure
    planMAStateGetFromMAMsg(h->heur.ma_state, msg, &state);
    planPotAgentInit(&h->pot, &state, -1, &pot);

    mout = planMAMsgNew(PLAN_MA_MSG_HEUR,
                        PLAN_MA_MSG_HEUR_POT_LP_RESPONSE,
                        comm->node_id);
    planMAMsgSetPotAgent(mout, &pot);
    planMACommSendToNode(comm, 0, mout);
    planMAMsgDel(mout);
    planPotAgentFree(&pot);
}

static void processLP(plan_heur_ma_pot_t *h)
{
    // TODO: Set h->init_heur and h->pot.pot and h->agent_data[x].potval
}

static void sendPot(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm, int agent_id)
{
    if (h->init_heur == -1){
        // Potentials are not computed yet -- set agent as pending
        h->agent_data[agent_id].pot_requested = 1;
        return;
    }

    // TODO
}

static void sendPotToPending(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm)
{
    int i;

    for (i = 0; i < h->agent_size; ++i){
        if (i != h->agent_id && h->agent_data[i].pot_requested){
            sendPot(h, comm, i);
            h->agent_data[i].pot_requested = 0;
        }
    }
}

static void setPot(plan_heur_ma_pot_t *h, const plan_ma_msg_t *msg)
{
    // TODO: Set h->init_heur and h->pot.pot
}

static int heurHeur(const plan_heur_ma_pot_t *h,
                    plan_state_id_t state_id,
                    const plan_search_t *search)
{
    plan_state_space_t *state_space = (plan_state_space_t *)search->state_space;
    const plan_state_space_node_t *node, *parent_node;
    int heur, local_heur, parent_heur, parent_stored_heur;

    node = planStateSpaceNode(state_space, state_id);
    parent_node = planStateSpaceNode(state_space, node->parent_state_id);
    planStatePoolGetState(search->state_pool, node->parent_state_id, h->state2);

    local_heur = planPotStatePot(&h->pot, h->state);
    parent_heur = planPotStatePot(&h->pot, h->state2);
    parent_stored_heur = parent_node->heuristic;
    heur = local_heur + (parent_stored_heur - parent_heur);
    return heur;
}

