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
#include "plan/prio_queue.h"
#include "heur_relax.h"
#include "op_id_tr.h"

struct _private_t {
    plan_heur_relax_t relax;
    plan_op_id_tr_t op_id_tr;
    int *fake_pre; /*!< Mapping between op_id and fake precodition ID */
};
typedef struct _private_t private_t;

struct _plan_heur_ma_max_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;
    plan_op_id_tr_t op_id_tr;

    int agent_id;
    int agent_size;            /*!< Number of agents in cluster */
    int *agent_changed;        /*!< True/False for each agent signaling
                                    whether an operator changed */
    int cur_agent_id;          /*!< ID of the agent from which a response
                                    is expected. */
    plan_arr_int_t *agent_op;  /*!< Operators owned by a corresponding agent */
    plan_state_t *init_state;  /*!< Stored actual state for which the
                                    heuristic is computed */
    plan_arr_int_t *op_by_owner; /*!< List of operators divided by an owner */
    plan_heur_relax_op_t *old_op;

    private_t private;
};
typedef struct _plan_heur_ma_max_t plan_heur_ma_max_t;

#define HEUR(parent) \
    bor_container_of(parent, plan_heur_ma_max_t, heur)

static void heurDel(plan_heur_t *_heur);
static int heurMAMax(plan_heur_t *heur, plan_ma_comm_t *comm,
                     const plan_state_t *state, plan_heur_res_t *res);
static int heurMAMaxUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                           const plan_ma_msg_t *msg, plan_heur_res_t *res);
static void heurMAMaxRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg);

static void privateInit(private_t *private, const plan_problem_t *prob,
                        unsigned flags)
{
    plan_op_t *op;
    int op_size;
    int i;

    planProblemCreatePrivateProjOps(prob->op, prob->op_size,
                                    prob->var, prob->var_size,
                                    &op, &op_size);
    planHeurRelaxInit(&private->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      prob->var, prob->var_size, prob->goal, op, op_size,
                      flags);

    // Add fake precondition to public operators.
    // The fake precondition will be used for received values.
    private->fake_pre = BOR_CALLOC_ARR(int, op_size);
    for (i = 0; i < op_size; ++i){
        if (!op[i].is_private)
            private->fake_pre[i] = planHeurRelaxAddFakePre(&private->relax, i);
    }

    planOpIdTrInit(&private->op_id_tr, op, op_size);
    planProblemDestroyOps(op, op_size);
}

static void privateFree(private_t *private)
{
    planHeurRelaxFree(&private->relax);
    if (private->fake_pre)
        BOR_FREE(private->fake_pre);
    planOpIdTrFree(&private->op_id_tr);
}

static int agentSize(const plan_op_t *op, int op_size)
{
    int agent_size = 0;
    int i;

    for (i = 0; i < op_size; ++i){
        agent_size = BOR_MAX(agent_size, op[i].owner + 1);
    }

    return agent_size;
}

static void initAgentOp(plan_heur_ma_max_t *heur,
                        const plan_op_t *op, int op_size)
{
    int i;

    heur->agent_op = BOR_CALLOC_ARR(plan_arr_int_t, heur->agent_size);
    for (i = 0; i < op_size; ++i)
        planArrIntAdd(heur->agent_op + op[i].owner, i);
}

static void initOpByOwner(plan_heur_ma_max_t *heur,
                          const plan_problem_t *prob)
{
    int i, op_id, owner;

    heur->op_by_owner = BOR_CALLOC_ARR(plan_arr_int_t, heur->agent_size);
    for (i = 0; i < heur->relax.cref.op_size; ++i){
        op_id = heur->relax.cref.op_id[i];
        if (op_id > 0){
            owner = prob->proj_op[op_id].owner;
            planArrIntAdd(heur->op_by_owner + owner, i);
        }
    }
}

plan_heur_t *planHeurMARelaxMaxNew(const plan_problem_t *prob,
                                   unsigned flags)
{
    plan_heur_ma_max_t *heur;

    heur = BOR_ALLOC(plan_heur_ma_max_t);
    _planHeurInit(&heur->heur, heurDel, NULL, NULL);
    _planHeurMAInit(&heur->heur, heurMAMax, NULL, heurMAMaxUpdate, heurMAMaxRequest);
    planHeurRelaxInit(&heur->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      prob->var, prob->var_size, prob->goal,
                      prob->proj_op, prob->proj_op_size, flags);
    planOpIdTrInit(&heur->op_id_tr, prob->proj_op, prob->proj_op_size);

    heur->agent_size = agentSize(prob->proj_op, prob->proj_op_size);
    heur->agent_changed = BOR_CALLOC_ARR(int, heur->agent_size);
    initAgentOp(heur, prob->proj_op, prob->proj_op_size);
    initOpByOwner(heur, prob);
    heur->old_op = BOR_ALLOC_ARR(plan_heur_relax_op_t,
                                 heur->relax.cref.op_size);

    heur->init_state = planStateNew(prob->var_size);

    privateInit(&heur->private, prob, flags);

    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    int i;
    plan_heur_ma_max_t *heur = HEUR(_heur);

    BOR_FREE(heur->agent_changed);
    for (i = 0; i < heur->agent_size; ++i)
        planArrIntFree(heur->agent_op + i);
    BOR_FREE(heur->agent_op);
    BOR_FREE(heur->old_op);

    planStateDel(heur->init_state);
    for (i = 0; i < heur->agent_size; ++i)
        planArrIntFree(heur->op_by_owner + i);
    BOR_FREE(heur->op_by_owner);

    privateFree(&heur->private);

    planOpIdTrFree(&heur->op_id_tr);
    planHeurRelaxFree(&heur->relax);
    _planHeurFree(&heur->heur);
    BOR_FREE(heur);
}


