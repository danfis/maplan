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

struct _agent_data_t {
    plan_pot_prob_t prob; /*!< Other agent's pot-problem */
    int have_prob;        /*!< True if the problem was already received */
    int priv_var_from;    /*!< First ID where is mapped remote agent's
                               first private LP variable */
    int priv_var_to;      /*!< Last ID+1 of mapped last private LP variable */

    double *pot;          /*!< Potentials transformed to the remote
                               agent's problem. */
    int pot_size;         /*!< Length of .pot[] */
    int pot_requested;    /*!< True if potentials were requested before
                               they were computed. */
};
typedef struct _agent_data_t agent_data_t;

struct _plan_heur_ma_potential_t {
    plan_heur_t heur;
    plan_pot_t pot;

    int agent_id;
    int agent_size;
    agent_data_t *agent_data;
    int init_heur;

    plan_state_t *state;
};
typedef struct _plan_heur_ma_potential_t plan_heur_ma_potential_t;
#define HEUR(parent) \
    bor_container_of((parent), plan_heur_ma_potential_t, heur)

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

static void sendResults(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm,
                        int agent_id)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_RESULTS_RESPONSE,
                       comm->node_id);
    planMAMsgSetPotPot(msg, h->agent_data[agent_id].pot,
                       h->agent_data[agent_id].pot_size);
    planMAMsgSetPotInitState(msg, h->init_heur);
    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);

    h->agent_data[agent_id].pot_requested = 0;
}

static void potStart(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm,
                     const plan_state_t *state);
static void potRequestPot(plan_heur_ma_potential_t *h,
                          plan_ma_comm_t *comm);
static int potUpdate(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm,
                     const plan_ma_msg_t *msg);
static int potCompute(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm);

plan_heur_t *planHeurMAPotentialNew(const plan_problem_t *p)
{
    plan_heur_ma_potential_t *h;
    PLAN_STATE_STACK(state, p->state_pool->num_vars);

    h = BOR_ALLOC(plan_heur_ma_potential_t);
    bzero(h, sizeof(*h));
    _planHeurInit(&h->heur, heurDel, NULL, NULL);
    _planHeurMAInit(&h->heur, NULL, heurHeurNode, heurUpdate, heurRequest);

    planStatePoolGetState(p->state_pool, p->initial_state, &state);
    planPotInit(&h->pot, p->var, p->var_size, p->goal,
                p->op, p->op_size, &state, 0);

    h->agent_id = p->agent_id;
    h->agent_size = p->num_agents;
    h->agent_data = BOR_CALLOC_ARR(agent_data_t, h->agent_size);
    h->init_heur = -1;

    h->state = planStateNew(p->state_pool->num_vars);

    return &h->heur;
}

static void heurDel(plan_heur_t *heur)
{
    plan_heur_ma_potential_t *h = HEUR(heur);
    int i;

    for (i = 0; i < h->agent_size; ++i){
        planPotProbFree(&h->agent_data[i].prob);
        if (h->agent_data[i].pot != NULL)
            BOR_FREE(h->agent_data[i].pot);
    }
    if (h->agent_data)
        BOR_FREE(h->agent_data);

    planStateDel(h->state);
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
    plan_heur_ma_potential_t *h = HEUR(heur);

    planStatePoolGetState(search->state_pool, state_id, h->state);

    if (h->pot.pot == NULL){
        if (comm->node_id == 0){
            potStart(h, comm, h->state);
        }else{
            potRequestPot(h, comm);
        }
        return -1;
    }

    res->heur = planPotStatePot(&h->pot, h->state);
    return 0;
}

static int heurUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    plan_heur_ma_potential_t *h = HEUR(heur);
    int type, subtype, size;

    type = planMAMsgType(msg);
    subtype = planMAMsgSubType(msg);

    if (type == PLAN_MA_MSG_HEUR
            && subtype == PLAN_MA_MSG_HEUR_POT_RESPONSE){
        if (potUpdate(h, comm, msg) == 0){
            res->heur = potCompute(h, comm);
            return 0;
        }

        return -1;
    }

    if (type == PLAN_MA_MSG_HEUR
            && subtype == PLAN_MA_MSG_HEUR_POT_RESULTS_RESPONSE){
        size = planMAMsgPotPotSize(msg);
        h->pot.pot = BOR_ALLOC_ARR(double, size);
        planMAMsgPotPot(msg, h->pot.pot);
        res->heur = planMAMsgPotInitState(msg);
        return 0;
    }

    fprintf(stderr, "[%d] Unexpected update message: %x/%x\n",
            comm->node_id, type, subtype);
    fflush(stderr);
    return -1;
}

