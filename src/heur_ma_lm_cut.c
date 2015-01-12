#include <boruvka/alloc.h>
#include <boruvka/lifo.h>

#include "plan/heur.h"
#include "heur_relax.h"
#include "op_id_tr.h"

struct _private_t {
    plan_heur_relax_t relax;
    plan_op_id_tr_t op_id_tr;
    plan_oparr_t public_op; /*!< List of public operators */
    int *updated_ops;       /*!< Array for operators that were updated */
    plan_heur_relax_op_t *opcmp; /*!< Array for comparing operators before
                                      and after change */
};
typedef struct _private_t private_t;

struct _ma_lm_cut_op_t {
    int owner;
    int changed;
};
typedef struct _ma_lm_cut_op_t ma_lm_cut_op_t;

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
    plan_factarr_t init_state; /*!< Stored actual state for which the
                                    heuristic is computed */
    ma_lm_cut_op_t *op;        /*!< Local info about operators */
    int op_size;               /*!< Number of projected operators */
    plan_heur_relax_op_t *opcmp; /*!< Array for comparison of operators
                                      before and after a change */
    int *updated_ops;          /*!< Array for operators that were updated */

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

/** Updates op[] array with values from of operators from message if case
 *  the value is higher than the one in op[] array.
 *  If op_all is non-NULL values of all operators from message are written
 *  to this array.
 *  Arguments updated_ops and updated_ops_size are output values where are
 *  stored IDs of operators that were written to op[]. */
static void updateOpValueFromMsg(const plan_ma_msg_t *msg,
                                 const plan_op_id_tr_t *op_id_tr,
                                 plan_heur_relax_op_t *op,
                                 plan_heur_relax_op_t *op_all,
                                 int *updated_ops, int *updated_ops_size)
{
    int i, op_size, op_id;
    plan_cost_t op_value, value;

    *updated_ops_size = 0;

    op_size = planMAMsgOpSize(msg);
    for (i = 0; i < op_size; ++i){
        // Read operator data from message
        op_id = planMAMsgOp(msg, i, NULL, NULL, &op_value);

        // Translate operator ID from global ID to local ID and ignore
        // those which are unknown
        op_id = planOpIdTrLoc(op_id_tr, op_id);
        if (op_id < 0)
            continue;

        // Write operator's value if it is requested
        if (op_all)
            op_all[op_id].value = op_value;

        // Determine current value of operator
        value = op[op_id].value;

        // Record updated value of operator and write operator's ID to
        // output array
        if (value < op_value){
            op[op_id].value = op_value;
            updated_ops[*updated_ops_size] = op_id;
            ++(*updated_ops_size);
        }
    }
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

    private->updated_ops = BOR_ALLOC_ARR(int, private->relax.cref.op_size);
    private->opcmp = BOR_ALLOC_ARR(plan_heur_relax_op_t,
                                   private->relax.cref.op_size);
}

