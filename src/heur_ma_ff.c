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
#include <boruvka/rbtree_int.h>

#include "plan/heur.h"
#include "heur_relax.h"
#include "op_id_tr.h"

struct _plan_heur_ma_ff_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;

    plan_state_t *state; /*!< State for which the heuristic is computed */
    plan_arr_int_t relaxed_plan; /*!< Relaxed plan computed on all operators.
                                    It means that the operators are
                                    identified by its global ID */

    bor_rbtree_int_t *peer_op;          /*!< Set of operators that are
                                             requested from other peers */
    bor_rbtree_int_node_t *pre_peer_op; /*!< Pre-allocated node to
                                             remote_op set */
    int peer_op_size;                   /*!< Number of peer ops in .peer_op */

    const plan_op_t *base_op;
    int base_op_size;
    plan_op_id_tr_t op_id_tr;
    plan_op_t *private_op;
    int private_op_size;
    plan_op_id_tr_t private_op_id_tr;
};
typedef struct _plan_heur_ma_ff_t plan_heur_ma_ff_t;

#define HEUR(parent) bor_container_of(parent, plan_heur_ma_ff_t, heur)


/** Adds operator to the MA relaxed plan if not already there.
 *  Returns 0 if the operator was inserted, -1 otherwise */
static int maAddOpToRelaxedPlan(plan_heur_ma_ff_t *ma, int id, int cost);
/** Adds peer-operator to the register. Returns 0 if the operator was
 *  inserted or -1 if operator was already there or already in relaxed plan. */
static int maAddPeerOp(plan_heur_ma_ff_t *ma, int id);
/** Removes peer-operator from the registry */
static void maDelPeerOp(plan_heur_ma_ff_t *ma, int id);
/** Sends HEUR_REQUEST message to the peer */
static void maSendHeurRequest(const plan_heur_ma_ff_t *ff,
                              plan_ma_comm_t *comm, int peer_id,
                              const plan_state_t *state, int op_id);
/** Computes heuristic value from the relaxed plan */
static void maHeur(plan_heur_ma_ff_t *heur,
                   plan_heur_res_t *res);
/** Returns operator corresponding to its global ID or NULL if this node
 *  does not know this operator */
static const plan_op_t *maOpFromId(plan_heur_ma_ff_t *heur, int op_id);
static const plan_op_t *maPrivateOpFromId(plan_heur_ma_ff_t *heur, int op_id);
/** Performs local exploration from the initial state stored in .ma.state
 *  to the specified goal. */
static void maExploreLocal(plan_heur_ma_ff_t *heur,
                           plan_ma_comm_t *comm,
                           const plan_part_state_t *goal,
                           plan_heur_res_t *res);
/** Update relaxed plan by received local operator */
static void maUpdateLocalOp(plan_heur_ma_ff_t *heur,
                            plan_ma_comm_t *comm,
                            int op_id);


static void heurDel(plan_heur_t *_heur);
static int planHeurRelaxFFMA(plan_heur_t *heur,
                             plan_ma_comm_t *comm,
                             const plan_state_t *state,
                             plan_heur_res_t *res);
static int planHeurRelaxFFMAUpdate(plan_heur_t *heur,
                                   plan_ma_comm_t *comm,
                                   const plan_ma_msg_t *msg,
                                   plan_heur_res_t *res);
static void planHeurRelaxFFMARequest(plan_heur_t *heur,
                                     plan_ma_comm_t *comm,
                                     const plan_ma_msg_t *msg);