static void heurRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
    plan_heur_ma_potential_t *h = HEUR(heur);
    PLAN_STATE_STACK(state, h->pot.var_size);
    plan_ma_msg_t *mout;
    int type, subtype, agent_id;

    type = planMAMsgType(msg);
    subtype = planMAMsgSubType(msg);

    if (type == PLAN_MA_MSG_HEUR && subtype == PLAN_MA_MSG_HEUR_POT_REQUEST){
        mout = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_POT_RESPONSE,
                            comm->node_id);
        planMAStateGetFromMAMsg(h->heur.ma_state, msg, &state);
        planMAMsgSetPotProb(mout, &h->pot);
        planMACommSendToNode(comm, planMAMsgAgent(msg), mout);
        planMAMsgDel(mout);

    }else if (type == PLAN_MA_MSG_HEUR
                && subtype == PLAN_MA_MSG_HEUR_POT_RESULTS_REQUEST){
        agent_id = planMAMsgAgent(msg);
        if (h->init_heur >= 0){
            sendResults(h, comm, agent_id);
        }else{
            h->agent_data[agent_id].pot_requested = 1;
        }

    }else{
        fprintf(stderr, "[%d] Unexpected request message: %x/%x\n",
                comm->node_id, type, subtype);
        fflush(stderr);
    }
}

static void potStart(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm,
                     const plan_state_t *state)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_POT_REQUEST,
                       comm->node_id);
    planMAStateSetMAMsg2(h->heur.ma_state, state, msg);
    planMACommSendToAll(comm, msg);
    planMAMsgDel(msg);
}

static void potRequestPot(plan_heur_ma_potential_t *h,
                          plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;
    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_RESULTS_REQUEST,
                       comm->node_id);
    planMACommSendToNode(comm, 0, msg);
    planMAMsgDel(msg);
}

static int potUpdate(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm,
                     const plan_ma_msg_t *msg)
{
    int agent_id, i, cnt;

    agent_id = planMAMsgAgent(msg);
    planMAMsgGetPotProb(msg, &h->agent_data[agent_id].prob);
    h->agent_data[agent_id].have_prob = 1;

    for (cnt = 0, i = 0; i < h->agent_size; ++i)
        cnt += h->agent_data[i].have_prob;

    if (cnt == h->agent_size - 1)
        return 0;
    return -1;
}

static int probMergeConstr(plan_pot_constr_t **dst, int *dst_size,
                           plan_pot_constr_t *src, int src_size,
                           int private_start_id, int offset)
{
    plan_pot_constr_t *d, *s;
    int i, j, from, max_id = 0;

    from = *dst_size;
    *dst_size += src_size;
    *dst = BOR_REALLOC_ARR(*dst, plan_pot_constr_t, *dst_size);
    for (i = 0; i < src_size; ++i){
        d = (*dst) + i + from;
        s = src + i;

        *d = *s;
        d->var_id = BOR_ALLOC_ARR(int, d->coef_size);
        d->coef = BOR_ALLOC_ARR(int, d->coef_size);
        memcpy(d->var_id, s->var_id, sizeof(int) * d->coef_size);
        memcpy(d->coef, s->coef, sizeof(int) * d->coef_size);

        for (j = 0; j < d->coef_size; ++j){
            if (d->var_id[j] >= private_start_id)
                d->var_id[j] += offset;
            max_id = BOR_MAX(max_id, d->var_id[j]);
        }
    }

    return max_id + 1;
}

