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
#include <boruvka/lifo.h>

#include "plan/heur.h"
#include "heur_relax.h"
#include "op_id_tr.h"

#define PLAN_COST_UNREACHABLE (PLAN_COST_MAX / 2)

#define STATE_INIT             0
#define STATE_GOAL_ZONE        1
#define STATE_GOAL_ZONE_UPDATE 2
#define STATE_FIND_CUT         3
#define STATE_FIND_CUT_UPDATE  4
#define STATE_CUT              5
#define STATE_CUT_UPDATE       6
#define STATE_HMAX             7
#define STATE_HMAX_UPDATE      8
#define STATE_SIZE             9
#define STATE_FINISH           10

/** Cut op/fact states: */
#define CUT_GOAL_ZONE  1
#define CUT_START_ZONE 2
#define CUT_OP_DONE    3

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
/** Marks goal zone starting from the specified operator */
static void cutMarkGoalZoneOp(cut_t *cut, const plan_heur_relax_t *relax,
                              int op_id);
/** Performs find-cut procedure from the specified fact */
static void cutFind(cut_t *cut, const plan_heur_relax_t *relax,
                    int start_fact, int is_init);
/** Performs find-cut procedure from the specified operator */
static void cutFindOp(cut_t *cut, const plan_heur_relax_t *relax, int op_id);
/** Applies cut to the relaxed heur struct, returns IDs of operators that
 *  were changed */
static void cutApply(cut_t *cut, plan_heur_relax_t *relax,
                     int *updated_ops, int *updated_ops_size);


struct _private_t {
    plan_ma_state_t *ma_state;
    plan_heur_relax_t relax;
    plan_op_id_tr_t op_id_tr;
    plan_arr_int_t public_op; /*!< List of public operators */
    int *updated_ops;       /*!< Array for operators that were updated */
    plan_heur_relax_op_t *opcmp; /*!< Array for comparing operators before
                                      and after change */

    cut_t cut;
};
typedef struct _private_t private_t;

/** Initialize private_t structure */
static void privateInit(private_t *private,
                        plan_ma_state_t *ma_state,
                        const plan_problem_t *prob);
/** Free allocated resources */
static void privateFree(private_t *private);

/** Methods for processing specific REQUEST messages */
static void privateInitStep(private_t *private, plan_ma_comm_t *comm,
                            const plan_ma_msg_t *msg);
static void privateHMaxInc(private_t *private, plan_ma_comm_t *comm,
                           const plan_ma_msg_t *msg);
static void privateHMax(private_t *private, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg);
static void privateSupp(private_t *private, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg);
static void privateGoalZone(private_t *private, plan_ma_comm_t *comm,
                            const plan_ma_msg_t *msg);
static void privateFindCut(private_t *private, plan_ma_comm_t *comm,
                            const plan_ma_msg_t *msg);
static void privateCut(private_t *private, plan_ma_comm_t *comm,
                       const plan_ma_msg_t *msg);


/** Forward declaration */
typedef struct _plan_heur_ma_lm_cut_t plan_heur_ma_lm_cut_t;
/** Method for procedure corresponding to the current state */
typedef int (*state_step_fn)(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg);

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

#ifdef DEBUG
static void debugRelax(const plan_heur_relax_t *relax, int agent_id,
                       const plan_op_id_tr_t *op_id_tr, const char *name);
static void debugCut(const cut_t *cut, const plan_heur_relax_t *relax,
                     int agent_id, const plan_op_id_tr_t *op_id_tr,
                     const char *name);
static void assertMsgType(const plan_heur_ma_lm_cut_t *heur,
                          const plan_ma_msg_t *msg, int subtype,
                          int check_agent_id);
#else /* DEBUG */
# define assertMsgType(heur, msg, subtype, check_agent_id)
#endif /* DEBUG */

static void heurDel(plan_heur_t *_heur);
static int heurMA(plan_heur_t *heur, plan_ma_comm_t *comm,
                  const plan_state_t *state, plan_heur_res_t *res);
static int heurMAUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg, plan_heur_res_t *res);
static void heurMARequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                          const plan_ma_msg_t *msg);
/** Allocates enough private_t structures to cover also the specified
 *  agent_id */
static void privateAlloc(plan_heur_ma_lm_cut_t *heur, int agent_id);
/** Returns next agent ID that is marked as changed or -1 if none of agents
 *  is marked as changed. */
static int nextAgentId(const plan_heur_ma_lm_cut_t *heur);
/** Returns number of remote agents stored as owners in operators */
static int agentSize(const plan_op_t *op, int op_size);
/** Returns true if any operator has a conditional effect */
static int checkConditionalEffects(const plan_op_t *op, int op_size);
static void setAllAgentsAndOpsAsChanged(plan_heur_ma_lm_cut_t *heur);
/** Set .agent_changed[] to 0 and set cur_agent_id to next agent after the
 *  main one. */
static void zeroizeAgentChanged(plan_heur_ma_lm_cut_t *heur);
/** Fills msg[] with messages of specified subtype for each agent except
 *  the main one. */
static void createMsgsForAgents(plan_heur_ma_lm_cut_t *heur, int subtype,
                                plan_ma_msg_t **msg);
/** Sends messages to their corresponding receivers. If only_agent_changed
 *  is set to 1, only those messages for which .agent_changed[] is true are
 *  sent. All messages are also deleted. */
static int sendMsgs(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                    plan_ma_msg_t **msg, int only_agent_changed);
/** Returns true if we reached to dead-end */
static int checkDeadEnd(const plan_heur_relax_t *relax);
/** Returns true if computation of h^lm-cut is finished */
static int checkFinish(const plan_heur_relax_t *relax);
static void updateHMaxByCut(plan_heur_relax_t *relax, cut_t *cut,
                            int *updated_ops);

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
static int stepCutUpdate(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                         const plan_ma_msg_t *msg);
static int stepHMax(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                    const plan_ma_msg_t *msg);
static int stepHMaxUpdate(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                          const plan_ma_msg_t *msg);

