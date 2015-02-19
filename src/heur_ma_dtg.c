#include <boruvka/alloc.h>
#include "plan/heur.h"
#include "heur_dtg.h"

struct _pending_req_t {
    int wait;
    int cost;
};
typedef struct _pending_req_t pending_req_t;

struct _plan_heur_ma_dtg_t {
    plan_heur_t heur;
    plan_heur_dtg_data_t data;
    plan_heur_dtg_ctx_t ctx;

    plan_part_state_t goal;
    plan_state_t state;
    plan_op_t *fake_op;
    int fake_op_size;
    int token;
    pending_req_t *waitlist;
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

plan_heur_t *planHeurMADTGNew(const plan_problem_t *agent_def)
{
    plan_heur_ma_dtg_t *hdtg;

    hdtg = BOR_ALLOC(plan_heur_ma_dtg_t);
    _planHeurInit(&hdtg->heur, hdtgDel, NULL);
    _planHeurMAInit(&hdtg->heur, hdtgHeur, hdtgUpdate, hdtgRequest);
    initFakeOp(hdtg, agent_def);
    initDTGData(hdtg, agent_def);
    planHeurDTGCtxInit(&hdtg->ctx, &hdtg->data);

    planPartStateInit(&hdtg->goal, agent_def->var_size);
    planPartStateCopy(&hdtg->goal, agent_def->goal);
    planStateInit(&hdtg->state, agent_def->var_size);
    hdtg->token = 1;
    hdtg->waitlist = BOR_CALLOC_ARR(pending_req_t, hdtg->fake_op_size);

    return &hdtg->heur;
}

static void hdtgDel(plan_heur_t *heur)
{
    plan_heur_ma_dtg_t *hdtg = HEUR(heur);
    int i;

    BOR_FREE(hdtg->waitlist);
    planPartStateFree(&hdtg->goal);
    planStateFree(&hdtg->state);
    for (i = 0; i < hdtg->fake_op_size; ++i)
        planOpFree(hdtg->fake_op + i);
    if (hdtg->fake_op)
        BOR_FREE(hdtg->fake_op);
    planHeurDTGDataFree(&hdtg->data);

    _planHeurFree(&hdtg->heur);
    BOR_FREE(hdtg);
}



static int hdtgNextToken(plan_heur_ma_dtg_t *hdtg)
{
    ++hdtg->token;
    if (hdtg->token == 0)
        ++hdtg->token;
    return hdtg->token;
}

static int hdtgFindOwners(plan_heur_ma_dtg_t *hdtg,
                          int var, int from, int to,
                          int *owner)
{
    const plan_dtg_trans_t *trans;
    int found = 0, fake_val, i;

    bzero(owner, sizeof(int) * hdtg->fake_op_size);
    fake_val = hdtg->data.dtg.dtg[var].val_size - 1;

    // Process first transition
    trans = planDTGTrans(&hdtg->data.dtg, var, from, fake_val);
    for (i = 0; i < trans->ops_size; ++i)
        owner[trans->ops[i]->owner] += 1;

    // Process second transition
    if (to >= 0){
        trans = planDTGTrans(&hdtg->data.dtg, var, fake_val, to);
        for (i = 0; i < trans->ops_size; ++i)
            owner[trans->ops[i]->owner] += 1;
    }else{
        for (i = 0; i < hdtg->fake_op_size; ++i)
            owner[i] += 1;
    }

    for (i = 0; i < hdtg->fake_op_size; ++i){
        if (owner[i] == 2){
            found = 1;
        }else{
            owner[i] = 0;
        }
    }

    if (!found)
        return -1;
    return 0;
}

static int hdtgSendRequest(plan_heur_ma_dtg_t *hdtg,
                           plan_ma_comm_t *comm,
                           int var, int from, int to)
{
    plan_ma_msg_t *msg;
    int token = 0;
    int owner[hdtg->fake_op_size];
    int i;

    if (hdtgFindOwners(hdtg, var, from, to, owner) != 0)
        return -1;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_DTG_REQUEST,
                       comm->node_id);
    planMAMsgSetInitAgent(msg, comm->node_id);
    planMAMsgSetStateFull2(msg, &hdtg->state);
    planMAMsgSetHeurToken(msg, token);
    planMAMsgAddHeurRequestedAgent(msg, comm->node_id);
    planMAMsgSetDTGReq(msg, var, from, to);

    for (i = 0; i < hdtg->fake_op_size; ++i){
        if (owner[i]){
            fprintf(stderr, "[%d] Send request to %d\n", comm->node_id, i);
            fflush(stderr);
            planMACommSendToNode(comm, i, msg);

            // Add this request to the waitlist of pending requests
            hdtg->waitlist[i].wait = 1;
            hdtg->waitlist[i].cost = 0;
        }
    }
    planMAMsgDel(msg);

    return 0;
}

static int hdtgSendOpenRequest(plan_heur_ma_dtg_t *hdtg,
                               plan_ma_comm_t *comm,
                               int var, int from)
{
    return hdtgSendRequest(hdtg, comm, var, from, -1);
}

