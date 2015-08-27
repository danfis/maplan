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

struct _prob_t {
    plan_pot_prob_t prob;
    int *state_var_id;
    int state_var_id_size;
};
typedef struct _prob_t prob_t;

static prob_t *probNew(int size);
static void probDel(prob_t *prob, int size);

struct _plan_heur_ma_potential_t {
    plan_heur_t heur;
    plan_pot_t pot;

    int pot_pending;
    prob_t *pot_prob;
    int pot_prob_size;
    int *pot_state_var_id;
    int pot_state_var_id_size;
};
typedef struct _plan_heur_ma_potential_t plan_heur_ma_potential_t;
#define HEUR(parent) \
    bor_container_of((parent), plan_heur_ma_potential_t, heur)

static void heurDel(plan_heur_t *heur);
static int heurHeur(plan_heur_t *heur, plan_ma_comm_t *comm,
                    const plan_state_t *state, plan_heur_res_t *res);
static int heurUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res);
static void heurRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg);

static void potStart(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm,
                     const plan_state_t *state);
static int potUpdate(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm,
                     const plan_ma_msg_t *msg);
static void potCompute(plan_heur_ma_potential_t *h);

plan_heur_t *planHeurMAPotentialNew(const plan_problem_t *p)
{
    plan_heur_ma_potential_t *h;

    h = BOR_ALLOC(plan_heur_ma_potential_t);
    _planHeurInit(&h->heur, heurDel, NULL, NULL);
    _planHeurMAInit(&h->heur, heurHeur, NULL, heurUpdate, heurRequest);
    planPotInit(&h->pot, p->var, p->var_size, p->goal, p->op, p->op_size, 0);
    h->pot_pending = -1;

    return &h->heur;
}

static void heurDel(plan_heur_t *heur)
{
    plan_heur_ma_potential_t *h = HEUR(heur);
    planPotFree(&h->pot);
    _planHeurFree(&h->heur);
    BOR_FREE(h);
}

static int heurHeur(plan_heur_t *heur, plan_ma_comm_t *comm,
                    const plan_state_t *state, plan_heur_res_t *res)
{
    plan_heur_ma_potential_t *h = HEUR(heur);

    if (h->pot.pot == NULL){
        potStart(h, comm, state);
        return -1;
    }

    // TODO
    return 0;
}

static int heurUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    plan_heur_ma_potential_t *h = HEUR(heur);

    if (potUpdate(h, comm, msg) == 0){
        potCompute(h);
        // TODO
        return 0;
    }

    return -1;
}

static void heurRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
    plan_heur_ma_potential_t *h = HEUR(heur);
    PLAN_STATE_STACK(state, h->pot.var_size);
    plan_ma_msg_t *mout;

    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || planMAMsgSubType(msg) != PLAN_MA_MSG_HEUR_POT_REQUEST){
        fprintf(stderr, "[%d] Unexpected message: %d/%d\n",
                comm->node_id, planMAMsgType(msg), planMAMsgSubType(msg));
        fflush(stderr);
        return;
    }

    mout = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_POT_RESPONSE,
                        comm->node_id);
    planMAStateGetFromMAMsg(h->heur.ma_state, msg, &state);
    planMAMsgSetPotProb(mout, &h->pot, &state);
    planMACommSendToNode(comm, planMAMsgAgent(msg), mout);
    planMAMsgDel(mout);
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

    h->pot_pending = comm->node_size - 1;
    h->pot_prob_size = h->pot_pending;
    h->pot_prob = probNew(h->pot_prob_size);
    h->pot_state_var_id = BOR_ALLOC_ARR(int, h->pot.var_size);
    h->pot_state_var_id_size = planPotToVarIds(&h->pot, state,
                                               h->pot_state_var_id);
}

static int potUpdate(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm,
                     const plan_ma_msg_t *msg)
{
    prob_t *prob;

    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || planMAMsgSubType(msg) != PLAN_MA_MSG_HEUR_POT_RESPONSE){
        fprintf(stderr, "[%d] Unexpected message: %d/%d\n",
                comm->node_id, planMAMsgType(msg), planMAMsgSubType(msg));
        fflush(stderr);
        return -1;
    }

    prob = h->pot_prob + h->pot_pending - 1;
    planMAMsgGetPotProb(msg, &prob->prob);
    prob->state_var_id_size = planMAMsgPotProbStateVarIdSize(msg);
    prob->state_var_id = BOR_ALLOC_ARR(int, prob->state_var_id_size);
    planMAMsgPotProbStateVarId(msg, prob->state_var_id);

    --h->pot_pending;
    if (h->pot_pending == 0)
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
        s = (*dst) + i;
        *d = *s;
        bzero(s, sizeof(*s));

        for (j = 0; j < d->coef_size; ++j){
            if (d->var_id[j] >= private_start_id)
                d->var_id[j] += offset;
            max_id = BOR_MAX(max_id, d->var_id[j]);
        }
    }

    return max_id + 1;
}

static int probMerge(plan_pot_prob_t *prob, plan_pot_prob_t *merge,
                     int private_start_id, int offset)
{
    int size = 0, siz;

    // TODO: prob->goal
    siz = probMergeConstr(&prob->op, &prob->op_size,
                          merge->op, merge->op_size,
                          private_start_id, offset);
    size = BOR_MAX(size, siz);

    siz = probMergeConstr(&prob->maxpot, &prob->maxpot_size,
                          merge->maxpot, merge->maxpot_size,
                          private_start_id, offset);
    size = BOR_MAX(size, siz);

    return size;
}

static void potCompute(plan_heur_ma_potential_t *h)
{
    plan_pot_prob_t prob;
    int i, size, var_size, offset;

    bzero(&prob, sizeof(prob));

    var_size = probMerge(&prob, &h->pot.prob, h->pot.lp_var_private, 0);
    for (i = 0; i < h->pot_prob_size; ++i){
        offset = var_size - h->pot.lp_var_private;
        size = probMerge(&prob, &h->pot_prob[i].prob,
                         h->pot.lp_var_private, offset);
        var_size = BOR_MAX(var_size, size);
    }

    probDel(h->pot_prob, h->pot_prob_size);
    h->pot_prob = NULL;
    h->pot_prob_size = 0;
}



static prob_t *probNew(int size)
{
    return BOR_CALLOC_ARR(prob_t, size);
}

static void probDel(prob_t *prob, int size)
{
    int i;

    if (!prob)
        return;

    for (i = 0; i < size; ++i){
        planPotProbFree(&prob[i].prob);
        if (prob[i].state_var_id)
            BOR_FREE(prob[i].state_var_id);
    }

    BOR_FREE(prob);
}
