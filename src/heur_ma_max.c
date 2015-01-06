#include <boruvka/alloc.h>
#include "plan/heur.h"
#include "plan/prioqueue.h"
#include "heur_relax.h"
#include "op_id_tr.h"

struct _private_t {
    plan_heur_relax_t relax;
    plan_factarr_t fact;
    plan_op_id_tr_t op_id_tr;
    int *fake_pre; /*!< Mapping between op_id and fake precodition ID */
};
typedef struct _private_t private_t;

struct _plan_heur_ma_max_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;
    plan_op_id_tr_t op_id_tr;

    int agent_id;
    plan_heur_relax_op_t *old_op;
    plan_oparr_t *agent_op;    /*!< Operators owned by a corresponding agent */
    int *agent_changed;        /*!< True/False for each agent signaling
                                    whether an operator changed */
    int agent_size;            /*!< Number of agents in cluster */
    int cur_agent_id;          /*!< ID of the agent from which a response
                                    is expected. */
    plan_factarr_t init_state; /*!< Stored actual state for which the
                                    heuristic is computed */
    int *is_init_fact;         /*!< True/False flag for each fact whether
                                    it belongs to the .init_state */
    int *op_owner;

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

static void initAgent(plan_heur_ma_max_t *heur, const plan_problem_t *prob)
{
    int i, j;
    const plan_op_t *op;
    plan_oparr_t *oparr;

    heur->agent_op = NULL;
    heur->agent_changed = NULL;
    heur->agent_size = 0;

    for (i = 0; i < prob->proj_op_size; ++i){
        op = prob->proj_op + i;

        if (op->owner >= heur->agent_size){
            heur->agent_op = BOR_REALLOC_ARR(heur->agent_op, plan_oparr_t,
                                             op->owner + 1);
            for (j = heur->agent_size; j < op->owner + 1; ++j){
                oparr = heur->agent_op + j;
                oparr->size = 0;
                oparr->op = BOR_ALLOC_ARR(int, prob->proj_op_size);
            }
            heur->agent_size = op->owner + 1;
        }

        oparr = heur->agent_op + op->owner;
        oparr->op[oparr->size++] = i;
    }

    for (i = 0; i < heur->agent_size; ++i){
        oparr = heur->agent_op + i;
        oparr->op = BOR_REALLOC_ARR(oparr->op, int, oparr->size);
    }

    heur->agent_changed = BOR_ALLOC_ARR(int, heur->agent_size);
    heur->old_op = BOR_ALLOC_ARR(plan_heur_relax_op_t,
                                 heur->relax.cref.op_size);
}

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

    private->fact.fact = BOR_ALLOC_ARR(int, private->relax.cref.fact_size);
    private->fact.size = 0;
    for (i = 0; i < private->relax.cref.fact_size; ++i){
        if (private->relax.cref.fact_eff[i].size > 0
                || private->relax.cref.fact_pre[i].size > 0){
            private->fact.fact[private->fact.size++] = i;
        }
    }

    private->fact.fact = BOR_REALLOC_ARR(private->fact.fact, int,
                                         private->fact.size);

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
    if (private->fact.fact)
        BOR_FREE(private->fact.fact);
    if (private->fake_pre)
        BOR_FREE(private->fake_pre);
    planOpIdTrFree(&private->op_id_tr);
}

