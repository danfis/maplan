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
#include "heur_dtg.h"

struct _pending_req_t {
    int wait;
    int cost;
};
typedef struct _pending_req_t pending_req_t;

struct _pending_reqs_t {
    pending_req_t *pending; /*!< List of pending requests */
    int response_agent;     /*!< ID of agent to which send response */
    int response_token;     /*!< Token expected as response */
    int response_depth;     /*!< Depth of a distributed recursion */
    int base_cost;          /*!< Base cost to which add minimal cost from
                                 requests */
    int size;               /*!< Number of pending requests in this slot */
};
typedef struct _pending_reqs_t pending_reqs_t;

struct _plan_heur_ma_dtg_t {
    plan_heur_t heur;
    plan_heur_dtg_data_t data;
    plan_heur_dtg_ctx_t ctx;

    plan_part_state_t goal;
    plan_state_t state;
    plan_op_t *fake_op;
    int fake_op_size;

    pending_reqs_t *waitlist;
    int waitlist_size;
};
typedef struct _plan_heur_ma_dtg_t plan_heur_ma_dtg_t;
#define HEUR(parent) \
    bor_container_of((parent), plan_heur_ma_dtg_t, heur)

static void hdtgDel(plan_heur_t *heur);
static int hdtgHeur(plan_heur_t *heur, plan_ma_comm_t *comm,
                    const plan_state_t *state, plan_heur_res_t *res);
static int hdtgUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res);
static void hdtgRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg);

static void initFakeOp(plan_heur_ma_dtg_t *hdtg,
                       const plan_problem_t *prob);
static void initDTGData(plan_heur_ma_dtg_t *hdtg,
                        const plan_problem_t *prob);

static void pendingReqsInit(plan_heur_ma_dtg_t *hdtg, pending_reqs_t *r);
static void pendingReqsFree(plan_heur_ma_dtg_t *hdtg, pending_reqs_t *r);

/** Returns next available token, i.e., slot for pending requests.
 *  If not available a new one is allocated. */
static int hdtgNextToken(plan_heur_ma_dtg_t *hdtg);
/** Sets 1 for each owner in {owner} array that should receive request for
 *  path between from and to. */
static int hdtgFindOwners(plan_heur_ma_dtg_t *hdtg,
                          int var, int from, int to, int *owner);
/** Sends response to the request. If depth > 1, RESRESPONSE is send which
 *  enables distributed recursion */
static void hdtgSendResponse(plan_heur_ma_dtg_t *hdtg,
                             plan_ma_comm_t *comm,
                             int agent, int token, int cost,
                             int depth);
/** Sends request to the agents on transition from-to.
 *  If it is request from request (recursion), base request message req_msg
 *  is provided. */
static int hdtgSendRequest(plan_heur_ma_dtg_t *hdtg,
                           plan_ma_comm_t *comm,
                           int var, int from, int to,
                           const plan_ma_msg_t *req_msg,
                           int *token_out);
/** Check path whether it contains '?' value and send request if so.
 *  Returns >0 if request was send, 0 if path is ok and -1 if path is not
 *  reachable. */
static int hdtgCheckPath(plan_heur_ma_dtg_t *hdtg,
                         plan_ma_comm_t *comm,
                         const plan_heur_dtg_path_t *path,
                         int var, int goal, int init_val,
                         const plan_ma_msg_t *req_msg,
                         int *token_out);
/** Performs one step in dtg heuristic */
static int hdtgStep(plan_heur_ma_dtg_t *hdtg, plan_ma_comm_t *comm);
/** Process response or req-response */
static int update(plan_heur_ma_dtg_t *hdtg, plan_ma_comm_t *comm,
                  const plan_ma_msg_t *msg);