static void setInitState(plan_heur_ma_max_t *heur, const plan_state_t *state)
{
    planStateCopy(heur->init_state, state);
}

static void sendRequest(plan_heur_ma_max_t *heur, plan_ma_comm_t *comm,
                        int agent_id)
{
    plan_ma_msg_t *msg;
    plan_arr_int_t *oparr;
    plan_ma_msg_op_t *msg_op;
    int i, op_id, value;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_MAX_REQUEST,
                       planMACommId(comm));
    planMAStateSetMAMsg2(heur->heur.ma_state, heur->init_state, msg);

    oparr = heur->agent_op + agent_id;
    for (i = 0; i < oparr->size; ++i){
        op_id = planOpIdTrGlob(&heur->op_id_tr, oparr->arr[i]);
        value = heur->relax.op[oparr->arr[i]].value;
        msg_op = planMAMsgAddOp(msg);
        planMAMsgOpSetOpId(msg_op, op_id);
        planMAMsgOpSetValue(msg_op, value);
    }
    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static int heurMAMax(plan_heur_t *_heur, plan_ma_comm_t *comm,
                     const plan_state_t *state, plan_heur_res_t *res)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);
    int i;

    // Store state for which we are evaluating heuristics
    setInitState(heur, state);

    // Every agent must be requested at least once, so make sure of that.
    for (i = 0; i < heur->agent_size; ++i){
        heur->agent_changed[i] = 1;
        if (i == planMACommId(comm))
            heur->agent_changed[i] = 0;
    }
    heur->cur_agent_id = (planMACommId(comm) + 1) % heur->agent_size;
    heur->agent_id = comm->node_id;

    // h^max heuristics for all facts and operators reachable from the
    // state
    planHeurRelaxFull(&heur->relax, state);

    // Send request to next agent
    sendRequest(heur, comm, heur->cur_agent_id);

    return -1;
}


static int needsUpdate(const plan_heur_ma_max_t *heur)
{
    int i, val = 0;
    for (i = 0; i < heur->agent_size; ++i)
        val += heur->agent_changed[i];
    return val;
}

static void updateRelax(plan_heur_ma_max_t *heur,
                        const int *update_op, int update_op_size)
{
    int i, j, op_id, size;
    plan_arr_int_t *oparr;

    // Remember old values of operators
    size = sizeof(plan_heur_relax_op_t) * heur->relax.cref.op_size;
    memcpy(heur->old_op, heur->relax.op, size);

    // Update h^max heuristic using changed operators
    planHeurRelaxUpdateMaxFull(&heur->relax, update_op, update_op_size);

    // Find out which agent has changed operator
    for (i = 0; i < heur->agent_size; ++i){
        if (i == heur->agent_id || heur->agent_changed[i])
            continue;

        oparr = heur->op_by_owner + i;
        for (j = 0; j < oparr->size; ++j){
            op_id = oparr->arr[j];
            if (heur->old_op[op_id].value != heur->relax.op[op_id].value){
                heur->agent_changed[i] = 1;
                break;
            }
        }
    }
}

static void updateHMax(plan_heur_ma_max_t *heur, const plan_ma_msg_t *msg)
{
    const plan_ma_msg_op_t *msg_op;
    int i, *update_op, update_op_size, op_id, value;
    int response_op_size, response_op_id, response_value;

    response_op_size = planMAMsgOpSize(msg);
    update_op = alloca(sizeof(int) * response_op_size);
    update_op_size = 0;
    for (i = 0; i < response_op_size; ++i){
        // Read data for operator
        msg_op = planMAMsgOp(msg, i);
        response_op_id = planMAMsgOpOpId(msg_op);
        response_value = planMAMsgOpValue(msg_op);

        // Translate operator ID from response to local ID
        op_id = planOpIdTrLoc(&heur->op_id_tr, response_op_id);

        // Determine current value of operator
        value = heur->relax.op[op_id].value;

        // Record updated value of operator and remember operator for
        // later.
        if (value != response_value){
            heur->relax.op[op_id].value = response_value;
            update_op[update_op_size++] = op_id;
        }
    }

    // Early exit if no operators were changed
    if (update_op_size == 0)
        return;

    // Update .relax structure
    updateRelax(heur, update_op, update_op_size);
}

