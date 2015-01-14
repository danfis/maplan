#include <boruvka/alloc.h>
#include <boruvka/lifo.h>

#include "plan/heur.h"
#include "heur_relax.h"
#include "op_id_tr.h"

#define STATE_INIT             0
#define STATE_GOAL_ZONE        1
#define STATE_GOAL_ZONE_UPDATE 2
#define STATE_FIND_CUT         3
#define STATE_FIND_CUT_UPDATE  4
#define STATE_CUT              5
#define STATE_HMAX             6
#define STATE_HMAX_UPDATE      7
#define STATE_FINISH           8
#define STATE_SIZE             9

typedef struct _plan_heur_ma_lm_cut_t plan_heur_ma_lm_cut_t;
typedef int (*state_step_fn)(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg);

/** Cut op/fact states: */
#define CUT_GOAL_ZONE 1
#define CUT_START_ZONE 2
#define CUT_OP_DONE 3

struct _cut_op_t {
    int state;
    int in_cut;
};
typedef struct _cut_op_t cut_op_t;

struct _cut_fact_t {
    int state;
};
typedef struct _cut_fact_t cut_fact_t;

struct _cut_t {
    cut_op_t *op;
    cut_fact_t *fact;
    plan_cost_t min_cut;
};
typedef struct _cut_t cut_t;

/** Initialize cut_t structure */
static void cutInit(cut_t *cut, const plan_heur_relax_t *relax);
/** Frees allocated resources */
static void cutFree(cut_t *cut);
/** Initializes cut_t structure to initial values */
static void cutInitCycle(cut_t *cut, const plan_heur_relax_t *relax);
/** Marks goal zone in cut_t structure */
static void cutMarkGoalZone(cut_t *cut, const plan_heur_relax_t *relax,
                            int goal_id);
/** Performs find-cut procedure from the specified fact */
static void cutFind(cut_t *cut, const plan_heur_relax_t *relax, int start_fact);
/** Performs find-cut procedure from the specified operator */
static void cutFindOp(cut_t *cut, const plan_heur_relax_t *relax, int op_id);


struct _private_t {
    plan_heur_relax_t relax;
    plan_op_id_tr_t op_id_tr;
    plan_oparr_t public_op; /*!< List of public operators */
    int *updated_ops;       /*!< Array for operators that were updated */
    plan_heur_relax_op_t *opcmp; /*!< Array for comparing operators before
                                      and after change */

    cut_t cut;
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

    int state;
    state_step_fn *state_step; /*!< Array of functions assigned to each state */
    plan_cost_t heur_value;    /*!< Value of the heuristic */
    int agent_id;
    int agent_size;            /*!< Number of agents in cluster */
    int *agent_changed;        /*!< True/False for each agent signaling
                                    whether an operator changed */
    int cur_agent_id;          /*!< ID of the agent from which a response
                                    is expected. */
    plan_state_t *init_state;  /*!< Stored actual state for which the
                                    heuristic is computed */
    ma_lm_cut_op_t *op;        /*!< Local info about operators */
    int op_size;               /*!< Number of projected operators */
    plan_heur_relax_op_t *opcmp; /*!< Array for comparison of operators
                                      before and after a change */
    int *updated_ops;          /*!< Array for operators that were updated */
    cut_t cut;                 /*!< Structure for finding cut */

    const plan_problem_t *prob;
    private_t *private;
    int private_size;
};

#define HEUR(parent) \
    bor_container_of(parent, plan_heur_ma_lm_cut_t, heur)

static void debugRelax(const plan_heur_relax_t *relax, int agent_id,
                       const plan_op_id_tr_t *op_id_tr, const char *name);
static void debugCut(const cut_t *cut, const plan_heur_relax_t *relax,
                     int agent_id, const plan_op_id_tr_t *op_id_tr,
                     const char *name);