plan_heur_t *planHeurMADTGNew(const plan_problem_t *agent_def)
{
    plan_heur_ma_dtg_t *hdtg;

    hdtg = BOR_ALLOC(plan_heur_ma_dtg_t);
    _planHeurInit(&hdtg->heur, hdtgDel, NULL, NULL);
    _planHeurMAInit(&hdtg->heur, hdtgHeur, NULL, hdtgUpdate, hdtgRequest);
    initFakeOp(hdtg, agent_def);
    initDTGData(hdtg, agent_def);
    planHeurDTGCtxInit(&hdtg->ctx, &hdtg->data);

    planPartStateInit(&hdtg->goal, agent_def->var_size);
    planPartStateCopy(&hdtg->goal, agent_def->goal);
    planStateInit(&hdtg->state, agent_def->var_size);

    hdtg->waitlist = NULL;
    hdtg->waitlist_size = 0;

    return &hdtg->heur;
}

static void hdtgDel(plan_heur_t *heur)
{
    plan_heur_ma_dtg_t *hdtg = HEUR(heur);
    int i;

    for (i = 0; i < hdtg->waitlist_size; ++i)
        pendingReqsFree(hdtg, hdtg->waitlist + i);
    if (hdtg->waitlist)
        BOR_FREE(hdtg->waitlist);
    planPartStateFree(&hdtg->goal);
    planStateFree(&hdtg->state);
    for (i = 0; i < hdtg->fake_op_size; ++i)
        planOpFree(hdtg->fake_op + i);
    if (hdtg->fake_op)
        BOR_FREE(hdtg->fake_op);
    planHeurDTGDataFree(&hdtg->data);
    planHeurDTGCtxFree(&hdtg->ctx);

    _planHeurFree(&hdtg->heur);
    BOR_FREE(hdtg);
}

static int hdtgHeur(plan_heur_t *heur, plan_ma_comm_t *comm,
                    const plan_state_t *state, plan_heur_res_t *res)
{
    plan_heur_ma_dtg_t *hdtg = HEUR(heur);
    int ret;

    // Remember initial state
    planStateCopy(&hdtg->state, state);
    // Private values are copied too, because these values are not in DTG
    // graph anyway, so the agent ignores these values.

    // Start computing dtg heuristic
    planHeurDTGCtxInitStep(&hdtg->ctx, &hdtg->data,
                           hdtg->state.val, hdtg->state.size,
                           hdtg->goal.vals, hdtg->goal.vals_size);

    // Run dtg heuristic
    while ((ret = hdtgStep(hdtg, comm)) == 0);
    if (ret == 1)
        return -1;

    res->heur = hdtg->ctx.heur;
    return 0;
}

static int hdtgUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    plan_heur_ma_dtg_t *hdtg = HEUR(heur);

    if (update(hdtg, comm, msg) == 0){
        res->heur = hdtg->ctx.heur;
        return 0;
    }
    return -1;
}

static void _reqDTG(plan_heur_ma_dtg_t *hdtg, const plan_ma_msg_t *msg,
                    int *var, int *val_from, int *val_to_out)
{
    const plan_heur_dtg_path_t *path;
    int val_to;
    int i, size, val, vlen;

    // Read var and value pair from message
    planMAMsgDTGReq(msg, var, val_from, &val_to);

    // Determine some usable val_to if not already set
    path = planHeurDTGDataPath(&hdtg->data, *var, *val_from);

    // If val_to was received as -1, find the nearest value from all
    // reachable values also stored in message.
    if (val_to == -1){
        size = planMAMsgDTGReqReachableSize(msg);
        vlen = INT_MAX;
        for (i = 0; i < size; ++i){
            val = planMAMsgDTGReqReachable(msg, i);
            if (path->pre[val].val != -1 && path->pre[val].len < vlen){
                vlen = path->pre[val].len;
                val_to = val;
            }
        }
    }

    // If val_to is still -1 use fake-value
    if (val_to == -1)
        val_to = hdtg->data.dtg.dtg[*var].val_size - 1;

    *val_to_out = val_to;
}