static int heurMAMaxUpdate(plan_heur_t *_heur, plan_ma_comm_t *comm,
                           const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);
    int other_agent_id;

    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || planMAMsgSubType(msg) != PLAN_MA_MSG_HEUR_MAX_RESPONSE){
        fprintf(stderr, "Error[%d]: Heur response isn't for h^max"
                        " heuristic.\n", comm->node_id);
        return -1;
    }

    other_agent_id = planMAMsgAgent(msg);
    if (other_agent_id != heur->cur_agent_id){
        fprintf(stderr, "Error[%d]: Heur response from %d is expected, instead"
                        " response from %d is received.\n",
                        comm->node_id, heur->cur_agent_id, other_agent_id);
        return -1;
    }

    heur->agent_changed[other_agent_id] = 0;

    // Update h^max values of facts and operators considering changes in
    // the operators
    updateHMax(heur, msg);

    // Check if we are done
    if (!needsUpdate(heur)){
        res->heur = heur->relax.fact[heur->relax.cref.goal_id].value;
        return 0;
    }

    // Determine next agent to which send request
    do {
        heur->cur_agent_id = (heur->cur_agent_id + 1) % heur->agent_size;
    } while (!heur->agent_changed[heur->cur_agent_id]);
    sendRequest(heur, comm, heur->cur_agent_id);

    return -1;
}



static void requestSendResponse(private_t *private,
                                plan_ma_comm_t *comm,
                                const plan_op_id_tr_t *op_id_tr,
                                const plan_ma_msg_t *req_msg)
{
    plan_ma_msg_t *msg;
    const plan_ma_msg_op_t *msg_op;
    plan_ma_msg_op_t *add_op;
    int target_agent, i, len, op_id, old_value, new_value, loc_op_id;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_MAX_RESPONSE,
                       planMACommId(comm));

    len = planMAMsgOpSize(req_msg);
    for (i = 0; i < len; ++i){
        msg_op = planMAMsgOp(req_msg, i);
        op_id = planMAMsgOpOpId(msg_op);
        old_value = planMAMsgOpValue(msg_op);

        loc_op_id = planOpIdTrLoc(op_id_tr, op_id);
        if (loc_op_id < 0)
            continue;

        new_value = private->relax.op[loc_op_id].value;
        if (new_value != old_value){
            add_op = planMAMsgAddOp(msg);
            planMAMsgOpSetOpId(add_op, op_id);
            planMAMsgOpSetValue(add_op, new_value);
        }
    }

    target_agent = planMAMsgAgent(req_msg);
    planMACommSendToNode(comm, target_agent, msg);
    planMAMsgDel(msg);
}

static void requestSendEmptyResponse(plan_ma_comm_t *comm,
                                     const plan_ma_msg_t *req)
{
    plan_ma_msg_t *msg;
    int target_agent;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_MAX_RESPONSE,
                       planMACommId(comm));
    target_agent = planMAMsgAgent(req);
    planMACommSendToNode(comm, target_agent, msg);
    planMAMsgDel(msg);
}

static void heurMAMaxRequest(plan_heur_t *_heur, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);
    const plan_ma_msg_op_t *msg_op;
    private_t *private = &heur->private;
    int i, op_id, op_len, fact_id;
    plan_cost_t op_value;
    PLAN_STATE_STACK(state, private->relax.cref.fact_id.var_size);

    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || planMAMsgSubType(msg) != PLAN_MA_MSG_HEUR_MAX_REQUEST){
        fprintf(stderr, "Error[%d]: Heur request isn't for h^max"
                        " heuristic.\n", comm->node_id);
        return;
    }

    // Early exit if we have no private operators
    if (private->relax.cref.op_size == 0){
        requestSendEmptyResponse(comm, msg);
        return;
    }

    // Set up values of fake preconditions according to the operator values
    // received from the other agent.
    op_len = planMAMsgOpSize(msg);
    for (i = 0; i < op_len; ++i){
        msg_op = planMAMsgOp(msg, i);
        op_id = planMAMsgOpOpId(msg_op);
        op_value = planMAMsgOpValue(msg_op);
        op_id = planOpIdTrLoc(&private->op_id_tr, op_id);
        if (op_id < 0)
            continue;

        // Substract cost of the operator from the value to get correct
        // fact value
        op_value -= private->relax.op_init[op_id].cost;
        op_value = BOR_MAX(0, op_value);

        // Find out ID of the fake fact corresponding to the operator
        fact_id = private->fake_pre[op_id];
        // and set the initial value
        planHeurRelaxSetFakePreValue(&private->relax, fact_id, op_value);
    }

    // Run full relaxation heuristic
    planMAStateGetFromMAMsg(heur->heur.ma_state, msg, &state);
    planHeurRelaxFull(&private->relax, &state);

    // Send operator's new values back as response
    requestSendResponse(private, comm, &private->op_id_tr, msg);
}