/** Updates op[] array with values from of operators from message if case
 *  the value is higher than the one in op[] array.
 *  If op_all is non-NULL values of all operators from message are written
 *  to this array.
 *  Arguments updated_ops and updated_ops_size are output values where are
 *  stored IDs of operators that were written to op[]. */
static void hmaxUpdateOpValueFromMsg(const plan_ma_msg_t *msg,
                                     const plan_op_id_tr_t *op_id_tr,
                                     plan_heur_relax_op_t *op,
                                     plan_heur_relax_op_t *op_all,
                                     int *updated_ops, int *updated_ops_size);
/** Runs full h^max heuristic. */
static void hmaxFull(plan_heur_relax_t *relax, const plan_state_t *state);
/** Reset values of operators without supporter facts to the values that
 *  correspond to their preconditions (thus are not changed via values from
 *  other agents. Operators that are not in cut and was changed are added
 *  to the updated_ops array. */
static void hmaxResetNonSupportedOps(plan_heur_relax_t *relax,
                                     const cut_t *cut,
                                     int *updated_ops,
                                     int *updated_ops_size);



plan_heur_t *planHeurMALMCutNew(const plan_problem_t *prob)
{
    plan_heur_ma_lm_cut_t *heur;
    int i;

    if (checkConditionalEffects(prob->proj_op, prob->proj_op_size)){
        fprintf(stderr, "Error: ma-lm-cut heuristic cannot be run on"
                        " operators with conditional effects.\n");
        return NULL;
    }

    heur = BOR_ALLOC(plan_heur_ma_lm_cut_t);
    _planHeurInit(&heur->heur, heurDel, NULL, NULL);
    _planHeurMAInit(&heur->heur, heurMA, NULL, heurMAUpdate, heurMARequest);
    planHeurRelaxInit(&heur->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      prob->var, prob->var_size, prob->goal,
                      prob->proj_op, prob->proj_op_size, 0);
    planOpIdTrInit(&heur->op_id_tr, prob->proj_op, prob->proj_op_size);

    heur->state = STATE_INIT;
    heur->state_step = BOR_CALLOC_ARR(state_step_fn, STATE_SIZE);
    heur->state_step[STATE_INIT] = stepInit;
    heur->state_step[STATE_GOAL_ZONE] = stepGoalZone;
    heur->state_step[STATE_GOAL_ZONE_UPDATE] = stepGoalZoneUpdate;
    heur->state_step[STATE_FIND_CUT] = stepFindCut;
    heur->state_step[STATE_FIND_CUT_UPDATE] = stepFindCutUpdate;
    heur->state_step[STATE_CUT] = stepCut;
    heur->state_step[STATE_CUT_UPDATE] = stepCutUpdate;
    heur->state_step[STATE_HMAX] = stepHMax;
    heur->state_step[STATE_HMAX_UPDATE] = stepHMaxUpdate;

    heur->agent_size = agentSize(prob->proj_op, prob->proj_op_size);
    heur->agent_changed = BOR_CALLOC_ARR(int, heur->agent_size);

    heur->op_size = prob->proj_op_size;
    heur->op = BOR_CALLOC_ARR(ma_lm_cut_op_t, heur->op_size);
    for (i = 0; i < prob->proj_op_size; ++i)
        heur->op[i].owner = prob->proj_op[i].owner;

    heur->opcmp = BOR_ALLOC_ARR(plan_heur_relax_op_t,
                                heur->relax.cref.op_size);

    heur->init_state = planStateNew(prob->state_pool->num_vars);
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

static void heurMARequest(plan_heur_t *_heur, plan_ma_comm_t *comm,
                          const plan_ma_msg_t *msg)
{
    plan_heur_ma_lm_cut_t *heur = HEUR(_heur);
    private_t *private;
    int type, subtype;

    // Allocate private structure if needed and choose private_t
    // corresponding to the agent from which we received the message.
    privateAlloc(heur, planMAMsgAgent(msg));
    private = heur->private + planMAMsgAgent(msg);

    type = planMAMsgType(msg);
    subtype = planMAMsgSubType(msg);

    if (type == PLAN_MA_MSG_HEUR){
        if (subtype == PLAN_MA_MSG_HEUR_LM_CUT_INIT_REQUEST){
            privateInitStep(private, comm, msg);
            return;

        }else if (subtype == PLAN_MA_MSG_HEUR_LM_CUT_HMAX_INC_REQUEST){
            privateHMaxInc(private, comm, msg);
            return;

        }else if (subtype == PLAN_MA_MSG_HEUR_LM_CUT_HMAX_REQUEST){
            privateHMax(private, comm, msg);
            return;

        }else if(subtype == PLAN_MA_MSG_HEUR_LM_CUT_SUPP_REQUEST){
            privateSupp(private, comm, msg);
            return;

        }else if(subtype == PLAN_MA_MSG_HEUR_LM_CUT_GOAL_ZONE_REQUEST){
            privateGoalZone(private, comm, msg);
            return;

        }else if(subtype == PLAN_MA_MSG_HEUR_LM_CUT_FIND_CUT_REQUEST){
            privateFindCut(private, comm, msg);
            return;

        }else if(subtype == PLAN_MA_MSG_HEUR_LM_CUT_CUT_REQUEST){
            privateCut(private, comm, msg);
            return;
        }
    }

    fprintf(stderr, "Error[%d]: The request isn't for lm-cut heuristic.\n",
            comm->node_id);
}


static int mainLoop(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                    const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    while (heur->state_step[heur->state](heur, comm, msg) == 0
            && heur->state != STATE_FINISH);

    if (heur->state == STATE_FINISH){
        res->heur = heur->heur_value;
        return 0;
    }

    return -1;
}


/**** Private ****/
static void privateInit(private_t *private,
                        plan_ma_state_t *ma_state,
                        const plan_problem_t *prob)
{
    plan_op_t *op;
    int op_size;
    int i;

    private->ma_state = ma_state;
    planProblemCreatePrivateProjOps(prob->op, prob->op_size,
                                    prob->var, prob->var_size,
                                    &op, &op_size);
    planHeurRelaxInit(&private->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      prob->var, prob->var_size, prob->goal, op, op_size, 0);

    // Get IDs of all public operators
    planArrIntInit(&private->public_op, 2);
    for (i = 0; i < op_size; ++i){
        if (!op[i].is_private){
            planArrIntAdd(&private->public_op, i);
        }
    }

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
    planArrIntFree(&private->public_op);
    if (private->updated_ops)
        BOR_FREE(private->updated_ops);
    if (private->opcmp)
        BOR_FREE(private->opcmp);
    cutFree(&private->cut);
}
/**** Private END ****/



/**** INIT STEP ****/
/**
 * In the init step:
 * 1. Main agent sends init request to all other agents,
 * 2. Main agent computes full h^max heuristic on projected operators and
 *    sends HMax request to first remote agent.
 * 3. Remote agent receives init request and computes full h^max heuristic
 *    on private projections of operators.
 */

static void sendInitRequest(plan_heur_ma_lm_cut_t *heur,
                            plan_ma_comm_t *comm, int agent_id)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_LM_CUT_INIT_REQUEST,
                       planMACommId(comm));
    planMAStateSetMAMsg2(heur->heur.ma_state, heur->init_state, msg);
    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static void sendHMaxRequest(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                            int agent_id)
{
    plan_ma_msg_t *msg;
    plan_ma_msg_op_t *add_op;
    int i, op_id, value;
    ma_lm_cut_op_t op;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_LM_CUT_HMAX_REQUEST,
                       planMACommId(comm));

    // Add ID and value of all operators there were changed to the message.
    for (i = 0; i < heur->op_size; ++i){
        op = heur->op[i];
        if (op.owner == agent_id && op.changed){
            value = heur->relax.op[i].value;
            op_id = planOpIdTrGlob(&heur->op_id_tr, i);
            add_op = planMAMsgAddOp(msg);
            planMAMsgOpSetOpId(add_op, op_id);
            planMAMsgOpSetValue(add_op, value);

            heur->op[i].changed = 0;
        }
    }
    heur->agent_changed[agent_id] = 0;

    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static int stepInit(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                    const plan_ma_msg_t *msg)
{
    int i;

    // Every agent must be requested at least once, so make sure of that.
    setAllAgentsAndOpsAsChanged(heur);

    // Send init requests
    for (i = 0; i < heur->agent_size; ++i){
        if (i != heur->agent_id)
            sendInitRequest(heur, comm, i);
    }

    // h^max heuristics for all facts and operators reachable from the
    // state
    hmaxFull(&heur->relax, heur->init_state);

    // Send request to next agent
    sendHMaxRequest(heur, comm, heur->cur_agent_id);

    heur->state = STATE_HMAX_UPDATE;
    return -1;
}