static void hdtgRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
    plan_heur_ma_dtg_t *hdtg = HEUR(heur);
    //PLAN_STATE_STACK(state, hdtg->data.dtg.var_size);
    const plan_heur_dtg_path_t *path;
    int subtype;
    int agent_id, token, depth, req_token;
    int var, val_from, val_to, cost;
    int ret;

    subtype = planMAMsgSubType(msg);
    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || (subtype != PLAN_MA_MSG_HEUR_DTG_REQUEST
                    && subtype != PLAN_MA_MSG_HEUR_DTG_REQRESPONSE)){
        return;
    }

    // If the message is response from distributed recursion, just process
    // this response and exit
    if (subtype == PLAN_MA_MSG_HEUR_DTG_REQRESPONSE){
        update(hdtg, comm, msg);
        return;
    }

    // Read basic info from request
    agent_id = planMAMsgAgent(msg);
    token = planMAMsgHeurToken(msg);
    depth = planMAMsgHeurRequestedAgentSize(msg);

    // Determine variable ID and pair of values
    _reqDTG(hdtg, msg, &var, &val_from, &val_to);

    // Get path corresponding to the 'from' value
    path = planHeurDTGDataPath(&hdtg->data, var, val_from);

    // If val_to is still unreachable report it back as zero (we don't know
    // whether it is dead end or not)
    if (path->pre[val_to].val == -1){
        hdtgSendResponse(hdtg, comm, agent_id, token, 0, depth);
        return;
    }

    // Determine cost
    cost = path->pre[val_to].len;

    // Check path if it contains '?' values
    ret = hdtgCheckPath(hdtg, comm, path, var, val_from, val_to, msg,
                        &req_token);
    if (ret < 0){
        // Could not send request (either there is no path or we reached
        // limit for distributed recursion) -- report back zero.
        hdtgSendResponse(hdtg, comm, agent_id, token, 0, depth);

    }else if (ret > 0){
        // Save cost as a base cost for updates
        hdtg->waitlist[req_token].base_cost = cost;

    }else{
        // Just send response
        hdtgSendResponse(hdtg, comm, agent_id, token, cost, depth);
    }
}




static void dtgAddOp(plan_heur_ma_dtg_t *hdtg, const plan_op_t *op)
{
    const plan_op_t *trans_op = hdtg->fake_op + op->owner;
    plan_var_id_t var;
    plan_val_t val, val2;
    int _;

    PLAN_PART_STATE_FOR_EACH(op->pre, _, var, val){
        val2 = hdtg->data.dtg.dtg[var].val_size - 1;
        if (!planDTGHasTrans(&hdtg->data.dtg, var, val, val2, trans_op)){
            planDTGAddTrans(&hdtg->data.dtg, var, val, val2, trans_op);
        }
    }

    PLAN_PART_STATE_FOR_EACH(op->eff, _, var, val){
        val2 = hdtg->data.dtg.dtg[var].val_size - 1;
        if (!planDTGHasTrans(&hdtg->data.dtg, var, val2, val, trans_op)){
            planDTGAddTrans(&hdtg->data.dtg, var, val2, val, trans_op);
        }
    }
}

static void initFakeOp(plan_heur_ma_dtg_t *hdtg,
                       const plan_problem_t *prob)
{
    int max_agent_id = 0;
    int i;
    char name[128];

    for (i = 0; i < prob->proj_op_size; ++i)
        max_agent_id = BOR_MAX(max_agent_id, prob->proj_op[i].owner);

    hdtg->fake_op_size = max_agent_id + 1;
    hdtg->fake_op = BOR_ALLOC_ARR(plan_op_t, hdtg->fake_op_size);
    for (i = 0; i < hdtg->fake_op_size; ++i){
        planOpInit(hdtg->fake_op + i, 1);
        hdtg->fake_op[i].owner = i;
        sprintf(name, "agent-%d", i);
        hdtg->fake_op[i].name = BOR_STRDUP(name);
    }
}