plan_heur_t *planHeurMARelaxFFNew(const plan_problem_t *prob)
{
    plan_heur_ma_ff_t *heur;

    heur = BOR_ALLOC(plan_heur_ma_ff_t);
    _planHeurInit(&heur->heur, heurDel, NULL, NULL);
    _planHeurMAInit(&heur->heur, planHeurRelaxFFMA, NULL,
                    planHeurRelaxFFMAUpdate, planHeurRelaxFFMARequest);

    planHeurRelaxInit(&heur->relax, PLAN_HEUR_RELAX_TYPE_ADD,
                      prob->var, prob->var_size,
                      prob->goal,
                      prob->proj_op, prob->proj_op_size, 0);

    heur->state = planStateNew(prob->var_size);

    heur->relaxed_plan.arr = NULL;
    heur->relaxed_plan.size = 0;

    heur->peer_op = borRBTreeIntNew();
    heur->pre_peer_op = BOR_ALLOC(bor_rbtree_int_node_t);
    heur->peer_op_size = 0;

    heur->base_op = prob->proj_op;
    heur->base_op_size = prob->proj_op_size;

    planOpIdTrInit(&heur->op_id_tr, prob->proj_op, prob->proj_op_size);

    planProblemCreatePrivateProjOps(prob->op, prob->op_size,
                                    prob->var, prob->var_size,
                                    &heur->private_op,
                                    &heur->private_op_size);
    planOpIdTrInit(&heur->private_op_id_tr, heur->private_op,
                  heur->private_op_size);
    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_ma_ff_t *heur = HEUR(_heur);
    bor_rbtree_int_node_t *n;

    _planHeurFree(&heur->heur);
    planHeurRelaxFree(&heur->relax);

    planStateDel(heur->state);
    if (heur->relaxed_plan.arr)
        BOR_FREE(heur->relaxed_plan.arr);

    while ((n = borRBTreeIntExtractMin(heur->peer_op)) != NULL){
        BOR_FREE(n);
    }
    borRBTreeIntDel(heur->peer_op);
    BOR_FREE(heur->pre_peer_op);
    planOpIdTrFree(&heur->op_id_tr);
    planProblemDestroyOps(heur->private_op, heur->private_op_size);
    planOpIdTrFree(&heur->private_op_id_tr);
    BOR_FREE(heur);
}


static int maAddOpToRelaxedPlan(plan_heur_ma_ff_t *ma, int id, int cost)
{
    int i;

    if (ma->relaxed_plan.size <= id){
        i = ma->relaxed_plan.size;
        ma->relaxed_plan.size = id + 1;
        ma->relaxed_plan.arr = BOR_REALLOC_ARR(ma->relaxed_plan.arr, int,
                                               ma->relaxed_plan.size);
        // Initialize the newly allocated memory
        for (; i < ma->relaxed_plan.size; ++i){
            ma->relaxed_plan.arr[i] = -1;
        }
    }

    if (ma->relaxed_plan.arr[id] == -1){
        ma->relaxed_plan.arr[id] = cost;
        return 0;
    }

    return -1;
}

static int maAddPeerOp(plan_heur_ma_ff_t *ma, int id)
{
    bor_rbtree_int_node_t *n;

    if (id < ma->relaxed_plan.size && ma->relaxed_plan.arr[id] >= 0)
        return -1;

    n = borRBTreeIntInsert(ma->peer_op, id, ma->pre_peer_op);
    if (n == NULL){
        // The ID was inserted, preallocate next peer_op
        ma->pre_peer_op = BOR_ALLOC(bor_rbtree_int_node_t);
        // Increase the counter
        ++ma->peer_op_size;
        return 0;
    }

    return -1;
}

static void maDelPeerOp(plan_heur_ma_ff_t *ma, int id)
{
    bor_rbtree_int_node_t *n;

    n = borRBTreeIntFind(ma->peer_op, id);
    if (n != NULL){
        borRBTreeIntRemove(ma->peer_op, n);
        --ma->peer_op_size;
        BOR_FREE(n);
    }
}

static void maSendHeurRequest(const plan_heur_ma_ff_t *heur,
                              plan_ma_comm_t *comm, int peer_id,
                              const plan_state_t *state, int op_id)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_FF_REQUEST,
                       comm->node_id);
    planMAStateSetMAMsg2(heur->heur.ma_state, state, msg);
    planMAMsgSetGoalOpId(msg, op_id);
    planMACommSendToNode(comm, peer_id, msg);
    planMAMsgDel(msg);
}

