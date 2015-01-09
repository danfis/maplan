#include <boruvka/alloc.h>
#include <boruvka/lifo.h>

#include "plan/heur.h"
#include "heur_relax.h"
#include "op_id_tr.h"

struct _private_t {
    plan_heur_relax_t relax;
    plan_op_id_tr_t op_id_tr;
    plan_oparr_t public_op; /*!< List of public operators */
};
typedef struct _private_t private_t;

struct _plan_heur_ma_lm_cut_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;
    plan_op_id_tr_t op_id_tr;

    int agent_id;
    int agent_size;            /*!< Number of agents in cluster */
    int *agent_changed;        /*!< True/False for each agent signaling
                                    whether an operator changed */
    int cur_agent_id;          /*!< ID of the agent from which a response
                                    is expected. */
    plan_oparr_t *agent_op;    /*!< Operators owned by a corresponding agent */
    plan_factarr_t init_state; /*!< Stored actual state for which the
                                    heuristic is computed */
    plan_oparr_t *op_by_owner; /*!< List of operators divided by an owner */
    plan_heur_relax_op_t *old_op;

    const plan_problem_t *prob;
    private_t *private;
    int private_size;
};
typedef struct _plan_heur_ma_lm_cut_t plan_heur_ma_lm_cut_t;

#define HEUR(parent) \
    bor_container_of(parent, plan_heur_ma_lm_cut_t, heur)

static void heurDel(plan_heur_t *_heur);
static int heurMA(plan_heur_t *heur, plan_ma_comm_t *comm,
                  const plan_state_t *state, plan_heur_res_t *res);
static int heurMAUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg, plan_heur_res_t *res);
static void heurMARequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                          const plan_ma_msg_t *msg);

static void privateInit(private_t *private, const plan_problem_t *prob)
{
    plan_op_t *op;
    int op_size;
    int i;

    planProblemCreatePrivateProjOps(prob->op, prob->op_size,
                                    prob->var, prob->var_size,
                                    &op, &op_size);
    planHeurRelaxInit(&private->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      prob->var, prob->var_size, prob->goal, op, op_size);

    // Get IDs of all public operators
    private->public_op.op = BOR_ALLOC_ARR(int, op_size);
    private->public_op.size = 0;
    for (i = 0; i < op_size; ++i){
        if (!op[i].is_private){
            private->public_op.op[private->public_op.size++] = i;
        }
    }
    private->public_op.op = BOR_REALLOC_ARR(private->public_op.op, int,
                                            private->public_op.size);

    planOpIdTrInit(&private->op_id_tr, op, op_size);
    planProblemDestroyOps(op, op_size);
}