static void privateInitStep(private_t *private, plan_ma_comm_t *comm,
                            const plan_ma_msg_t *msg)
{
    PLAN_STATE_STACK(state, private->relax.cref.fact_id.var_size);

    // Early exit if we have no private operators
    if (private->relax.cref.op_size == 0)
        return;

    // Run full relaxation heuristic
    planMAStateGetFromMAMsg(private->ma_state, msg, &state);
    hmaxFull(&private->relax, &state);
}
/**** INIT STEP END ****/


/**** HMAX UPDATE STEP ****/
/**
 * In this step the h^max heuristic is computed on cluster of agents.
 * When the computation is finished and all agents have same h^max values
 * for all operators and facts, supporters of operators are chosen so that
 * for each operator only one agent knows its supporter.
 */

static void sendSuppRequest(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg[heur->agent_size];
    int agent_id, i, op_id;

    // Create one message per remote agent
    createMsgsForAgents(heur, PLAN_MA_MSG_HEUR_LM_CUT_SUPP_REQUEST, msg);

    // Fill message for each agent with the agent's operators that have
    // supporter facts know to this (main) agent.
    for (i = 0; i < heur->op_size; ++i){
        agent_id = heur->op[i].owner;
        if (agent_id != heur->agent_id && heur->relax.op[i].supp >= 0){
            op_id = planOpIdTrGlob(&heur->op_id_tr, i);
            if (op_id >= 0)
                planMAMsgOpSetOpId(planMAMsgAddOp(msg[agent_id]), op_id);
        }
    }

    // Send message to all remote agents
    sendMsgs(heur, comm, msg, 0);
}

static void updateHMax(plan_heur_ma_lm_cut_t *heur,
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

static int stepHMaxUpdate(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                          const plan_ma_msg_t *msg)
{
    int updated_ops_size;
    assertMsgType(heur, msg, PLAN_MA_MSG_HEUR_LM_CUT_HMAX_RESPONSE, 1);

    // Update operators' values from message
    hmaxUpdateOpValueFromMsg(msg, &heur->op_id_tr, heur->relax.op, NULL,
                             heur->updated_ops, &updated_ops_size);

    // Update h^max values of facts and operators considering changes in
    // the operators
    if (updated_ops_size > 0){
        updateHMax(heur, heur->updated_ops, updated_ops_size);
    }

    heur->cur_agent_id = nextAgentId(heur);
    if (heur->cur_agent_id >= 0){
        // If there are more agents that have changed operators send the
        // request and wait for response.
        sendHMaxRequest(heur, comm, heur->cur_agent_id);
        return -1;

    }else{
        // Detect dead-end or if computation is finished
        if (checkDeadEnd(&heur->relax)){
            heur->heur_value = PLAN_HEUR_DEAD_END;
            heur->state = STATE_FINISH;
            return 0;

        }else if (checkFinish(&heur->relax)){
            heur->state = STATE_FINISH;
            return 0;
        }

        sendSuppRequest(heur, comm);
        heur->state = STATE_GOAL_ZONE;
        return 0;
    }

    return -1;
}

static void sendHMaxEmptyResponse(plan_ma_comm_t *comm,
                                  const plan_ma_msg_t *req)
{
    plan_ma_msg_t *msg;
    int target_agent;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_LM_CUT_HMAX_RESPONSE,
                       planMACommId(comm));
    target_agent = planMAMsgAgent(req);
    planMACommSendToNode(comm, target_agent, msg);
    planMAMsgDel(msg);
}