static void maHeur(plan_heur_ma_ff_t *heur,
                   plan_heur_res_t *res)
{
    int i;
    plan_cost_t hval = 0;

    for (i = 0; i < heur->relaxed_plan.size; ++i){
        if (heur->relaxed_plan.arr[i] > 0)
            hval += heur->relaxed_plan.arr[i];
    }

    res->heur = hval;
    // TODO: preferred operators
}

static const plan_op_t *maOpFromId(plan_heur_ma_ff_t *heur, int op_id)
{
    int id;

    id = planOpIdTrLoc(&heur->op_id_tr, op_id);
    if (id >= 0)
        return heur->base_op + id;
    return NULL;
}

static const plan_op_t *maPrivateOpFromId(plan_heur_ma_ff_t *heur, int op_id)
{
    int id;

    id = planOpIdTrLoc(&heur->private_op_id_tr, op_id);
    if (id >= 0)
        return heur->private_op + id;
    return NULL;
}

static void maExploreLocal(plan_heur_ma_ff_t *heur,
                           plan_ma_comm_t *comm,
                           const plan_part_state_t *goal,
                           plan_heur_res_t *res)
{
    PLAN_STATE_STACK(state, heur->relax.cref.fact_id.var_size);
    const plan_op_t *op;
    int i, global_id, owner;

    // Initialize initial state
    planStateCopy(&state, heur->state);

    // Compute heuristic from the initial state to the specified goal
    if (!goal){
        planHeurRelax(&heur->relax, &state);
        res->heur = heur->relax.fact[heur->relax.cref.goal_id].value;
    }else{
        res->heur = planHeurRelax2(&heur->relax, &state, goal);
    }
    if (res->heur == PLAN_HEUR_DEAD_END)
        return;

    // Mark relaxed plan
    if (!goal){
        planHeurRelaxMarkPlan(&heur->relax);
    }else{
        planHeurRelaxMarkPlan2(&heur->relax, goal);
    }

    for (i = 0; i < heur->relax.cref.op_size; ++i){
        if (!heur->relax.plan_op[i])
            continue;

        // Get the corresponding operator
        op = heur->base_op + i;
        global_id = op->global_id;
        owner = op->owner;

        if (owner != comm->node_id){
            // The operator is owned by remote peer.
            // Add it to the set of operators we are waiting for from
            // other peers
            if (maAddPeerOp(heur, global_id) == 0){
                // Send a request to the owner
                maSendHeurRequest(heur, comm, owner, heur->state, global_id);
            }
        }

        // Add the operator to the relaxed plan
        maAddOpToRelaxedPlan(heur, global_id, op->cost);
    }
}

static void maUpdateLocalOp(plan_heur_ma_ff_t *heur,
                            plan_ma_comm_t *comm,
                            int op_id)
{
    const plan_op_t *op;
    plan_heur_res_t res;

    op = maOpFromId(heur, op_id);
    if (op == NULL)
        return;

    if (maAddOpToRelaxedPlan(heur, op_id, op->cost) == 0){
        planHeurResInit(&res);
        maExploreLocal(heur, comm, op->pre, &res);
    }
}

static int planHeurRelaxFFMA(plan_heur_t *_heur,
                             plan_ma_comm_t *comm,
                             const plan_state_t *state,
                             plan_heur_res_t *res)
{
    plan_heur_ma_ff_t *heur = HEUR(_heur);
    int i;

    // Remember the state for which we want to compute heuristic
    planStateCopy(heur->state, state);

    // Reset relaxed plan
    for (i = 0; i < heur->relaxed_plan.size; ++i)
        heur->relaxed_plan.arr[i] = -1;

    // Explore projected state space
    maExploreLocal(heur, comm, NULL, res);
    if (res->heur == PLAN_HEUR_DEAD_END)
        return 0;

    // If we are waiting for responses from other peers, postpone actual
    // computation of the heuristic value.
    if (heur->peer_op_size > 0){
        return -1;
    }

    // Compute heuristic value
    maHeur(heur, res);
    return 0;
}