static void initDTGData(plan_heur_ma_dtg_t *hdtg,
                        const plan_problem_t *prob)
{
    int i;
    const plan_op_t *op;

    planHeurDTGDataInitUnknown(&hdtg->data, prob->var, prob->var_size,
                               prob->op, prob->op_size);
    for (i = 0; i < prob->proj_op_size; ++i){
        op = prob->proj_op + i;
        if (op->owner == prob->agent_id)
            continue;
        dtgAddOp(hdtg, op);
    }
}

static void pendingReqsInit(plan_heur_ma_dtg_t *hdtg, pending_reqs_t *r)
{
    r->pending = BOR_CALLOC_ARR(pending_req_t, hdtg->fake_op_size);
    r->size = 0;
}

static void pendingReqsFree(plan_heur_ma_dtg_t *hdtg, pending_reqs_t *r)
{
    if (r->pending)
        BOR_FREE(r->pending);
}


static int hdtgNextToken(plan_heur_ma_dtg_t *hdtg)
{
    int i;

    for (i = 0; i < hdtg->waitlist_size; ++i){
        if (hdtg->waitlist[i].size == 0)
            return i;
    }

    ++hdtg->waitlist_size;
    hdtg->waitlist = BOR_REALLOC_ARR(hdtg->waitlist, pending_reqs_t,
                                     hdtg->waitlist_size);
    pendingReqsInit(hdtg, hdtg->waitlist + hdtg->waitlist_size - 1);
    return hdtg->waitlist_size - 1;
}

static int hdtgFindOwners(plan_heur_ma_dtg_t *hdtg,
                          int var, int from, int to,
                          int *owner)
{
    const plan_dtg_trans_t *trans;
    int found = 0, fake_val, i;

    bzero(owner, sizeof(int) * hdtg->fake_op_size);
    fake_val = hdtg->data.dtg.dtg[var].val_size - 1;

    // Process first transition.
    // Note that we need transition in an opposite way because we search
    // for paths from goals to known facts.
    trans = planDTGTrans(&hdtg->data.dtg, var, fake_val, from);
    for (i = 0; i < trans->ops_size; ++i){
        owner[trans->ops[i]->owner] = 1;
        found = 1;
    }

    // Second transition is ignored because we don't need it

    if (!found)
        return -1;
    return 0;
}

static void hdtgSendResponse(plan_heur_ma_dtg_t *hdtg,
                             plan_ma_comm_t *comm,
                             int agent, int token, int cost,
                             int depth)
{
    plan_ma_msg_t *msg;
    int subtype;

    subtype = PLAN_MA_MSG_HEUR_DTG_RESPONSE;
    if (depth > 1)
        subtype = PLAN_MA_MSG_HEUR_DTG_REQRESPONSE;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, subtype, comm->node_id);
    planMAMsgSetHeurToken(msg, token);
    planMAMsgSetHeurCost(msg, cost);
    planMACommSendToNode(comm, agent, msg);
    planMAMsgDel(msg);
}

/** Wrapper for hdtgFindOwners() that processes also req_msg is set. */
static int _sendReqFindOwners(plan_heur_ma_dtg_t *hdtg,
                              int var, int from, int to,
                              const plan_ma_msg_t *req_msg,
                              int *owner)
{
    int i, size, sum;

    if (hdtgFindOwners(hdtg, var, from, to, owner) != 0)
        return -1;

    if (req_msg){
        // Zeroize owners that were already asked
        size = planMAMsgHeurRequestedAgentSize(req_msg);
        for (i = 0; i < size; ++i){
            owner[planMAMsgHeurRequestedAgent(req_msg, i)] = 0;
        }

        // Check whether some owner was left
        for (sum = 0, i = 0; i < hdtg->fake_op_size; ++i)
            sum += owner[i];
        if (sum == 0)
            return -1;
    }

    return 0;
}