static void sendHMaxResponse(private_t *private, plan_ma_comm_t *comm,
                             int agent_id)
{
    int op_id;
    plan_cost_t value;
    plan_ma_msg_t *msg;
    plan_ma_msg_op_t *add_op;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_LM_CUT_HMAX_RESPONSE,
                       planMACommId(comm));

    // Check all public operators and send all that were changed
    PLAN_ARR_INT_FOR_EACH(&private->public_op, op_id){
        if (private->opcmp[op_id].value == private->relax.op[op_id].value)
            continue;
        if (private->relax.op[op_id].unsat > 0)
            continue;

        value = private->relax.op[op_id].value;
        op_id = planOpIdTrGlob(&private->op_id_tr, op_id);
        add_op = planMAMsgAddOp(msg);
        planMAMsgOpSetOpId(add_op, op_id);
        planMAMsgOpSetValue(add_op, value);
    }
    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static void privateHMax(private_t *private, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
    int updated_ops_size;

    // Early exit if we have no private operators
    if (private->relax.cref.op_size == 0){
        sendHMaxEmptyResponse(comm, msg);
        return;
    }

    // Copy current state of operators to compare array.
    // This has to be done before relax->op values are changed according to
    // the received message because we need to compare on the state of
    // operators that is assumed by the initiator agent.
    memcpy(private->opcmp, private->relax.op,
           sizeof(plan_heur_relax_op_t) * private->relax.cref.op_size);

    // Update operators' values from received message
    hmaxUpdateOpValueFromMsg(msg, &private->op_id_tr,
                             private->relax.op, private->opcmp,
                             private->updated_ops, &updated_ops_size);

    // Update .relax structure if something was changed
    if (updated_ops_size > 0){
        planHeurRelaxUpdateMaxFull(&private->relax, private->updated_ops,
                                   updated_ops_size);
    }

    // Send response to the main agent
    sendHMaxResponse(private, comm, planMAMsgAgent(msg));
}

static void privateSupp(private_t *private, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
    int i, size, op_id;

    // Mark all received operators as without supporter fact because the
    // supporter fact is hold by the main agent.
    size = planMAMsgOpSize(msg);
    for (i = 0; i < size; ++i){
        op_id = planMAMsgOpOpId(planMAMsgOp(msg, i));
        op_id = planOpIdTrLoc(&private->op_id_tr, op_id);
        if (op_id >= 0)
            private->relax.op[op_id].supp = -1;
    }

    // Initialize cut_t structure it will be needed in next phases
    cutInitCycle(&private->cut, &private->relax);
}
/**** HMAX UPDATE STEP END ****/


/**** HMAX INC ****/
/**
 * In this step, cut is applied to the current state of relaxation
 * heuristic, the heuristic is partially repaired and computation of h^max
 * starts on whole agent cluster.
 */

static void sendHMaxIncRequest(plan_heur_ma_lm_cut_t *heur,
                               plan_ma_comm_t *comm, int agent_id)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_LM_CUT_HMAX_INC_REQUEST,
                       planMACommId(comm));
    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static int stepHMax(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                    const plan_ma_msg_t *msg)
{
    int i;

    // Update h^max values according to the cut
    updateHMaxByCut(&heur->relax, &heur->cut, heur->updated_ops);

    // Send request to each agent so they update their state of relaxed
    // problem according to the current cut.
    for (i = 0; i < heur->agent_size; ++i){
        if (i == heur->agent_id)
            continue;
        sendHMaxIncRequest(heur, comm, i);
    }

    // Every agent must be requested at least once, so make sure of that.
    setAllAgentsAndOpsAsChanged(heur);

    // Send request to next agent
    sendHMaxRequest(heur, comm, heur->cur_agent_id);

    heur->state = STATE_HMAX_UPDATE;
    return -1;
}

static void privateHMaxInc(private_t *private, plan_ma_comm_t *comm,
                           const plan_ma_msg_t *msg)
{
    updateHMaxByCut(&private->relax, &private->cut, private->updated_ops);
}
/**** HMAX INC END ****/


/**** GOAL ZONE ****/
/**
 * In this step, the operators and facts connected with a goal by zero-cost
 * operators are marked (thus they belong to goal-zone). This is done by
 * distributed graph search.
 */

static int sendGoalZoneRequests(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg[heur->agent_size];
    int i, op_id, agent_id;

    // Create msg for each agent
    createMsgsForAgents(heur, PLAN_MA_MSG_HEUR_LM_CUT_GOAL_ZONE_REQUEST, msg);

    // Add to message all operators from goal zone
    zeroizeAgentChanged(heur);
    for (i = 0; i < heur->op_size; ++i){
        if (heur->cut.op[i].state == CUT_GOAL_ZONE){
            // Mark operator as done to prevent repeated sending
            heur->cut.op[i].state = CUT_OP_DONE;

            // Skip operators this agent is owner of
            agent_id = heur->op[i].owner;
            if (agent_id == heur->agent_id)
                continue;

            op_id = planOpIdTrGlob(&heur->op_id_tr, i);
            planMAMsgOpSetOpId(planMAMsgAddOp(msg[agent_id]), op_id);
            heur->agent_changed[agent_id] = 1;
        }
    }

    // Send messages and return how many were sent
    return sendMsgs(heur, comm, msg, 1);
}

static int stepGoalZone(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
    // Initialize cut structure because we are searching for cut in a new
    // justification graph.
    cutInitCycle(&heur->cut, &heur->relax);

    // Mark operators and facts in goal-zone
    cutMarkGoalZone(&heur->cut, &heur->relax, heur->relax.cref.goal_id);

    // Send requests to other agents if needed, if not proceed directly to
    // find-cut step
    if (sendGoalZoneRequests(heur, comm) == 0){
        heur->state = STATE_FIND_CUT;
        return 0;
    }else{
        heur->state = STATE_GOAL_ZONE_UPDATE;
        return -1;
    }
}