static void privateFree(private_t *private)
{
    planHeurRelaxFree(&private->relax);
    planOpIdTrFree(&private->op_id_tr);
    if (private->public_op.op)
        BOR_FREE(private->public_op.op);
    if (private->updated_ops)
        BOR_FREE(private->updated_ops);
    if (private->opcmp)
        BOR_FREE(private->opcmp);
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

static void initOp(plan_heur_ma_lm_cut_t *heur,
                   const plan_op_t *op, int op_size)
{
    int i;

    heur->op_size = op_size;
    heur->op = BOR_CALLOC_ARR(ma_lm_cut_op_t, heur->op_size);
    for (i = 0; i < op_size; ++i){
        heur->op[i].owner = op[i].owner;
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
    initOp(heur, prob->proj_op, prob->proj_op_size);
    heur->opcmp = BOR_ALLOC_ARR(plan_heur_relax_op_t,
                                heur->relax.cref.op_size);

    heur->init_state.size = prob->var_size;
    heur->init_state.fact = BOR_ALLOC_ARR(int, heur->init_state.size);
    heur->updated_ops = BOR_ALLOC_ARR(int, heur->relax.cref.op_size);

    heur->prob = prob;
    heur->private = NULL;
    heur->private_size = 0;

    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_ma_lm_cut_t *heur = HEUR(_heur);
    int i;

    if (heur->agent_changed)
        BOR_FREE(heur->agent_changed);
    if (heur->opcmp)
        BOR_FREE(heur->opcmp);
    if (heur->init_state.fact)
        BOR_FREE(heur->init_state.fact);
    if (heur->op)
        BOR_FREE(heur->op);

    if (heur->updated_ops)
        BOR_FREE(heur->updated_ops);

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
    int i, op_id, value;
    ma_lm_cut_op_t op;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_LM_CUT_MAX_REQUEST,
                       planMACommId(comm));

    for (i = 0; i < heur->op_size; ++i){
        op = heur->op[i];
        if (op.owner == agent_id && op.changed){
            value = heur->relax.op[i].value;
            op_id = planOpIdTrGlob(&heur->op_id_tr, i);
            planMAMsgHeurLMCutMaxAddOp(msg, op_id, value);

            heur->op[i].changed = 0;
        }
    }
    heur->agent_changed[agent_id] = 0;

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

    // Set all operators as changed
    for (i = 0; i < heur->op_size; ++i)
        heur->op[i].changed = 1;

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
    int i, size;

    // Remember old values of operators
    size = sizeof(plan_heur_relax_op_t) * heur->relax.cref.op_size;
    memcpy(heur->opcmp, heur->relax.op, size);

    // Update h^max heuristic using changed operators
    planHeurRelaxUpdateMaxFull(&heur->relax, update_op, update_op_size);

    // Record changes in operators
    for (i = 0; i < heur->op_size; ++i){
        if (heur->op[i].owner != heur->agent_id
                && !heur->op[i].changed
                && heur->opcmp[i].value != heur->relax.op[i].value){
            heur->op[i].changed = 1;
            if (heur->op[i].owner >= 0)
                heur->agent_changed[heur->op[i].owner] = 1;
        }
    }
}

static void updateHMax(plan_heur_ma_lm_cut_t *heur, const plan_ma_msg_t *msg)
{
    int updated_ops_size;

    // Update operators' values from message
    updateOpValueFromMsg(msg, &heur->op_id_tr, heur->relax.op, NULL,
                         heur->updated_ops, &updated_ops_size);

    // Early exit if no operators were changed
    if (updated_ops_size == 0)
        return;

    // Update .relax structure
    updateRelax(heur, heur->updated_ops, updated_ops_size);
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

static void privateMaxSendResponse(private_t *private, plan_ma_comm_t *comm,
                                   int agent_id)
{
    int i, op_id;
    plan_cost_t value;
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_LM_CUT_MAX_RESPONSE,
                       planMACommId(comm));

    // Check all public operators and send all that were changed
    for (i = 0; i < private->public_op.size; ++i){
        op_id = private->public_op.op[i];
        if (private->opcmp[op_id].value == private->relax.op[op_id].value)
            continue;

        value = private->relax.op[op_id].value;
        op_id = planOpIdTrGlob(&private->op_id_tr, op_id);
        planMAMsgHeurLMCutMaxAddOp(msg, op_id, value);
    }
    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static void privateMaxRequest(private_t *private, plan_ma_comm_t *comm,
                              const plan_ma_msg_t *msg)
{
    int updated_ops_size;

    // Early exit if we have no private operators
    if (private->relax.cref.op_size == 0){
        requestMaxSendEmptyResponse(comm, msg);
        return;
    }

    // Copy current state of operators to compare array.
    // This has to be done before relax->op values are changed according to
    // the received message because we need to compare on the state of
    // operators that is assumed by the initiator agent.
    memcpy(private->opcmp, private->relax.op,
           sizeof(plan_heur_relax_op_t) * private->relax.cref.op_size);

    // Update operators' values from received message
    updateOpValueFromMsg(msg, &private->op_id_tr,
                         private->relax.op, private->opcmp,
                         private->updated_ops, &updated_ops_size);

    // Update .relax structure if anything was changed
    if (updated_ops_size > 0){
        planHeurRelaxUpdateMaxFull(&private->relax, private->updated_ops,
                                   updated_ops_size);
    }

    // Send response to the main agent
    privateMaxSendResponse(private, comm, planMAMsgAgent(msg));
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