static int probMergeGoal(plan_pot_constr_t *dst,
                         const plan_pot_constr_t *src,
                         int private_start_id, int offset)
{
    int i, j, max_id = 0, var_id;

    for (i = 0; i < src->coef_size; ++i){
        var_id = src->var_id[i];
        if (var_id >= private_start_id)
            var_id += offset;

        max_id = BOR_MAX(max_id, var_id);

        for (j = 0; j < dst->coef_size; ++j){
            if (var_id == dst->var_id[j])
                break;
        }

        if (j == dst->coef_size){
            ++dst->coef_size;
            dst->coef = BOR_REALLOC_ARR(dst->coef, int, dst->coef_size);
            dst->var_id = BOR_REALLOC_ARR(dst->var_id, int, dst->coef_size);
            dst->var_id[dst->coef_size - 1] = var_id;
            dst->coef[dst->coef_size - 1] = src->coef[i];
        }
    }

    for (j = 0; j < dst->coef_size; ++j)
        max_id = BOR_MAX(max_id, dst->var_id[j]);

    return max_id + 1;
}

static int probMerge(plan_pot_prob_t *prob, plan_pot_prob_t *merge,
                     int private_start_id, int offset)
{
    int i, var_id, size = 0, siz;

    size = probMergeGoal(&prob->goal, &merge->goal, private_start_id, offset);

    siz = probMergeConstr(&prob->op, &prob->op_size,
                          merge->op, merge->op_size,
                          private_start_id, offset);
    size = BOR_MAX(size, siz);

    siz = probMergeConstr(&prob->maxpot, &prob->maxpot_size,
                          merge->maxpot, merge->maxpot_size,
                          private_start_id, offset);
    size = BOR_MAX(size, siz);

    if (prob->var_size < size){
        siz = prob->var_size;
        prob->var_size = size;
        prob->state_coef = BOR_REALLOC_ARR(prob->state_coef, int,
                                           prob->var_size);
        bzero(prob->state_coef + siz, sizeof(int) * (size - siz));
    }

    for (i = 0; i < merge->var_size; ++i){
        var_id = i;
        if (var_id >= private_start_id)
            var_id += offset;
        if (merge->state_coef[i] == 1)
            prob->state_coef[var_id] = 1;
    }

    return size;
}

static void potSetAgentPot(const plan_heur_ma_potential_t *h,
                           agent_data_t *data)
{
    int size, i;

    data->pot_size  = h->pot.lp_var_private;
    data->pot_size += data->priv_var_to - data->priv_var_from;
    data->pot = BOR_ALLOC_ARR(double, data->pot_size);

    // Copy public potentials -- these are common for all agents
    for (i = 0; i < h->pot.lp_var_private; ++i)
        data->pot[i] = h->pot.pot[i];

    // Copy private parts
    size = h->pot.lp_var_private;
    for (i = data->priv_var_from; i < data->priv_var_to; ++i)
        data->pot[size++] = h->pot.pot[i];
}

static int potCompute(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm)
{
    plan_pot_prob_t prob;
    int i, size, var_size, offset;
    double dheur;

    bzero(&prob, sizeof(prob));

    // Merge all problems
    var_size = probMerge(&prob, &h->pot.prob, h->pot.lp_var_private, 0);
    for (i = 0; i < h->agent_size; ++i){
        if (i == h->agent_id)
            continue;

        offset = var_size - h->pot.lp_var_private;
        size = probMerge(&prob, &h->agent_data[i].prob,
                         h->pot.lp_var_private, offset);
        var_size = BOR_MAX(var_size, size);

        h->agent_data[i].priv_var_from = h->pot.lp_var_private + offset;
        h->agent_data[i].priv_var_from = BOR_MAX(h->agent_data[i].priv_var_from,
                                                 h->pot.lp_var_private);
        h->agent_data[i].priv_var_to = var_size;
    }

    // Compute LP program
    h->pot.pot = BOR_ALLOC_ARR(double, prob.var_size);
    planPotCompute2(&prob, h->pot.pot);

    // Set potentials for all agents
    for (i = 0; i < h->agent_size; ++i){
        if (i == h->agent_id)
            continue;

        potSetAgentPot(h, h->agent_data + i);
    }

    for (dheur = 0., i = 0; i < prob.var_size; ++i){
        if (prob.state_coef[i] > 0)
            dheur += h->pot.pot[i];
    }
    h->init_heur = dheur;
    h->init_heur = BOR_MAX(h->init_heur, 0);

    planPotProbFree(&prob);

    // Response to all pending requests
    for (i = 0; i < h->agent_size; ++i){
        if (i != h->agent_id && h->agent_data[i].pot_requested)
            sendResults(h, comm, i);
    }

    return h->init_heur;
}