static int hdtgStepRequest(plan_heur_ma_dtg_t *hdtg,
                           plan_ma_comm_t *comm,
                           const plan_heur_dtg_path_t *path,
                           int var, int goal, int init_val)
{
    int fake_val, prev_val, cur_val, next_val;

    fake_val = hdtg->data.dtg.dtg[var].val_size - 1;
    prev_val = -1;
    cur_val = init_val;

    if (init_val == fake_val){
        // Decrement heur value by 1 because we must substract cost of a
        // fake transition.
        hdtg->ctx.heur -= 1;

        // Send "open-ended" request to agents on transition
        next_val = path->pre[init_val].val;
        if (hdtgSendOpenRequest(hdtg, comm, var, next_val) != 0){
            hdtg->ctx.heur = PLAN_HEUR_DEAD_END;
            return -1;
        }
        return 1;

        prev_val = init_val;
        cur_val = path->pre[init_val].val;
    }

    while (cur_val != goal){
        if (cur_val == fake_val){
            // Decrement heur value by 2 because we must substract cost of
            // fake transitions to the fake value and from the fake value.
            hdtg->ctx.heur -= 2;

            // Send request to agents on transition
            next_val = path->pre[cur_val].val;
            if (hdtgSendRequest(hdtg, comm, var, next_val, prev_val) != 0){
                hdtg->ctx.heur = PLAN_HEUR_DEAD_END;
                return -1;
            }
            return 1;
        }

        prev_val = cur_val;
        cur_val = path->pre[cur_val].val;
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
        // The '?' value isn't reacheble either -- this means we have
        // reached dead-end.
        hdtg->ctx.heur = PLAN_HEUR_DEAD_END;
        return -1;
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

static int hdtgHeur(plan_heur_t *heur, plan_ma_comm_t *comm,
                    const plan_state_t *state, plan_heur_res_t *res)
{
    plan_heur_ma_dtg_t *hdtg = HEUR(heur);
    int ret;

    fprintf(stderr, "[%d] DTG HEUR\n", comm->node_id);
    // Remember initial state
    planStateCopy(&hdtg->state, state);
    // TODO: disable unknown values

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
    int token, agent_id, cost;
    int ret;

    // Read data from received message
    agent_id = planMAMsgAgent(msg);
    token = planMAMsgHeurToken(msg);
    cost = planMAMsgHeurCost(msg);
    fprintf(stderr, "[%d] token: %d, agent: %d, cost: %d\n", comm->node_id, token,
            agent_id, cost);
    fflush(stderr);

    // Update heuristic value
    hdtg->ctx.heur += cost;

    // Proceed with dtg heuristic
    while ((ret = hdtgStep(hdtg, comm)) == 0);
    if (ret == 1)
        return -1;

    res->heur = hdtg->ctx.heur;
    return 0;
}

static void hdtgSendResponse(plan_heur_ma_dtg_t *hdtg,
                             plan_ma_comm_t *comm,
                             int agent, int token, int cost)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_DTG_RESPONSE,
                       comm->node_id);
    planMAMsgSetHeurToken(msg, token);
    planMAMsgSetHeurCost(msg, cost);
    fprintf(stderr, "[%d] Send response to %d\n", comm->node_id, agent);
    fflush(stderr);
    planMACommSendToNode(comm, agent, msg);
    planMAMsgDel(msg);
}

static void hdtgRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
    plan_heur_ma_dtg_t *hdtg = HEUR(heur);
    PLAN_STATE_STACK(state, hdtg->data.dtg.var_size);
    const plan_heur_dtg_path_t *path;
    int agent_id, token;
    int var, val_from, val_to;

    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || planMAMsgSubType(msg) != PLAN_MA_MSG_HEUR_DTG_REQUEST){
        fprintf(stderr, "[%d] Error: Invalid request received.\n",
                comm->node_id);
        return;
    }

    agent_id = planMAMsgAgent(msg);
    token = planMAMsgHeurToken(msg);
    fprintf(stderr, "[%d] %d\n", comm->node_id, agent_id);

    planMAMsgDTGReq(msg, &var, &val_from, &val_to);
    if (val_to == -1){
        planMAMsgStateFull(msg, &state);
        // TODO: disable unknown values
        val_to = planStateGet(&state, var);
    }
    fprintf(stderr, "[%d] var: %d, %d -> %d\n", comm->node_id, var, val_from, val_to);
    fflush(stderr);

    path = planHeurDTGDataPath(&hdtg->data, var, val_from);
    // TODO: Check whole path and perform distributed recursion if
    // necessary
    if (path->pre[val_to].val == -1){
        hdtgSendResponse(hdtg, comm, agent_id, token, 1000);
    }else{
        hdtgSendResponse(hdtg, comm, agent_id, token, path->pre[val_to].len);
    }

    /*
    fprintf(stderr, "[%d] len: %d\n", comm->node_id, path->pre[val_to].len);
    fprintf(stderr, "[%d] pre: %d\n", comm->node_id, path->pre[val_to].val);
    planDTGPrint(&hdtg->data.dtg, stderr);
    fflush(stderr);
    */
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
        hdtg->fake_op[i].name = strdup(name);
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