static plan_ma_msg_t *_sendReqMsgNew(plan_heur_ma_dtg_t *hdtg,
                                     plan_ma_comm_t *comm,
                                     int var, int from, int to,
                                     int token, const int *owner,
                                     const plan_ma_msg_t *req_msg)
{
    plan_ma_msg_t *msg;
    int i, size, range, *vals;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_DTG_REQUEST,
                       comm->node_id);
    if (req_msg){
        // Copy reachable values
        planMAMsgCopyDTGReqReachable(msg, req_msg);

        // Copy requested agents to the next request
        size = planMAMsgHeurRequestedAgentSize(req_msg);
        for (i = 0; i < size; ++i){
            planMAMsgAddHeurRequestedAgent(msg,
                    planMAMsgHeurRequestedAgent(req_msg, i));
        }

    }else{
        vals = hdtg->ctx.values.val[var];
        range = hdtg->ctx.values.val_range[var] - 1;
        for (i = 0; i < range; ++i){
            if (vals[i])
                planMAMsgAddDTGReqReachable(msg, i);
        }
    }

    planMAMsgSetHeurToken(msg, token);
    planMAMsgAddHeurRequestedAgent(msg, comm->node_id);
    planMAMsgSetDTGReq(msg, var, from, to);

    return msg;
}

static int hdtgSendRequest(plan_heur_ma_dtg_t *hdtg,
                           plan_ma_comm_t *comm,
                           int var, int from, int to,
                           const plan_ma_msg_t *req_msg,
                           int *token_out)
{
    plan_ma_msg_t *msg;
    pending_reqs_t *wait;
    int token;
    int owner[hdtg->fake_op_size];
    int i;

    // Find out which agents are on receiving side.
    if (_sendReqFindOwners(hdtg, var, from, to, req_msg, owner) != 0)
        return -1;

    // Reserve token (slot) for responses.
    token = hdtgNextToken(hdtg);

    // Create a message
    msg = _sendReqMsgNew(hdtg, comm, var, from, to, token, owner, req_msg);

    // Initialize waitlist slot
    wait = hdtg->waitlist + token;
    wait->base_cost = 0;
    wait->response_agent = -1;
    wait->response_token = -1;
    wait->response_depth = 0;
    if (req_msg){
        wait->response_agent = planMAMsgAgent(req_msg);
        wait->response_token = planMAMsgHeurToken(req_msg);
        wait->response_depth = planMAMsgHeurRequestedAgentSize(req_msg);
    }

    // Send message to agents and update corresponding items in waitlist
    // slot.
    wait->size = 0;
    for (i = 0; i < hdtg->fake_op_size; ++i){
        if (!owner[i])
            continue;

        // Add this request to the waitlist of pending requests
        wait->pending[i].wait = 1;
        wait->pending[i].cost = 0;
        ++wait->size;

        planMACommSendToNode(comm, i, msg);
    }
    planMAMsgDel(msg);

    if (token_out)
        *token_out = token;

    return 0;
}

static int hdtgCheckPath(plan_heur_ma_dtg_t *hdtg,
                         plan_ma_comm_t *comm,
                         const plan_heur_dtg_path_t *path,
                         int var, int goal, int init_val,
                         const plan_ma_msg_t *req_msg,
                         int *token_out)
{
    int fake_val, prev_val, cur_val, next_val;
    int ret;

    fake_val = hdtg->data.dtg.dtg[var].val_size - 1;
    prev_val = -1;
    cur_val = init_val;

    if (init_val == fake_val){
        // Send "open-ended" request to agents on transition
        next_val = path->pre[init_val].val;
        ret = hdtgSendRequest(hdtg, comm, var, next_val, -1,
                              req_msg, token_out);
        if (ret != 0)
            return -1;

        // Signal to decrement heur value by 1 because we must substract
        // cost of a fake transition.
        return 1;

        prev_val = init_val;
        cur_val = path->pre[init_val].val;
    }

    while (cur_val != goal){
        if (cur_val == fake_val){
            // Send request to agents on transition
            next_val = path->pre[cur_val].val;
            ret = hdtgSendRequest(hdtg, comm, var, next_val, prev_val,
                                  req_msg, token_out);
            if (ret != 0)
                return -1;

            // Signal to decrement heur value by 2 because we must
            // substract cost oo fake transitions to the fake value and
            // from the fake value.
            return 2;
        }

        prev_val = cur_val;
        cur_val = path->pre[cur_val].val;
    }

    return 0;
}