static int planHeurRelaxFFMAUpdate(plan_heur_t *_heur,
                                   plan_ma_comm_t *comm,
                                   const plan_ma_msg_t *msg,
                                   plan_heur_res_t *res)
{
    plan_heur_ma_ff_t *heur = HEUR(_heur);
    const plan_ma_msg_op_t *op;
    int i, len, op_id, cost, owner, from_agent;

    from_agent = planMAMsgAgent(msg);

    maDelPeerOp(heur, planMAMsgGoalOpId(msg));

    // Then explore all other peer-operators
    len = planMAMsgOpSize(msg);
    for (i = 0; i < len; ++i){
        op = planMAMsgOp(msg, i);
        op_id = planMAMsgOpOpId(op);
        cost = planMAMsgOpCost(op);
        owner = planMAMsgOpOwner(op);

        if (owner == comm->node_id){
            maUpdateLocalOp(heur, comm, op_id);

        }else{
            if (owner != from_agent && maAddPeerOp(heur, op_id) == 0){
                maSendHeurRequest(heur, comm, owner, heur->state, op_id);
            }
            maAddOpToRelaxedPlan(heur, op_id, cost);
        }
    }

    if (heur->peer_op_size > 0)
        return -1;

    maHeur(heur, res);
    return 0;
}

static void maSendEmptyResponse(plan_ma_comm_t *comm,
                                int peer_id, int op_id)
{
    plan_ma_msg_t *resp;

    resp = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_FF_RESPONSE,
                        comm->node_id);
    planMAMsgSetGoalOpId(resp, op_id);
    planMACommSendToNode(comm, peer_id, resp);
    planMAMsgDel(resp);
}

static void planHeurRelaxFFMARequest(plan_heur_t *_heur,
                                     plan_ma_comm_t *comm,
                                     const plan_ma_msg_t *msg)
{
    plan_heur_ma_ff_t *heur = HEUR(_heur);
    PLAN_STATE_STACK(state, heur->relax.cref.fact_id.var_size);
    plan_ma_msg_t *response;
    const plan_op_t *op;
    plan_ma_msg_op_t *msg_op;
    int i, op_id, agent_id;
    int global_id, owner;
    plan_cost_t h, cost;

    op_id = planMAMsgGoalOpId(msg);
    agent_id = planMAMsgAgent(msg);

    // Initialize initial state
    planMAStateGetFromMAMsg(heur->heur.ma_state, msg, &state);

    // Find target operator
    op = maPrivateOpFromId(heur, op_id);
    if (op == NULL){
        maSendEmptyResponse(comm, agent_id, op_id);
        return;
    }

    // Compute heuristic from the initial state to the precondition of the
    // requested operator.
    h = planHeurRelax2(&heur->relax, &state, op->pre);
    if (h == PLAN_HEUR_DEAD_END){
        maSendEmptyResponse(comm, agent_id, op_id);
        return;
    }
    planHeurRelaxMarkPlan2(&heur->relax, op->pre);

    // Now .relaxed_plan is filled, so write to the response and send it
    // back.
    response = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_FF_RESPONSE,
                            comm->node_id);
    planMAMsgSetGoalOpId(response, op_id);
    for (i = 0; i < heur->relax.cref.op_size; ++i){
        if (!heur->relax.plan_op[i])
            continue;

        op = heur->base_op + i;
        global_id = op->global_id;
        owner = op->owner;
        cost = op->cost;

        msg_op = planMAMsgAddOp(response);
        planMAMsgOpSetOpId(msg_op, global_id);
        planMAMsgOpSetCost(msg_op, cost);
        planMAMsgOpSetOwner(msg_op, owner);
    }
    planMACommSendToNode(comm, agent_id, response);

    planMAMsgDel(response);
}