static void privateFree(private_t *private)
{
    planHeurRelaxFree(&private->relax);
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

static void initAgentOp(plan_heur_ma_lm_cut_t *heur,
                        const plan_op_t *op, int op_size)
{
    int i;
    plan_oparr_t *oparr;

    heur->agent_op = BOR_CALLOC_ARR(plan_oparr_t, heur->agent_size);
    for (i = 0; i < heur->agent_size; ++i)
        heur->agent_op[i].op = BOR_ALLOC_ARR(int, heur->relax.cref.op_size);

    for (i = 0; i < op_size; ++i){
        oparr = heur->agent_op + op[i].owner;
        oparr->op[oparr->size++] = i;
    }

    for (i = 0; i < heur->agent_size; ++i){
        heur->agent_op[i].op = BOR_REALLOC_ARR(heur->agent_op[i].op, int,
                                               heur->agent_op[i].size);
    }
}

static void initOpByOwner(plan_heur_ma_lm_cut_t *heur,
                          const plan_problem_t *prob)
{
    int i, op_id, owner;

    heur->op_by_owner = BOR_CALLOC_ARR(plan_oparr_t, heur->agent_size);
    for (i = 0; i < heur->agent_size; ++i){
        heur->op_by_owner[i].op = BOR_ALLOC_ARR(int, heur->relax.cref.op_size);
    }

    for (i = 0; i < heur->relax.cref.op_size; ++i){
        op_id = heur->relax.cref.op_id[i];
        if (op_id > 0){
            owner = prob->proj_op[op_id].owner;
            heur->op_by_owner[owner].op[heur->op_by_owner[owner].size++] = i;
        }
    }

    for (i = 0; i < heur->agent_size; ++i){
        heur->op_by_owner[i].op = BOR_REALLOC_ARR(heur->op_by_owner[i].op, int,
                                                  heur->op_by_owner[i].size);
    }
}

plan_heur_t *planHeurMALMCutNew(const plan_problem_t *prob)
{
    plan_heur_ma_lm_cut_t *heur;

    heur = BOR_ALLOC(plan_heur_ma_lm_cut_t);
    _planHeurInit(&heur->heur, heurDel, NULL);
    _planHeurMAInit(&heur->heur, heurMA, heurMAUpdate, heurMARequest);
    planHeurRelaxInit(&heur->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      prob->var, prob->var_size, prob->goal,
                      prob->proj_op, prob->proj_op_size);
    planOpIdTrInit(&heur->op_id_tr, prob->proj_op, prob->proj_op_size);

    heur->agent_size = agentSize(prob->proj_op, prob->proj_op_size);
    heur->agent_changed = BOR_CALLOC_ARR(int, heur->agent_size);
    initAgentOp(heur, prob->proj_op, prob->proj_op_size);
    initOpByOwner(heur, prob);
    heur->old_op = BOR_ALLOC_ARR(plan_heur_relax_op_t,
                                 heur->relax.cref.op_size);

    heur->init_state.size = prob->var_size;
    heur->init_state.fact = BOR_ALLOC_ARR(int, heur->init_state.size);

    heur->prob = prob;
    heur->private = NULL;
    heur->private_size = 0;

    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_ma_lm_cut_t *heur = HEUR(_heur);
    int i;

    BOR_FREE(heur->agent_changed);
    for (i = 0; i < heur->agent_size; ++i){
        if (heur->agent_op[i].op)
            BOR_FREE(heur->agent_op[i].op);
    }
    BOR_FREE(heur->agent_op);
    BOR_FREE(heur->old_op);

    BOR_FREE(heur->init_state.fact);
    for (i = 0; i < heur->agent_size; ++i){
        if (heur->op_by_owner[i].op)
            BOR_FREE(heur->op_by_owner[i].op);
    }
    BOR_FREE(heur->op_by_owner);

    for (i = 0; i < heur->private_size; ++i)
        privateFree(heur->private + i);
    if (heur->private)
        BOR_FREE(heur->private);

    planOpIdTrFree(&heur->op_id_tr);
    planHeurRelaxFree(&heur->relax);
    _planHeurFree(&heur->heur);
    BOR_FREE(heur);
}

static void setInitState(plan_heur_ma_lm_cut_t *heur, const plan_state_t *state)
{
    int i;

    for (i = 0; i < heur->init_state.size; ++i){
        heur->init_state.fact[i] = state->val[i];
    }
}

static void sendInitRequest(plan_heur_ma_lm_cut_t *heur,
                            plan_ma_comm_t *comm, int agent_id)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_LM_CUT_INIT_REQUEST,
                       planMACommId(comm));
    planMAMsgHeurLMCutSetInitRequest(msg, heur->init_state.fact,
                                     heur->init_state.size);
    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static void sendMaxRequest(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                           int agent_id)
{
    plan_ma_msg_t *msg;
    plan_oparr_t *oparr;
    int i, op_id, value;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_LM_CUT_MAX_REQUEST,
                       planMACommId(comm));

    oparr = heur->agent_op + agent_id;
    for (i = 0; i < oparr->size; ++i){
        op_id = planOpIdTrGlob(&heur->op_id_tr, oparr->op[i]);
        value = heur->relax.op[oparr->op[i]].value;
        planMAMsgHeurLMCutMaxAddOp(msg, op_id, value);
    }
    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static int heurMA(plan_heur_t *_heur, plan_ma_comm_t *comm,
                  const plan_state_t *state, plan_heur_res_t *res)
{
    plan_heur_ma_lm_cut_t *heur = HEUR(_heur);
    int i;

    // Store state for which we are evaluating heuristics
    setInitState(heur, state);

    // Every agent must be requested at least once, so make sure of that.
    heur->agent_id = planMACommId(comm);
    for (i = 0; i < heur->agent_size; ++i){
        if (i == heur->agent_id){
            heur->agent_changed[i] = 0;
        }else{
            heur->agent_changed[i] = 1;
            sendInitRequest(heur, comm, i);
        }
    }
    heur->cur_agent_id = (planMACommId(comm) + 1) % heur->agent_size;

    // h^max heuristics for all facts and operators reachable from the
    // state
    planHeurRelaxFull(&heur->relax, state);

    // Send request to next agent
    sendMaxRequest(heur, comm, heur->cur_agent_id);

    return -1;
}