static void heurDel(plan_heur_t *_heur);
static int heurMA(plan_heur_t *heur, plan_ma_comm_t *comm,
                  const plan_state_t *state, plan_heur_res_t *res);
static int heurMAUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg, plan_heur_res_t *res);
static void heurMARequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                          const plan_ma_msg_t *msg);

static int mainLoop(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                    const plan_ma_msg_t *msg, plan_heur_res_t *res);
static int stepInit(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                    const plan_ma_msg_t *msg);
static int stepGoalZone(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg);
static int stepGoalZoneUpdate(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                              const plan_ma_msg_t *msg);
static int stepFindCut(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                       const plan_ma_msg_t *msg);
static int stepFindCutUpdate(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg);
static int stepCut(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                   const plan_ma_msg_t *msg);
static int stepHMax(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                    const plan_ma_msg_t *msg);
static int stepHMaxUpdate(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                          const plan_ma_msg_t *msg);
static int stepFinish(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
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

    cutInit(&private->cut, &private->relax);
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
    cutFree(&private->cut);
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

static int checkConditionalEffects(const plan_op_t *op, int op_size)
{
    int i;

    for (i = 0; i < op_size; ++i){
        if (op[i].cond_eff_size > 0)
            return 1;
    }
    return 0;
}

plan_heur_t *planHeurMALMCutNew(const plan_problem_t *prob)
{
    plan_heur_ma_lm_cut_t *heur;

    if (checkConditionalEffects(prob->proj_op, prob->proj_op_size)){
        fprintf(stderr, "Error: ma-lm-cut heuristic cannot be run on"
                        " operators with conditional effects.\n");
        return NULL;
    }

    heur = BOR_ALLOC(plan_heur_ma_lm_cut_t);
    _planHeurInit(&heur->heur, heurDel, NULL);
    _planHeurMAInit(&heur->heur, heurMA, heurMAUpdate, heurMARequest);
    planHeurRelaxInit(&heur->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      prob->var, prob->var_size, prob->goal,
                      prob->proj_op, prob->proj_op_size);
    planOpIdTrInit(&heur->op_id_tr, prob->proj_op, prob->proj_op_size);

    heur->state = STATE_INIT;
    heur->state_step = BOR_CALLOC_ARR(state_step_fn, STATE_SIZE);
    heur->state_step[STATE_INIT] = stepInit;
    heur->state_step[STATE_GOAL_ZONE] = stepGoalZone;
    heur->state_step[STATE_GOAL_ZONE_UPDATE] = stepGoalZoneUpdate;
    heur->state_step[STATE_FIND_CUT] = stepFindCut;
    heur->state_step[STATE_FIND_CUT_UPDATE] = stepFindCutUpdate;
    heur->state_step[STATE_CUT] = stepCut;
    heur->state_step[STATE_HMAX] = stepHMax;
    heur->state_step[STATE_HMAX_UPDATE] = stepHMaxUpdate;
    heur->state_step[STATE_FINISH] = stepFinish;

    heur->agent_size = agentSize(prob->proj_op, prob->proj_op_size);
    heur->agent_changed = BOR_CALLOC_ARR(int, heur->agent_size);
    initOp(heur, prob->proj_op, prob->proj_op_size);
    heur->opcmp = BOR_ALLOC_ARR(plan_heur_relax_op_t,
                                heur->relax.cref.op_size);

    heur->init_state = planStateNew(prob->state_pool);
    heur->updated_ops = BOR_ALLOC_ARR(int, heur->relax.cref.op_size);

    cutInit(&heur->cut, &heur->relax);

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
    if (heur->init_state)
        planStateDel(heur->init_state);
    if (heur->op)
        BOR_FREE(heur->op);
    if (heur->state_step)
        BOR_FREE(heur->state_step);

    if (heur->updated_ops)
        BOR_FREE(heur->updated_ops);

    cutFree(&heur->cut);

    for (i = 0; i < heur->private_size; ++i)
        privateFree(heur->private + i);
    if (heur->private)
        BOR_FREE(heur->private);

    planOpIdTrFree(&heur->op_id_tr);
    planHeurRelaxFree(&heur->relax);
    _planHeurFree(&heur->heur);
    BOR_FREE(heur);
}

static int heurMA(plan_heur_t *_heur, plan_ma_comm_t *comm,
                  const plan_state_t *state, plan_heur_res_t *res)
{
    plan_heur_ma_lm_cut_t *heur = HEUR(_heur);

    heur->heur_value = 0;

    // Store state for which we are evaluating heuristics
    planStateCopy(heur->init_state, state);

    // Remember ID of this agent
    heur->agent_id = planMACommId(comm);

    // Set up first state
    heur->state = STATE_INIT;

    return mainLoop(heur, comm, NULL, res);
}

static int heurMAUpdate(plan_heur_t *_heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    plan_heur_ma_lm_cut_t *heur = HEUR(_heur);
    return mainLoop(heur, comm, msg, res);
}





static void sendInitRequest(plan_heur_ma_lm_cut_t *heur,
                            plan_ma_comm_t *comm, int agent_id)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_LM_CUT_INIT_REQUEST,
                       planMACommId(comm));
    planMAMsgHeurLMCutSetInitRequest(msg, heur->init_state);
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