plan_heur_t *planHeurMARelaxMaxNew(const plan_problem_t *prob)
{
    plan_heur_ma_max_t *heur;
    int i;

    heur = BOR_ALLOC(plan_heur_ma_max_t);
    _planHeurInit(&heur->heur, heurDel, NULL);
    _planHeurMAInit(&heur->heur, heurMAMax, heurMAMaxUpdate, heurMAMaxRequest);
    planHeurRelaxInit(&heur->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      prob->var, prob->var_size, prob->goal,
                      prob->proj_op, prob->proj_op_size);
    planOpIdTrInit(&heur->op_id_tr, prob->proj_op, prob->proj_op_size);

    initAgent(heur, prob);
    heur->init_state.size = prob->var_size;
    heur->init_state.fact = BOR_ALLOC_ARR(int, heur->init_state.size);
    heur->is_init_fact = BOR_ALLOC_ARR(int, heur->relax.cref.fact_size);

    heur->op_owner = BOR_ALLOC_ARR(int, prob->proj_op_size);
    for (i = 0; i < prob->proj_op_size; ++i)
        heur->op_owner[i] = prob->proj_op[i].owner;

    privateInit(&heur->private, prob);

    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    int i;
    plan_heur_ma_max_t *heur = HEUR(_heur);

    BOR_FREE(heur->agent_changed);
    for (i = 0; i < heur->agent_size; ++i){
        if (heur->agent_op[i].op)
            BOR_FREE(heur->agent_op[i].op);
    }
    BOR_FREE(heur->agent_op);
    BOR_FREE(heur->old_op);

    BOR_FREE(heur->init_state.fact);
    BOR_FREE(heur->is_init_fact);
    BOR_FREE(heur->op_owner);

    privateFree(&heur->private);

    planOpIdTrFree(&heur->op_id_tr);
    planHeurRelaxFree(&heur->relax);
    _planHeurFree(&heur->heur);
    BOR_FREE(heur);
}


static void setInitState(plan_heur_ma_max_t *heur, const plan_state_t *state)
{
    int i, fact_id;

    bzero(heur->is_init_fact, sizeof(int) * heur->relax.cref.fact_size);
    for (i = 0; i < heur->init_state.size; ++i){
        heur->init_state.fact[i] = state->val[i];
        fact_id = planFactId(&heur->relax.cref.fact_id, i, state->val[i]);
        heur->is_init_fact[fact_id] = 1;
    }
}

static void sendRequest(plan_heur_ma_max_t *heur, plan_ma_comm_t *comm,
                        int agent_id)
{
    plan_ma_msg_t *msg;
    plan_oparr_t *oparr;
    int i, op_id, value;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_MAX_REQUEST,
                       planMACommId(comm));
    planMAMsgHeurMaxSetRequest(msg, heur->init_state.fact,
                               heur->init_state.size);

    oparr = heur->agent_op + agent_id;
    for (i = 0; i < oparr->size; ++i){
        op_id = planOpIdTrGlob(&heur->op_id_tr, oparr->op[i]);
        value = heur->relax.op[oparr->op[i]].value;
        planMAMsgHeurMaxAddOp(msg, op_id, value);
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

static void updateHMax(plan_heur_ma_max_t *heur, const plan_ma_msg_t *msg)
{
    int i, *update_op, update_op_size, op_id, value;
    int response_op_size, response_op_id, response_value;

    response_op_size = planMAMsgHeurMaxOpSize(msg);
    update_op = alloca(sizeof(int) * response_op_size);
    update_op_size = 0;
    for (i = 0; i < response_op_size; ++i){
        // Read data for operator
        response_op_id = planMAMsgHeurMaxOp(msg, i, &response_value);

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

    // Remember old values of operators
    memcpy(heur->old_op, heur->relax.op,
           sizeof(plan_heur_relax_op_t) * heur->relax.cref.op_size);

    // Update h^max heuristic using changed operators
    planHeurRelaxUpdateMaxFull(&heur->relax, update_op, update_op_size);

    // Find out which agent has changed operator
    for (i = 0; i < heur->relax.cref.op_size; ++i){
        if (heur->old_op[i].value != heur->relax.op[i].value){
            int op_id = heur->relax.cref.op_id[i];
            if (op_id >= 0){
                int owner = heur->op_owner[op_id];
                if (owner != heur->agent_id)
                    heur->agent_changed[owner] = 1;
            }
        }
    }
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

    if (!needsUpdate(heur)){
        res->heur = heur->relax.fact[heur->relax.cref.goal_id].value;
        return 0;
    }

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
    op_len = planMAMsgHeurMaxOpSize(msg);
    for (i = 0; i < op_len; ++i){
        op_id = planMAMsgHeurMaxOp(msg, i, &op_value);
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
    planMAMsgHeurMaxState(msg, &state);
    planHeurRelaxFull(&private->relax, &state);

    // Send operator's new values back as response
    requestSendResponse(private, comm, &private->op_id_tr, msg);
}