static int stepGoalZoneUpdate(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                              const plan_ma_msg_t *msg)
{
    int i, size, op_id;
    assertMsgType(heur, msg, PLAN_MA_MSG_HEUR_LM_CUT_GOAL_ZONE_RESPONSE, 0);

    // Explore goal-zone from all received operators
    size = planMAMsgOpSize(msg);
    for (i = 0; i < size; ++i){
        op_id = planMAMsgOpOpId(planMAMsgOp(msg, i));
        op_id = planOpIdTrLoc(&heur->op_id_tr, op_id);
        if (op_id < 0)
            continue;
        cutMarkGoalZoneOp(&heur->cut, &heur->relax, op_id);
    }

    // Check if we are waiting for more responses. If not, send another
    // bulk of requests if needed or proceed to find-cut step.
    heur->agent_changed[planMAMsgAgent(msg)] = 0;
    if (nextAgentId(heur) < 0){
        if (sendGoalZoneRequests(heur, comm) == 0){
            heur->state = STATE_FIND_CUT;
            return 0;
        }
    }

    heur->state = STATE_GOAL_ZONE_UPDATE;
    return -1;
}

static void sendGoalZoneResponse(private_t *private, plan_ma_comm_t *comm,
                                 int agent_id)
{
    plan_ma_msg_t *msg;
    int op_id;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_LM_CUT_GOAL_ZONE_RESPONSE,
                       planMACommId(comm));

    // Add operators that in goal-zone
    PLAN_ARR_INT_FOR_EACH(&private->public_op, op_id){
        if (private->cut.op[op_id].state == CUT_GOAL_ZONE){
            private->cut.op[op_id].state = CUT_OP_DONE;
            op_id = private->relax.cref.op_id[op_id];
            if (op_id < 0)
                continue;
            op_id = planOpIdTrGlob(&private->op_id_tr, op_id);
            planMAMsgOpSetOpId(planMAMsgAddOp(msg), op_id);
        }
    }

    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static void privateGoalZone(private_t *private, plan_ma_comm_t *comm,
                            const plan_ma_msg_t *msg)
{
    int i, size, op_id;

    // Proceed with goal-zone exploration from the received operators
    size = planMAMsgOpSize(msg);
    for (i = 0; i < size; ++i){
        op_id = planMAMsgOpOpId(planMAMsgOp(msg, i));
        op_id = planOpIdTrLoc(&private->op_id_tr, op_id);
        if (op_id < 0)
            continue;

        cutMarkGoalZoneOp(&private->cut, &private->relax, op_id);
    }

    sendGoalZoneResponse(private, comm, planMAMsgAgent(msg));
}
/**** GOAL ZONE END ****/


/**** FIND CUT ****/
/**
 * In this step, justification graph is explored from the initial state
 * until it reaches the goal-zone. Operators that end in goal-zone fact are
 * marked as they are in cut.
 */

static int sendFindCutRequests(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                               int add_state)
{
    plan_ma_msg_t *msg[heur->agent_size];
    int i, op_id, agent_id;

    // Create one messge per agent
    createMsgsForAgents(heur, PLAN_MA_MSG_HEUR_LM_CUT_FIND_CUT_REQUEST, msg);

    // Set state to each message and update agent_change[] flag
    zeroizeAgentChanged(heur);
    for (agent_id = 0; agent_id < heur->agent_size; ++agent_id){
        if (agent_id == heur->agent_id)
            continue;

        if (add_state){
            planMAStateSetMAMsg2(heur->heur.ma_state, heur->init_state,
                                 msg[agent_id]);
            heur->agent_changed[agent_id] = 1;
        }
    }

    // Add operators from start-zone to a corresponding message according
    // to an owner of the opetator.
    for (i = 0; i < heur->op_size; ++i){
        if (heur->cut.op[i].state == CUT_START_ZONE){
            heur->cut.op[i].state = CUT_OP_DONE;

            agent_id = heur->op[i].owner;
            if (agent_id == heur->agent_id)
                continue;

            op_id = planOpIdTrGlob(&heur->op_id_tr, i);
            planMAMsgOpSetOpId(planMAMsgAddOp(msg[agent_id]), op_id);
            heur->agent_changed[agent_id] = 1;
        }
    }

    // Send messages to agents
    return sendMsgs(heur, comm, msg, 1);
}

static int stepFindCut(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                       const plan_ma_msg_t *msg)
{
    int i, fact_id;

    // Find cut from all facts in initial state and all fake preconditions
    for (i = 0; i < heur->init_state->size; ++i){
        fact_id = planFactIdVar(&heur->relax.cref.fact_id, i,
                                planStateGet(heur->init_state, i));
        if (fact_id >= 0)
            cutFind(&heur->cut, &heur->relax, fact_id, 1);
    }
    for (i = 0; i < heur->relax.cref.fake_pre_size; ++i){
        cutFind(&heur->cut, &heur->relax,
                heur->relax.cref.fake_pre[i].fact_id, 1);
    }

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
    assertMsgType(heur, msg, PLAN_MA_MSG_HEUR_LM_CUT_FIND_CUT_RESPONSE, 0);

    // Update minimal cost of cut from message
    heur->cut.min_cut = BOR_MIN(heur->cut.min_cut, planMAMsgMinCutCost(msg));

    // Proceed in exploring justification graph from the received operators
    size = planMAMsgOpSize(msg);
    for (i = 0; i < size; ++i){
        op_id = planMAMsgOpOpId(planMAMsgOp(msg, i));
        op_id = planOpIdTrLoc(&heur->op_id_tr, op_id);
        if (op_id < 0)
            continue;

        cutFindOp(&heur->cut, &heur->relax, op_id);
    }

    // Mark agent as processed
    from_agent = planMAMsgAgent(msg);
    heur->agent_changed[from_agent] = 0;

    // If messages from all agents were processed, try to send another
    // bulk of messages if anything changed. If nothing changed,
    // proceed to next phase.
    if (nextAgentId(heur) < 0){
        if (sendFindCutRequests(heur, comm, 0) == 0){
            heur->state = STATE_CUT;
            return 0;
        }
    }

    return -1;
}