static int needsUpdate(const plan_heur_ma_lm_cut_t *heur)
{
    int i, val = 0;
    for (i = 0; i < heur->agent_size; ++i)
        val += heur->agent_changed[i];
    return val;
}

static void updateRelax(plan_heur_ma_lm_cut_t *heur,
                        const int *update_op, int update_op_size)
{
    int i, j, op_id, size;
    plan_oparr_t *oparr;

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
            op_id = oparr->op[j];
            if (heur->old_op[op_id].value != heur->relax.op[op_id].value){
                heur->agent_changed[i] = 1;
                break;
            }
        }
    }
}

static void updateHMax(plan_heur_ma_lm_cut_t *heur, const plan_ma_msg_t *msg)
{
    int i, *update_op, update_op_size, op_id, value;
    int response_op_size, response_op_id, response_value;

    response_op_size = planMAMsgHeurLMCutMaxOpSize(msg);
    update_op = alloca(sizeof(int) * response_op_size);
    update_op_size = 0;
    for (i = 0; i < response_op_size; ++i){
        // Read data for operator
        response_op_id = planMAMsgHeurLMCutMaxOp(msg, i, &response_value);

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

static int heurMAUpdate(plan_heur_t *_heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    plan_heur_ma_lm_cut_t *heur = HEUR(_heur);
    int other_agent_id;

    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || planMAMsgSubType(msg) != PLAN_MA_MSG_HEUR_LM_CUT_MAX_RESPONSE){
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
    sendMaxRequest(heur, comm, heur->cur_agent_id);

    return -1;
}



static void requestSendResponse(private_t *private,
                                plan_ma_comm_t *comm,
                                const plan_op_id_tr_t *op_id_tr,
                                const plan_ma_msg_t *req_msg)
{
    plan_ma_msg_t *msg;
    int target_agent, i, len, op_id, old_value, new_value, loc_op_id;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_MAX_RESPONSE,
                       planMACommId(comm));

    len = planMAMsgHeurMaxOpSize(req_msg);
    for (i = 0; i < len; ++i){
        op_id = planMAMsgHeurMaxOp(req_msg, i, &old_value);

        loc_op_id = planOpIdTrLoc(op_id_tr, op_id);
        if (loc_op_id < 0)
            continue;

        new_value = private->relax.op[loc_op_id].value;
        if (new_value != old_value){
            planMAMsgHeurMaxAddOp(msg, op_id, new_value);
        }
    }

    target_agent = planMAMsgAgent(req_msg);
    planMACommSendToNode(comm, target_agent, msg);
    planMAMsgDel(msg);
}

static void requestMaxSendEmptyResponse(plan_ma_comm_t *comm,
                                        const plan_ma_msg_t *req)
{
    plan_ma_msg_t *msg;
    int target_agent;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_LM_CUT_MAX_RESPONSE,
                       planMACommId(comm));
    target_agent = planMAMsgAgent(req);
    planMACommSendToNode(comm, target_agent, msg);
    planMAMsgDel(msg);
}


static void allocPrivate(plan_heur_ma_lm_cut_t *heur, int agent_id)
{
    int i;

    if (agent_id < heur->private_size)
        return;

    heur->private = BOR_REALLOC_ARR(heur->private, private_t, agent_id + 1);
    for (i = heur->private_size; i < agent_id + 1; ++i)
        privateInit(heur->private + i, heur->prob);
    heur->private_size = agent_id + 1;
}