static int nextAgentId(const plan_heur_ma_lm_cut_t *heur)
{
    int from = (heur->cur_agent_id + 1) % heur->agent_size;
    int to = heur->cur_agent_id;
    int i;

    for (i = from; i != to; i = (i + 1) % heur->agent_size){
        if (heur->agent_changed[i])
            return i;
    }
    if (heur->agent_changed[to])
        return to;
    return -1;
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

static void sendSuppRequest(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg[heur->agent_size];
    int agent_id, i, op_id;

    // Create one message per remote agent
    for (agent_id = 0; agent_id < heur->agent_size; ++agent_id){
        if (agent_id == heur->agent_id){
            msg[agent_id] = NULL;
            continue;
        }

        msg[agent_id] = planMAMsgNew(PLAN_MA_MSG_HEUR,
                                     PLAN_MA_MSG_HEUR_LM_CUT_SUPP_REQUEST,
                                     planMACommId(comm));
    }

    // Fill message for each agent with the agent's operators that have
    // supporter facts know to this (main) agent.
    for (i = 0; i < heur->op_size; ++i){
        agent_id = heur->op[i].owner;
        if (agent_id != heur->agent_id && heur->relax.op[i].supp >= 0){
            op_id = planOpIdTrGlob(&heur->op_id_tr, i);
            if (op_id >= 0)
                planMAMsgHeurLMCutSuppAddOp(msg[agent_id], op_id);
        }
    }

    // Send message to all remote agents
    for (agent_id = 0; agent_id < heur->agent_size; ++agent_id){
        if (msg[agent_id]){
            planMACommSendToNode(comm, agent_id, msg[agent_id]);
            planMAMsgDel(msg[agent_id]);
        }
    }
}


static int mainLoop(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                    const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    while (heur->state_step[heur->state](heur, comm, msg) == 0);

    if (heur->state == STATE_FINISH){
        res->heur = heur->heur_value;
        return 0;
    }

    return -1;
}

static int stepInit(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                    const plan_ma_msg_t *msg)
{
    int i;

    // Every agent must be requested at least once, so make sure of that.
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
    planHeurRelaxFull(&heur->relax, heur->init_state);

    // Send request to next agent
    sendMaxRequest(heur, comm, heur->cur_agent_id);

    heur->state = STATE_HMAX_UPDATE;
    return -1;
}

static int stepGoalZone(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
    cutInitCycle(&heur->cut, &heur->relax);

    cutMarkGoalZone(&heur->cut, &heur->relax, heur->relax.cref.goal_id);

    // TODO: Send requests

    debugCut(&heur->cut, &heur->relax, comm->node_id, &heur->op_id_tr, "Main");

    heur->state = STATE_FIND_CUT;
    //heur->state = STATE_FINISH;
    return 0;
}

static int stepGoalZoneUpdate(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                              const plan_ma_msg_t *msg)
{
    // TODO
    return -1;
}

static int sendFindCutRequests(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                               int add_state)
{
    plan_ma_msg_t *msg[heur->agent_size];
    int i, op_id, agent_id;
    int num_sent = 0;

    bzero(heur->agent_changed, sizeof(int) * heur->agent_size);

    for (agent_id = 0; agent_id < heur->agent_size; ++agent_id){
        msg[agent_id] = NULL;
        if (agent_id != heur->agent_id){
            msg[agent_id] = planMAMsgNew(PLAN_MA_MSG_HEUR,
                                         PLAN_MA_MSG_HEUR_LM_CUT_FIND_CUT_REQUEST,
                                         planMACommId(comm));
            if (add_state){
                planMAMsgHeurLMCutFindCutSetState(msg[agent_id], heur->init_state);
                heur->agent_changed[agent_id] = 1;
            }
        }
    }

    for (i = 0; i < heur->op_size; ++i){
        if (heur->cut.op[i].state == CUT_START_ZONE){
            heur->cut.op[i].state = CUT_OP_DONE;

            agent_id = heur->op[i].owner;
            if (agent_id == heur->agent_id)
                continue;

            op_id = planOpIdTrGlob(&heur->op_id_tr, i);
            planMAMsgHeurLMCutFindCutAddOp(msg[agent_id], op_id);
            heur->agent_changed[agent_id] = 1;
        }
    }

    for (agent_id = 0; agent_id < heur->agent_size; ++agent_id){
        if (heur->agent_changed[agent_id]){
            planMACommSendToNode(comm, agent_id, msg[agent_id]);
            ++num_sent;
        }

        if (msg[agent_id])
            planMAMsgDel(msg[agent_id]);
    }

    return num_sent;
}

static int stepFindCut(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                       const plan_ma_msg_t *msg)
{
    int i, fact_id;

    // Find cut from all facts in initial state and all fake preconditions
    for (i = 0; i < heur->init_state->num_vars; ++i){
        fact_id = planFactId(&heur->relax.cref.fact_id, i,
                             planStateGet(heur->init_state, i));
        cutFind(&heur->cut, &heur->relax, fact_id);
    }
    for (i = 0; i < heur->relax.cref.fake_pre_size; ++i){
        cutFind(&heur->cut, &heur->relax,
                heur->relax.cref.fake_pre[i].fact_id);
    }

    debugCut(&heur->cut, &heur->relax, comm->node_id, &heur->op_id_tr, "MainFindCut");
    debugRelax(&heur->relax, comm->node_id, &heur->op_id_tr, "MainRelax");

    // Send request
    sendFindCutRequests(heur, comm, 1);
    heur->state = STATE_FIND_CUT_UPDATE;
    return -1;
}

static int stepFindCutUpdate(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg)
{
    int i, size, op_id;
    int from_agent;
    
    // Update minimal cost of cut from message
    heur->cut.min_cut = BOR_MIN(heur->cut.min_cut, planMAMsgMinCutCost(msg));

    // Proceed in exploring justification graph from the received operators
    size = planMAMsgOpSize(msg);
    for (i = 0; i < size; ++i){
        op_id = planMAMsgOp(msg, i, NULL, NULL, NULL);
        op_id = planOpIdTrLoc(&heur->op_id_tr, op_id);
        if (op_id < 0)
            continue;

        cutFindOp(&heur->cut, &heur->relax, op_id);
    }

    // Mark agent as processed
    from_agent = planMAMsgAgent(msg);
    heur->agent_changed[from_agent] = 0;

    debugCut(&heur->cut, &heur->relax, comm->node_id, &heur->op_id_tr, "MainFindCutUpdate");

    // If messages from all agents were processed, try to send another
    // bulk of messages if anything changed. If nothing changed,
    // proceed to next phase.
    heur->cur_agent_id = heur->agent_id;
    if (nextAgentId(heur) < 0){
        if (sendFindCutRequests(heur, comm, 0) == 0){
            heur->state = STATE_CUT;
            return 0;
        }
    }

    return -1;
}

static void sendCutRequests(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg[heur->agent_size];
    ma_lm_cut_op_t *op;
    int agent_id, i, op_id;

    fprintf(stderr, "sendCutRequests\n");
    fflush(stderr);
    for (agent_id = 0; agent_id < heur->agent_size; ++agent_id){
        msg[agent_id] = NULL;
        if (agent_id == heur->agent_id)
            continue;

        msg[agent_id] = planMAMsgNew(PLAN_MA_MSG_HEUR,
                                     PLAN_MA_MSG_HEUR_LM_CUT_CUT_REQUEST,
                                     planMACommId(comm));
        planMAMsgSetMinCutCost(msg[agent_id], heur->cut.min_cut);
    }

    for (i = 0; i < heur->op_size; ++i){
        op = heur->op + i;
        if (op->owner == heur->agent_id)
            continue;

        if (heur->cut.op[i].in_cut){
            op_id = planOpIdTrGlob(&heur->op_id_tr, i);
            // TODO
            planMAMsgAddOp(msg[op->owner], op_id, PLAN_COST_INVALID, -1,
                           PLAN_COST_INVALID);
        }
    }

    for (agent_id = 0; agent_id < heur->agent_size; ++agent_id){
        if (agent_id == heur->agent_id)
            continue;
        planMACommSendToNode(comm, agent_id, msg[agent_id]);
        planMAMsgDel(msg[agent_id]);
    }
}

static int stepCut(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                   const plan_ma_msg_t *msg)
{
    sendCutRequests(heur, comm);
    heur->heur_value += heur->cut.min_cut;
    heur->state = STATE_HMAX;
    return 0;
}

static int stepHMax(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                    const plan_ma_msg_t *msg)
{
    fprintf(stderr, "stepHMax\n");
    fflush(stderr);
    heur->state = STATE_FINISH;
    return 0;
}

static int stepHMaxUpdate(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                          const plan_ma_msg_t *msg)
{
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
    heur->cur_agent_id = nextAgentId(heur);
    if (heur->cur_agent_id >= 0){
        sendMaxRequest(heur, comm, heur->cur_agent_id);
    }else{
        sendSuppRequest(heur, comm);
        heur->state = STATE_GOAL_ZONE;
        return 0;
    }

    return -1;
}

static int stepFinish(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg)
{
    heur->heur_value = heur->relax.fact[heur->relax.cref.goal_id].value;
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

    //debugRelax(&private->relax, comm->node_id, &private->op_id_tr, "Private");
}

static void privateSuppRequest(private_t *private, plan_ma_comm_t *comm,
                               const plan_ma_msg_t *msg)
{
    int i, size, op_id;

    // Mark all received operators as without supporter fact because the
    // supporter fact is hold by the main agent.
    size = planMAMsgHeurLMCutSuppOpSize(msg);
    for (i = 0; i < size; ++i){
        op_id = planMAMsgHeurLMCutSuppOp(msg, i);
        op_id = planOpIdTrLoc(&private->op_id_tr, op_id);
        if (op_id >= 0)
            private->relax.op[op_id].supp = -1;
    }

    // Initialize cut_t structure it will be needed in next phases
    cutInitCycle(&private->cut, &private->relax);

#if 0 /* DEBUG */
    // Check that all operators have support either in this agent or in the
    // main agent
    int j;
    for (j = 0; j < private->op_id_tr.loc_to_glob_size; ++j){
        int supp = private->relax.op[j].supp;
        int found = 0;

        for (i = 0; i < size; ++i){
            op_id = planMAMsgHeurLMCutSuppOp(msg, i);
            op_id = planOpIdTrLoc(&private->op_id_tr, op_id);
            if (op_id == j){
                found = 1;
                if (supp != -1){
                    fprintf(stderr, "[%d] Error: Supporter for %d(%d) on"
                                    " both agents.\n",
                            comm->node_id, op_id,
                            planMAMsgHeurLMCutSuppOp(msg, i));
                    break;
                }
            }
        }

        if (!found && supp == -1 && private->relax.op[j].unsat == 0){
            fprintf(stderr, "[%d] Error: Supporter for %d(%d) is -1 on both"
                            " agents.\n",
                    comm->node_id, j, planOpIdTrGlob(&private->op_id_tr, j));
        }
    }
#endif
}

static void sendFindCutResponse(private_t *private, plan_ma_comm_t *comm,
                                int agent_id)
{
    plan_ma_msg_t *msg;
    int i, len, *ops, op_id;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_LM_CUT_FIND_CUT_RESPONSE,
                       planMACommId(comm));

    len = private->public_op.size;
    ops = private->public_op.op;
    for (i = 0; i < len; ++i){
        op_id = ops[i];
        if (private->cut.op[op_id].state == CUT_START_ZONE){
            op_id = private->relax.cref.op_id[op_id];
            if (op_id < 0)
                continue;
            op_id = planOpIdTrGlob(&private->op_id_tr, op_id);
            // TODO: Better API in ma_msg module
            planMAMsgAddOp(msg, op_id, PLAN_COST_INVALID, -1,
                           PLAN_COST_INVALID);
        }
    }

    // Set minimal cost of cut so far
    planMAMsgSetMinCutCost(msg, private->cut.min_cut);

    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static void privateFindCutRequest(private_t *private, plan_ma_comm_t *comm,
                                  const plan_ma_msg_t *msg)
{
    int i, size, fact_id, op_id;

    if (planMAMsgHasStateFull(msg)){
        for (i = 0; i < private->relax.cref.fact_id.var_size; ++i){
            fact_id = planMAMsgStateFullVal(msg, i);
            fact_id = planFactId(&private->relax.cref.fact_id, i, fact_id);
            cutFind(&private->cut, &private->relax, fact_id);
        }
        for (i = 0; i < private->relax.cref.fake_pre_size; ++i){
            fact_id = private->relax.cref.fake_pre[i].fact_id;
            cutFind(&private->cut, &private->relax, fact_id);
        }
    }

    size = planMAMsgOpSize(msg);
    for (i = 0; i < size; ++i){
        op_id = planMAMsgOp(msg, i, NULL, NULL, NULL);
        op_id = planOpIdTrLoc(&private->op_id_tr, op_id);
        if (op_id < 0)
            continue;

        cutFindOp(&private->cut, &private->relax, op_id);
    }

    debugRelax(&private->relax, comm->node_id, &private->op_id_tr, "Private");
    debugCut(&private->cut, &private->relax, comm->node_id, &private->op_id_tr, "Private");

    sendFindCutResponse(private, comm, planMAMsgAgent(msg));
}

static void privateCutRequest(private_t *private, plan_ma_comm_t *comm,
                              const plan_ma_msg_t *msg)
{
    int i, size, op_id;

    size = planMAMsgOpSize(msg);
    for (i = 0; i < size; ++i){
        op_id = planMAMsgOp(msg, i, NULL, NULL, NULL);
        op_id = planOpIdTrLoc(&private->op_id_tr, op_id);
        if (op_id < 0)
            continue;

        private->cut.op[op_id].in_cut = 1;
    }

    private->cut.min_cut = planMAMsgMinCutCost(msg);

    debugRelax(&private->relax, comm->node_id, &private->op_id_tr, "PrivateCut");
    debugCut(&private->cut, &private->relax, comm->node_id, &private->op_id_tr, "PrivateCut");
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

        }else if(subtype == PLAN_MA_MSG_HEUR_LM_CUT_SUPP_REQUEST){
            privateSuppRequest(private, comm, msg);
            return;

        }else if(subtype == PLAN_MA_MSG_HEUR_LM_CUT_FIND_CUT_REQUEST){
            privateFindCutRequest(private, comm, msg);
            return;

        }else if(subtype == PLAN_MA_MSG_HEUR_LM_CUT_CUT_REQUEST){
            privateCutRequest(private, comm, msg);
            return;
        }
    }

    fprintf(stderr, "Error[%d]: The request isn't for lm-cut heuristic.\n",
            comm->node_id);
}