static void sendFindCutResponse(private_t *private, plan_ma_comm_t *comm,
                                int agent_id)
{
    plan_ma_msg_t *msg;
    int op_id;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_LM_CUT_FIND_CUT_RESPONSE,
                       planMACommId(comm));

    // Add operators that were discovered in start-zone
    PLAN_ARR_INT_FOR_EACH(&private->public_op, op_id){
        if (private->cut.op[op_id].state == CUT_START_ZONE){
            private->cut.op[op_id].state = CUT_OP_DONE;
            op_id = private->relax.cref.op_id[op_id];
            if (op_id < 0)
                continue;
            op_id = planOpIdTrGlob(&private->op_id_tr, op_id);
            planMAMsgOpSetOpId(planMAMsgAddOp(msg), op_id);
        }
    }

    // Set minimal cost of cut so far
    planMAMsgSetMinCutCost(msg, private->cut.min_cut);

    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static void privateFindCut(private_t *private, plan_ma_comm_t *comm,
                           const plan_ma_msg_t *msg)
{
    int i, size, fact_id, op_id;
    PLAN_STATE_STACK(state, private->relax.cref.fact_id.var_size);

    // If also state was received start exploring start-zone from that state
    // and all fake preconditions.
    if (planMAStateMAMsgIsSet(private->ma_state, msg)){
        planMAStateGetFromMAMsg(private->ma_state, msg, &state);

        for (i = 0; i < private->relax.cref.fact_id.var_size; ++i){
            fact_id = planStateGet(&state, i);
            fact_id = planFactIdVar(&private->relax.cref.fact_id, i, fact_id);
            if (fact_id >= 0)
                cutFind(&private->cut, &private->relax, fact_id, 1);
        }
        for (i = 0; i < private->relax.cref.fake_pre_size; ++i){
            fact_id = private->relax.cref.fake_pre[i].fact_id;
            cutFind(&private->cut, &private->relax, fact_id, 1);
        }
    }

    // Proceed with exploring from the received operators
    size = planMAMsgOpSize(msg);
    for (i = 0; i < size; ++i){
        op_id = planMAMsgOpOpId(planMAMsgOp(msg, i));
        op_id = planOpIdTrLoc(&private->op_id_tr, op_id);
        if (op_id < 0)
            continue;

        cutFindOp(&private->cut, &private->relax, op_id);
    }

    sendFindCutResponse(private, comm, planMAMsgAgent(msg));
}
/**** FIND CUT END ****/


/**** CUT ****/
/**
 * In this step, information about cut is distributed among all agents in
 * cluster.
 */

static void sendCutRequests(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg[heur->agent_size];
    ma_lm_cut_op_t *op;
    int agent_id, i, op_id;

    // Create message for each agent and set minimal cut cost
    createMsgsForAgents(heur, PLAN_MA_MSG_HEUR_LM_CUT_CUT_REQUEST, msg);
    zeroizeAgentChanged(heur);
    for (agent_id = 0; agent_id < heur->agent_size; ++agent_id){
        if (agent_id == heur->agent_id)
            continue;

        planMAMsgSetMinCutCost(msg[agent_id], heur->cut.min_cut);
        heur->agent_changed[agent_id] = 1;
    }

    // Add operators from cut to a corresponding msg
    for (i = 0; i < heur->op_size; ++i){
        op = heur->op + i;
        if (op->owner == heur->agent_id)
            continue;

        if (heur->cut.op[i].in_cut){
            op_id = planOpIdTrGlob(&heur->op_id_tr, i);
            planMAMsgOpSetOpId(planMAMsgAddOp(msg[op->owner]), op_id);
        }
    }

    sendMsgs(heur, comm, msg, 0);
}

static int stepCut(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                   const plan_ma_msg_t *msg)
{
    // Check if have found any cut.
    // Finding no cut means that there is serious error in the
    // implementation, so leave it here.
    if (heur->cut.min_cut == PLAN_COST_MAX){
        heur->heur_value = PLAN_HEUR_DEAD_END;
        heur->state = STATE_FINISH;
        return 0;
    }

    sendCutRequests(heur, comm);
    heur->heur_value += heur->cut.min_cut;
    heur->state = STATE_CUT_UPDATE;
    return -1;
}

static int stepCutUpdate(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                         const plan_ma_msg_t *msg)
{
    int i, size, op_id;
    assertMsgType(heur, msg, PLAN_MA_MSG_HEUR_LM_CUT_CUT_RESPONSE, 0);

    // Add received operators to the cut
    size = planMAMsgOpSize(msg);
    for (i = 0; i < size; ++i){
        op_id = planMAMsgOpOpId(planMAMsgOp(msg, i));
        op_id = planOpIdTrLoc(&heur->op_id_tr, op_id);
        if (op_id < 0)
            continue;

        heur->cut.op[op_id].in_cut = 1;
    }

    // Wait for all other responses and then proceed to hmax-step
    heur->agent_changed[planMAMsgAgent(msg)] = 0;
    if (nextAgentId(heur) < 0){
        heur->state = STATE_HMAX;
        return 0;
    }

    heur->state = STATE_CUT_UPDATE;
    return -1;
}