static int hdtgStepRequest(plan_heur_ma_dtg_t *hdtg,
                           plan_ma_comm_t *comm,
                           const plan_heur_dtg_path_t *path,
                           int var, int goal, int init_val)
{
    int ret;

    ret = hdtgCheckPath(hdtg, comm, path, var, goal, init_val, NULL, NULL);
    if (ret < 0){
        // Path is not reachable. We must ignore this because the fact that
        // this particular path isn't reachable does not mean there is no
        // path from the initial state to the goal. It just can lead other
        // way.
        return 0;

    }else if (ret > 0){
        return 1;
    }
    return 0;
}

static int hdtgStepNoPath(plan_heur_ma_dtg_t *hdtg,
                          plan_ma_comm_t *comm,
                          plan_heur_dtg_open_goal_t open_goal)
{
    int fake_val;

    fake_val = hdtg->data.dtg.dtg[open_goal.var].val_size - 1;
    if (open_goal.path->pre[fake_val].val != -1){
        // At least '?' value is reachable so try other agents and ask them
        // for cost of path.
        // But first we must 'invent' the local cost of path.
        hdtg->ctx.heur += open_goal.path->pre[fake_val].len;
        return hdtgStepRequest(hdtg, comm, open_goal.path, open_goal.var,
                               open_goal.val, fake_val);
    }else{
        // The '?' value isn't reacheble either -- just ignore this because
        // we cannot be sure it means dead-end.
        return 0;
    }
}

static int hdtgStep(plan_heur_ma_dtg_t *hdtg, plan_ma_comm_t *comm)
{
    plan_heur_dtg_open_goal_t open_goal;
    int ret;

    ret = planHeurDTGCtxStep(&hdtg->ctx, &hdtg->data);
    if (ret == -1)
        return -1;

    open_goal = hdtg->ctx.cur_open_goal;
    if (open_goal.min_val == -1){
        // We haven't found any path in DTG -- find out whether the '?'
        // value is reachable.
        return hdtgStepNoPath(hdtg, comm, open_goal);
    }

    // Check the path if we need to ask other agents
    return hdtgStepRequest(hdtg, comm, open_goal.path, open_goal.var,
                           open_goal.val, open_goal.min_val);
}


static int update(plan_heur_ma_dtg_t *hdtg, plan_ma_comm_t *comm,
                  const plan_ma_msg_t *msg)
{
    pending_reqs_t *req;
    int token, agent_id, cost;
    int i, ret;

    // Read data from received message
    agent_id = planMAMsgAgent(msg);
    token = planMAMsgHeurToken(msg);
    cost = planMAMsgHeurCost(msg);

    // Find corresponding pending request
    req = hdtg->waitlist + token;

    // Record data from response
    req->pending[agent_id].cost = cost;

    // Decrement number of pending requests and if there are some requests
    // still pending wait for them
    --req->size;
    if (req->size != 0)
        return -1;

    // Find minimal cost and zeroize structure
    for (i = 0; i < hdtg->fake_op_size; ++i){
        if (req->pending[i].wait)
            cost = BOR_MIN(cost, req->pending[i].cost);
        req->pending[i].wait = 0;
        req->pending[i].cost = 0;
    }

    // Update cost with base cost
    cost += req->base_cost;

    if (req->response_agent == -1){
        // Update heuristic value
        hdtg->ctx.heur += cost;

        // Proceed with dtg heuristic
        while ((ret = hdtgStep(hdtg, comm)) == 0);
        if (ret == 1)
            return -1;

    }else{
        // Send response to the parent caller
        hdtgSendResponse(hdtg, comm, req->response_agent,
                         req->response_token, cost, req->response_depth);
    }

    return 0;
}