/*** CUT ***/
static void cutInit(cut_t *cut, const plan_heur_relax_t *relax)
{
    cut->op = BOR_ALLOC_ARR(cut_op_t, relax->cref.op_size);
    cut->fact = BOR_ALLOC_ARR(cut_fact_t, relax->cref.fact_size);
}

static void cutFree(cut_t *cut)
{
    if (cut->op)
        BOR_FREE(cut->op);
    if (cut->fact)
        BOR_FREE(cut->fact);
}

static void cutInitCycle(cut_t *cut, const plan_heur_relax_t *relax)
{
    bzero(cut->op, sizeof(cut_op_t) * relax->cref.op_size);
    bzero(cut->fact, sizeof(cut_fact_t) * relax->cref.fact_size);
    cut->min_cut = PLAN_COST_MAX;
}

static void cutMarkGoalZone(cut_t *cut, const plan_heur_relax_t *relax,
                            int goal_id)
{
    bor_lifo_t lifo;
    int fact_id;
    int i, len, *ops, op_id;
    plan_heur_relax_op_t *op;

    borLifoInit(&lifo, sizeof(int));
    cut->fact[goal_id].state = CUT_GOAL_ZONE;
    borLifoPush(&lifo, &goal_id);
    while (!borLifoEmpty(&lifo)){
        fact_id = *(int *)borLifoBack(&lifo);
        borLifoPop(&lifo);

        len = relax->cref.fact_eff[fact_id].size;
        ops = relax->cref.fact_eff[fact_id].op;
        for (i = 0; i < len; ++i){
            op_id = ops[i];
            op = relax->op + op_id;
            if (cut->op[op_id].state == 0 && op->cost == 0){
                cut->op[op_id].state = CUT_GOAL_ZONE;
                if (op->supp >= 0 && cut->fact[op->supp].state == 0){
                    cut->fact[op->supp].state = CUT_GOAL_ZONE;
                    borLifoPush(&lifo, &op->supp);
                }
            }
        }
    }

    borLifoFree(&lifo);
}

