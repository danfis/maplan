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

struct _plan_heur_ma_potential_t {
    plan_heur_t heur;
    plan_pot_t pot;

    int pot_pending;
    plan_pot_prob_t *pot_prob;
    int pot_prob_size;
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

static void potStart(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm,
                     const plan_state_t *state);
static int potUpdate(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm,
                     const plan_ma_msg_t *msg);
static int potCompute(plan_heur_ma_potential_t *h);

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
    h->pot_pending = -1;
    h->state = planStateNew(p->state_pool->num_vars);

    return &h->heur;
}

static void heurDel(plan_heur_t *heur)
{
    plan_heur_ma_potential_t *h = HEUR(heur);
    planStateDel(h->state);
    planPotFree(&h->pot);
    _planHeurFree(&h->heur);
    BOR_FREE(h);
}

static int stateHeur(const plan_heur_ma_potential_t *h,
                     const plan_state_t *state)
{
    int heur;

    heur = planPotStatePot(&h->pot, state);
    heur = BOR_MAX(heur, 0);
    return heur;
}

static int privateHeur(const plan_heur_ma_potential_t *h,
                       plan_state_space_t *state_space,
                       plan_state_pool_t *state_pool,
                       int parent_state_id)
{
    plan_state_space_node_t *node;
    PLAN_STATE_STACK(state, state_pool->num_vars);
    int heur;

    if (parent_state_id < 0)
        return 0;

    node = planStateSpaceNode(state_space, parent_state_id);
    planStatePoolGetState(state_pool, parent_state_id, &state);

    heur = node->heuristic - stateHeur(h, &state);
    return heur;
}

static int heurHeurNode(plan_heur_t *heur,
                        plan_ma_comm_t *comm,
                        plan_state_id_t state_id,
                        plan_search_t *search,
                        plan_heur_res_t *res)
{
    plan_heur_ma_potential_t *h = HEUR(heur);
    //plan_state_space_node_t *node;

    //node = planStateSpaceNode(search->state_space, state_id);
    planStatePoolGetState(search->state_pool, state_id, h->state);

    if (h->pot.pot == NULL){
        potStart(h, comm, h->state);
        return -1;
    }

    res->heur = stateHeur(h, h->state);
    return 0;
}

static int heurUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    plan_heur_ma_potential_t *h = HEUR(heur);

    if (potUpdate(h, comm, msg) == 0){
        res->heur = potCompute(h);
        fprintf(stderr, "[%d] Init H: %d\n", comm->node_id, res->heur);
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
    planMAMsgSetPotProb(mout, &h->pot);
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
    h->pot_prob = BOR_CALLOC_ARR(plan_pot_prob_t, h->pot_prob_size);
}

static int potUpdate(plan_heur_ma_potential_t *h, plan_ma_comm_t *comm,
                     const plan_ma_msg_t *msg)
{
    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || planMAMsgSubType(msg) != PLAN_MA_MSG_HEUR_POT_RESPONSE){
        fprintf(stderr, "[%d] Unexpected message: %d/%d\n",
                comm->node_id, planMAMsgType(msg), planMAMsgSubType(msg));
        fflush(stderr);
        return -1;
    }

    planMAMsgGetPotProb(msg, h->pot_prob + h->pot_pending - 1);

    --h->pot_pending;
    if (h->pot_pending == 0)
        return 0;
    return -1;
}

static int probMergeConstr(plan_pot_constr_t **dst, int *dst_size,
                           plan_pot_constr_t *src, int src_size,
                           int private_start_id, int offset,
                           int rewrite)
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

        if (rewrite){
            bzero(s, sizeof(*s));

        }else{
            d->var_id = BOR_ALLOC_ARR(int, d->coef_size);
            d->coef = BOR_ALLOC_ARR(int, d->coef_size);
            memcpy(d->var_id, s->var_id, sizeof(int) * d->coef_size);
            memcpy(d->coef, s->coef, sizeof(int) * d->coef_size);
        }

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
                     int private_start_id, int offset, int rewrite)
{
    int i, var_id, size = 0, siz;

    size = probMergeGoal(&prob->goal, &merge->goal, private_start_id, offset);

    siz = probMergeConstr(&prob->op, &prob->op_size,
                          merge->op, merge->op_size,
                          private_start_id, offset, rewrite);
    size = BOR_MAX(size, siz);

    siz = probMergeConstr(&prob->maxpot, &prob->maxpot_size,
                          merge->maxpot, merge->maxpot_size,
                          private_start_id, offset, rewrite);
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

static int potCompute(plan_heur_ma_potential_t *h)
{
    plan_pot_prob_t prob;
    int i, size, var_size, offset, heur = 0;
    double dheur;

    bzero(&prob, sizeof(prob));

    var_size = probMerge(&prob, &h->pot.prob, h->pot.lp_var_private, 0, 0);
    for (i = 0; i < h->pot_prob_size; ++i){
        offset = var_size - h->pot.lp_var_private;
        size = probMerge(&prob, h->pot_prob + i,
                         h->pot.lp_var_private, offset, 1);
        var_size = BOR_MAX(var_size, size);
    }

    h->pot.pot = BOR_ALLOC_ARR(double, prob.var_size);
    planPotCompute2(&prob, h->pot.pot);

    for (dheur = 0., i = 0; i < prob.var_size; ++i){
        if (prob.state_coef[i] > 0)
            dheur += h->pot.pot[i];
    }
    heur = dheur;
    heur = BOR_MAX(heur, 0);

    planPotProbFree(&prob);
    for (i = 0; i < h->pot_prob_size; ++i)
        planPotProbFree(h->pot_prob + i);
    BOR_FREE(h->pot_prob);
    h->pot_prob = NULL;
    h->pot_prob_size = 0;

    return heur;
}