static void sendCutResponse(private_t *private, plan_ma_comm_t *comm,
                            int agent_id)
{
    plan_ma_msg_t *msg;
    int op_id;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_LM_CUT_CUT_RESPONSE,
                       planMACommId(comm));

    // Add all operators from cut
    PLAN_ARR_INT_FOR_EACH(&private->public_op, op_id){
        if (private->cut.op[op_id].in_cut){
            op_id = planOpIdTrGlob(&private->op_id_tr, op_id);
            planMAMsgOpSetOpId(planMAMsgAddOp(msg), op_id);
        }
    }
    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static void privateCut(private_t *private, plan_ma_comm_t *comm,
                       const plan_ma_msg_t *msg)
{
    int i, size, op_id;

    // Add all received operators to the cut
    size = planMAMsgOpSize(msg);
    for (i = 0; i < size; ++i){
        op_id = planMAMsgOpOpId(planMAMsgOp(msg, i));
        op_id = planOpIdTrLoc(&private->op_id_tr, op_id);
        if (op_id < 0)
            continue;

        private->cut.op[op_id].in_cut = 1;
    }

    // Update cost of the cut
    private->cut.min_cut = planMAMsgMinCutCost(msg);

    sendCutResponse(private, comm, planMAMsgAgent(msg));
}
/**** CUT END ****/