static void cutFindAddEff(cut_t *cut, const plan_heur_relax_t *relax,
                          int op_id, bor_lifo_t *lifo)
{
    int i, len, *facts, fact_id;

    len = relax->cref.op_eff[op_id].size;
    facts = relax->cref.op_eff[op_id].fact;
    for (i = 0; i < len; ++i){
        fact_id = facts[i];
        if (cut->fact[fact_id].state == CUT_GOAL_ZONE){
            cut->op[op_id].in_cut = 1;
            cut->min_cut = BOR_MIN(cut->min_cut, relax->op[op_id].cost);

        }else if (cut->fact[fact_id].state == 0){
            cut->fact[fact_id].state = CUT_START_ZONE;
            borLifoPush(lifo, &fact_id);
        }
    }
}

static void cutFindLoop(cut_t *cut, const plan_heur_relax_t *relax,
                        bor_lifo_t *lifo)
{
    int fact_id;
    int i, len, *ops, op_id;
    plan_heur_relax_op_t *op;

    while (!borLifoEmpty(lifo)){
        fact_id = *(int *)borLifoBack(lifo);
        borLifoPop(lifo);

        len = relax->cref.fact_pre[fact_id].size;
        ops = relax->cref.fact_pre[fact_id].op;
        for (i = 0; i < len; ++i){
            op_id = ops[i];
            op = relax->op + op_id;
            if (cut->op[op_id].state == 0 && op->supp == fact_id){
                cut->op[op_id].state = CUT_START_ZONE;
                cutFindAddEff(cut, relax, op_id, lifo);
            }
        }
    }
}