static void privateInitRequest(private_t *private, plan_ma_comm_t *comm,
                               const plan_ma_msg_t *msg)
{
    PLAN_STATE_STACK(state, private->relax.cref.fact_id.var_size);

    // Early exit if we have no private operators
    if (private->relax.cref.op_size == 0)
        return;

    // Run full relaxation heuristic
    planMAMsgHeurMaxState(msg, &state);
    planHeurRelaxFull(&private->relax, &state);
}

static void privateMaxRequest(private_t *private, plan_ma_comm_t *comm,
                              const plan_ma_msg_t *msg)
{
    int i, *update_op, update_op_size, op_id, value;
    int response_op_size, response_op_id, response_value;

    // Early exit if we have no private operators
    if (private->relax.cref.op_size == 0){
        requestMaxSendEmptyResponse(comm, msg);
        return;
    }

    plan_heur_relax_op_t *old_op;
    old_op = BOR_ALLOC_ARR(plan_heur_relax_op_t,
                           private->relax.cref.op_size);
    memcpy(old_op, private->relax.op, sizeof(plan_heur_relax_op_t) *
            private->relax.cref.op_size);

    response_op_size = planMAMsgHeurLMCutMaxOpSize(msg);
    update_op = alloca(sizeof(int) * response_op_size);
    update_op_size = 0;
    for (i = 0; i < response_op_size; ++i){
        // Read data for operator
        response_op_id = planMAMsgHeurLMCutMaxOp(msg, i, &response_value);

        // Translate operator ID from response to local ID
        op_id = planOpIdTrLoc(&private->op_id_tr, response_op_id);
        if (op_id < 0)
            continue;

        old_op[op_id].value = response_value;

        // Determine current value of operator
        value = private->relax.op[op_id].value;

        // Record updated value of operator and remember operator for
        // later.
        if (value < response_value){
            private->relax.op[op_id].value = response_value;
            update_op[update_op_size++] = op_id;
        }
    }

    // Early exit if no operators were changed
    if (update_op_size > 0){
        // Update .relax structure
        planHeurRelaxUpdateMaxFull(&private->relax, update_op, update_op_size);
    }


    plan_ma_msg_t *msgout;
    msgout = planMAMsgNew(PLAN_MA_MSG_HEUR,
                          PLAN_MA_MSG_HEUR_LM_CUT_MAX_RESPONSE,
                          planMACommId(comm));
    for (i = 0; i < private->public_op.size; ++i){
        op_id = private->public_op.op[i];
        if (old_op[op_id].value == private->relax.op[op_id].value)
            continue;

        value = private->relax.op[op_id].value;
        op_id = planOpIdTrGlob(&private->op_id_tr, op_id);
        planMAMsgHeurLMCutMaxAddOp(msgout, op_id, value);
    }
    planMACommSendToNode(comm, planMAMsgAgent(msg), msgout);
    planMAMsgDel(msgout);

    BOR_FREE(old_op);
}

static void heurMARequest(plan_heur_t *_heur, plan_ma_comm_t *comm,
                          const plan_ma_msg_t *msg)
{
    plan_heur_ma_lm_cut_t *heur = HEUR(_heur);
    private_t *private;
    int type, subtype;

    // Allocate private structure if needed
    allocPrivate(heur, planMAMsgAgent(msg));
    private = heur->private + planMAMsgAgent(msg);

    type = planMAMsgType(msg);
    subtype = planMAMsgSubType(msg);

    if (type == PLAN_MA_MSG_HEUR){
        if (subtype == PLAN_MA_MSG_HEUR_LM_CUT_INIT_REQUEST){
            privateInitRequest(private, comm, msg);
            return;

        }else if (subtype == PLAN_MA_MSG_HEUR_LM_CUT_MAX_REQUEST){
            privateMaxRequest(private, comm, msg);
            return;
        }
    }

    fprintf(stderr, "Error[%d]: The request isn't for lm-cut heuristic.\n",
            comm->node_id);
}