/**** HMAX ****/
static void hmaxUpdateOpValueFromMsg(const plan_ma_msg_t *msg,
                                     const plan_op_id_tr_t *op_id_tr,
                                     plan_heur_relax_op_t *op,
                                     plan_heur_relax_op_t *op_all,
                                     int *updated_ops, int *updated_ops_size)
{
    const plan_ma_msg_op_t *msg_op;
    int i, op_size, op_id;
    plan_cost_t op_value, value;

    *updated_ops_size = 0;

    op_size = planMAMsgOpSize(msg);
    for (i = 0; i < op_size; ++i){
        // Read operator data from message
        msg_op = planMAMsgOp(msg, i);
        op_id = planMAMsgOpOpId(msg_op);
        op_value = planMAMsgOpValue(msg_op);

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

static void hmaxFull(plan_heur_relax_t *relax, const plan_state_t *state)
{
    int i;

    planHeurRelaxFull(relax, state);

    // Modify unreachable operators so it can be propagated to other agents
    for (i = 0; i < relax->cref.op_size; ++i){
        if (relax->op[i].unsat > 0){
            relax->op[i].value = PLAN_COST_UNREACHABLE;
            relax->op[i].cost = 0;
            relax->op[i].supp = -1;
        }
    }
}

static void hmaxResetNonSupportedOps(plan_heur_relax_t *relax,
                                     const cut_t *cut,
                                     int *updated_ops,
                                     int *updated_ops_size)
{
    int op_id, fact_id, supp;
    plan_heur_relax_op_t *op;
    plan_cost_t value;

    for (op_id = 0; op_id < relax->cref.op_size; ++op_id){
        op = relax->op + op_id;
        if (op->supp != -1 || op->unsat > 0)
            continue;

        // Find biggest value and remember that fact as supporter
        value = -1;
        supp = -1;
        PLAN_ARR_INT_FOR_EACH(relax->cref.op_pre + op_id, fact_id){
            if (relax->fact[fact_id].value > value){
                value = relax->fact[fact_id].value;
                supp = fact_id;
            }
        }

        relax->op[op_id].value = value + relax->op[op_id].cost;
        relax->op[op_id].supp = supp;
        if (!cut->op[op_id].in_cut){
            updated_ops[(*updated_ops_size)++] = op_id;
        }
    }
}
/**** HMAX END ****/


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
    int fact_id, op_id;
    plan_heur_relax_op_t *op;

    borLifoInit(&lifo, sizeof(int));
    cut->fact[goal_id].state = CUT_GOAL_ZONE;
    borLifoPush(&lifo, &goal_id);
    while (!borLifoEmpty(&lifo)){
        fact_id = *(int *)borLifoBack(&lifo);
        borLifoPop(&lifo);

        PLAN_ARR_INT_FOR_EACH(relax->cref.fact_eff + fact_id, op_id){
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

static void cutMarkGoalZoneOp(cut_t *cut, const plan_heur_relax_t *relax,
                              int op_id)
{
    if (cut->op[op_id].state != 0)
        return;

    cut->op[op_id].state = CUT_OP_DONE;
    if (relax->op[op_id].supp >= 0)
        cutMarkGoalZone(cut, relax, relax->op[op_id].supp);
}

static void cutFindAddEff(cut_t *cut, const plan_heur_relax_t *relax,
                          int op_id, bor_lifo_t *lifo)
{
    int fact_id;

    PLAN_ARR_INT_FOR_EACH(relax->cref.op_eff + op_id, fact_id){
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
    int fact_id, op_id;
    plan_heur_relax_op_t *op;

    while (!borLifoEmpty(lifo)){
        fact_id = *(int *)borLifoBack(lifo);
        borLifoPop(lifo);

        PLAN_ARR_INT_FOR_EACH(relax->cref.fact_pre + fact_id, op_id){
            op = relax->op + op_id;
            if (cut->op[op_id].state == 0 && op->supp == fact_id){
                cut->op[op_id].state = CUT_START_ZONE;
                cutFindAddEff(cut, relax, op_id, lifo);
            }
        }
    }
}

static void cutFind(cut_t *cut, const plan_heur_relax_t *relax,
                    int start_fact, int is_init)
{
    bor_lifo_t lifo;

    if (!is_init && cut->fact[start_fact].state != 0)
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

static void cutApply(cut_t *cut, plan_heur_relax_t *relax,
                     int *updated_ops, int *updated_ops_size)
{
    int i;

    *updated_ops_size = 0;
    for (i = 0; i < relax->cref.op_size; ++i){
        if (cut->op[i].in_cut){
            relax->op[i].cost -= cut->min_cut;
            relax->op[i].value -= cut->min_cut;
            updated_ops[(*updated_ops_size)++] = i;
        }
    }
}
/*** CUT END ***/


/**** COMMON ****/
static void privateAlloc(plan_heur_ma_lm_cut_t *heur, int agent_id)
{
    int i;

    if (agent_id < heur->private_size)
        return;

    heur->private = BOR_REALLOC_ARR(heur->private, private_t, agent_id + 1);
    for (i = heur->private_size; i < agent_id + 1; ++i)
        privateInit(heur->private + i, heur->heur.ma_state, heur->prob);
    heur->private_size = agent_id + 1;
}

static int nextAgentId(const plan_heur_ma_lm_cut_t *heur)
{
    int i, from, to, id;

    from = 0;
    if (heur->cur_agent_id >= 0)
        from = heur->cur_agent_id + 1;
    to = from + heur->agent_size;

    for (i = from; i < to; ++i){
        id = i % heur->agent_size;
        if (heur->agent_changed[id])
            return id;
    }
    return -1;
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

static int checkConditionalEffects(const plan_op_t *op, int op_size)
{
    int i;

    for (i = 0; i < op_size; ++i){
        if (op[i].cond_eff_size > 0)
            return 1;
    }
    return 0;
}

static void setAllAgentsAndOpsAsChanged(plan_heur_ma_lm_cut_t *heur)
{
    int i;

    // Set all agents as changed and choose next agent
    for (i = 0; i < heur->agent_size; ++i){
        if (i == heur->agent_id){
            heur->agent_changed[i] = 0;
        }else{
            heur->agent_changed[i] = 1;
        }
    }
    heur->cur_agent_id = (heur->agent_id + 1) % heur->agent_size;

    // Set all operators as changed
    for (i = 0; i < heur->op_size; ++i)
        heur->op[i].changed = 1;
}

static void zeroizeAgentChanged(plan_heur_ma_lm_cut_t *heur)
{
    bzero(heur->agent_changed, sizeof(int) * heur->agent_size);
    heur->cur_agent_id = (heur->agent_id + 1) % heur->agent_size;
}

static void createMsgsForAgents(plan_heur_ma_lm_cut_t *heur, int subtype,
                                plan_ma_msg_t **msg)
{
    int i;

    for (i = 0; i < heur->agent_size; ++i){
        if (i == heur->agent_id){
            msg[i] = NULL;
        }else{
            msg[i] = planMAMsgNew(PLAN_MA_MSG_HEUR, subtype, heur->agent_id);
        }
    }
}

static int sendMsgs(plan_heur_ma_lm_cut_t *heur, plan_ma_comm_t *comm,
                    plan_ma_msg_t **msg, int only_agent_changed)
{
    int i, sent = 0;

    for (i = 0; i < heur->agent_size; ++i){
        if (msg[i]){
            if (!only_agent_changed || heur->agent_changed[i]){
                planMACommSendToNode(comm, i, msg[i]);
                ++sent;
            }
            planMAMsgDel(msg[i]);
        }
    }

    return sent;
}

static int checkDeadEnd(const plan_heur_relax_t *relax)
{
    int goal_id = relax->cref.goal_id;
    plan_heur_relax_fact_t *fact = relax->fact + goal_id;

    if (fact->value == PLAN_HEUR_DEAD_END
            || fact->value >= PLAN_COST_UNREACHABLE)
        return 1;
    return 0;
}

static int checkFinish(const plan_heur_relax_t *relax)
{
    int goal_id = relax->cref.goal_id;
    plan_heur_relax_fact_t *fact = relax->fact + goal_id;
    return fact->value == 0;
}

static void updateHMaxByCut(plan_heur_relax_t *relax, cut_t *cut,
                            int *updated_ops)
{
    int updated_ops_size = 0;

    // Apply cut to current state of relaxed problem
    cutApply(cut, relax, updated_ops, &updated_ops_size);
    // Update operators for which we don't know supporters
    hmaxResetNonSupportedOps(relax, cut, updated_ops, &updated_ops_size);
    // Update h^max according to changes
    planHeurRelaxIncMaxFull(relax, updated_ops, updated_ops_size);
}
/**** COMMON END ****/


/*** DEBUG ***/
#ifdef DEBUG
static void debugRelax(const plan_heur_relax_t *relax, int agent_id,
                       const plan_op_id_tr_t *op_id_tr, const char *name)
{
    int i, j, glob_id;

    fprintf(stderr, "[%d] %s\n", agent_id, name);
    for (i = 0; i < relax->cref.op_size; ++i){
        glob_id = -1;
        if (relax->cref.op_id[i] >= 0)
            glob_id = planOpIdTrGlob(op_id_tr, relax->cref.op_id[i]);

        fprintf(stderr, "[%d] Op[%d] value: %d, cost: %d, supp: %d, unsat: %d",
                agent_id, glob_id,
                relax->op[i].value,
                relax->op[i].cost,
                relax->op[i].supp,
                relax->op[i].unsat);

        fprintf(stderr, " Pre[");
        for (j = 0; j < relax->cref.op_pre[i].size; ++j){
            fprintf(stderr, " %d:%d",
                    relax->cref.op_pre[i].fact[j],
                    relax->fact[relax->cref.op_pre[i].fact[j]].value);
        }
        fprintf(stderr, "]");

        fprintf(stderr, " Eff[");
        for (j = 0; j < relax->cref.op_eff[i].size; ++j){
            fprintf(stderr, " %d:%d",
                    relax->cref.op_eff[i].fact[j],
                    relax->fact[relax->cref.op_eff[i].fact[j]].value);
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

static void assertMsgType(const plan_heur_ma_lm_cut_t *heur,
                          const plan_ma_msg_t *msg, int subtype,
                          int check_agent_id)
{
    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || planMAMsgSubType(msg) != subtype){
        fprintf(stderr, "Error[%d]: Received unexpected message (%x"
                        " instead of %x).\n",
                        heur->agent_id, planMAMsgSubType(msg), subtype);
        exit(-1);
    }

    if (check_agent_id && planMAMsgAgent(msg) != heur->cur_agent_id){
        fprintf(stderr, "Error[%d]: Heur response from %d is expected, instead"
                        " response from %d is received.\n",
                        heur->agent_id, heur->cur_agent_id,
                        planMAMsgAgent(msg));
        exit(-1);
    }
}
#endif /* DEBUG */
/*** DEBUG END ***/