static void cutFind(cut_t *cut, const plan_heur_relax_t *relax, int start_fact)
{
    bor_lifo_t lifo;

    if (cut->fact[start_fact].state != 0)
        return;

    borLifoInit(&lifo, sizeof(int));
    cut->fact[start_fact].state = CUT_START_ZONE;
    borLifoPush(&lifo, &start_fact);
    cutFindLoop(cut, relax, &lifo);
    borLifoFree(&lifo);
}

static void cutFindOp(cut_t *cut, const plan_heur_relax_t *relax, int op_id)
{
    bor_lifo_t lifo;

    if (cut->op[op_id].state != 0)
        return;

    borLifoInit(&lifo, sizeof(int));
    // Don't send back operator that was already received, so mark it
    // as done.
    cut->op[op_id].state = CUT_OP_DONE;
    cutFindAddEff(cut, relax, op_id, &lifo);
    cutFindLoop(cut, relax, &lifo);
    borLifoFree(&lifo);
}
/*** CUT END ***/



/*** DEBUG ***/
static void debugRelax(const plan_heur_relax_t *relax, int agent_id,
                       const plan_op_id_tr_t *op_id_tr, const char *name)
{
    int i, j;

    fprintf(stderr, "[%d] %s\n", agent_id, name);
    for (i = 0; i < relax->cref.op_size; ++i){
        fprintf(stderr, "[%d] Op[%d] value: %d, supp: %d",
                agent_id,
                planOpIdTrGlob(op_id_tr, relax->cref.op_id[i]),
                relax->op[i].value,
                relax->op[i].supp);

        fprintf(stderr, " Pre[");
        for (j = 0; j < relax->cref.op_pre[i].size; ++j){
            fprintf(stderr, " %d:%d",
                    relax->cref.op_pre[i].fact[j],
                    relax->fact[relax->cref.op_pre[i].fact[j]].value);
        }
        fprintf(stderr, "]\n");
    }

    for (i = 0; i < relax->cref.fact_size; ++i){
        fprintf(stderr, "[%d] Fact[%d] value: %d, supp: %d",
                agent_id, i,
                relax->fact[i].value,
                relax->fact[i].supp);

        fprintf(stderr, " Eff[");
        for (j = 0; j < relax->cref.fact_eff[i].size; ++j){
            fprintf(stderr, " %d:%d",
                    relax->cref.fact_eff[i].op[j],
                    relax->op[relax->cref.fact_eff[i].op[j]].value);
        }
        fprintf(stderr, "]\n");
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}

static void debugCut(const cut_t *cut, const plan_heur_relax_t *relax,
                     int agent_id, const plan_op_id_tr_t *op_id_tr,
                     const char *name)
{
    int i;

    fprintf(stderr, "[%d] %s\n", agent_id, name);
    fprintf(stderr, "[%d] min_cut: %d\n", agent_id, (int)cut->min_cut);
    for (i = 0; i < relax->cref.op_size; ++i){
        fprintf(stderr, "[%d] Op[%d] state: %d, in_cut: %d\n",
                agent_id, planOpIdTrGlob(op_id_tr, i),
                cut->op[i].state,
                cut->op[i].in_cut);
    }
    for (i = 0; i < relax->cref.fact_size; ++i){
        fprintf(stderr, "[%d] Fact[%d] state: %d\n",
                agent_id, i, cut->fact[i].state);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}
/*** DEBUG END ***/
