/*** * maplan * -------
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

#include "plan/ma_search.h"
#include "plan/ma_state.h"

#include "ma_snapshot.h"

/** Timeout before dead-end verification is started */
#define DEAD_END_BLOCK_TIME 1000 // 1 second

struct _term_t {
    int is_initiator;
    int initiator_id;

    int final_target;
    int final_counter;

    int final_ack_target;
    int final_ack_counter;
    int is_first;
    int is_last;
};
typedef struct _term_t term_t;

/** Main mutli-agent search structure. */
struct _plan_ma_search_t {
    plan_search_t *search;
    plan_ma_comm_t *comm; /*!< Communication channel between agents */
    plan_ma_state_t *ma_state;
    int solution_verify;

    int pub_state_reg;
    plan_ma_snapshot_reg_t snapshot;

    plan_path_t path;
    int res; /*!< Result of search */
    plan_state_id_t goal;
    plan_cost_t goal_cost;
    int blocked;
    int terminate;
    term_t term;

    plan_heur_t *heur;
};

/** Reference data for the received public states */
struct _pub_state_data_t {
    int agent_id;             /*!< ID of the source agent */
    plan_state_id_t state_id; /*!< ID of the state in remote agent's state
                                   pool. This is used for back-tracking. */
};
typedef struct _pub_state_data_t pub_state_data_t;

static int searchPostStep(plan_search_t *search, int res, void *ud);
static void searchExpandedNode(plan_search_t *search,
                               plan_state_space_node_t *node, void *ud);
static void searchReachedGoal(plan_search_t *search,
                              plan_state_space_node_t *node, void *ud);
static void searchMAHeur(plan_search_t *search, plan_heur_t *heur,
                         plan_state_id_t state_id, plan_heur_res_t *res,
                         void *userdata);
static void processMsg(plan_ma_search_t *ma, plan_ma_msg_t *msg);
static void publicStateSend(plan_ma_search_t *ma,
                            plan_state_space_node_t *node);
static void publicStateRecv(plan_ma_search_t *ma,
                            plan_ma_msg_t *msg);

/** Starts termination schema */
static void terminate(plan_ma_search_t *ma);
/** Process TERMINATE message */
static int terminateMsg(plan_ma_search_t *ma_search,
                        plan_ma_msg_t *msg);

/** Starts trace-path process */
static void tracePath(plan_ma_search_t *ma, plan_state_id_t goal_state);
/** Initialize path tracing for the specified goal state.
 *  Return 0 if the path was fully recovered and thus stored in path
 *  argument. 1 is returned in case the communication with other agents was
 *  initialized. -1 is returned on error. */
static int tracePathInit(const plan_search_t *search,
                         plan_state_id_t goal_state,
                         int state_pool_pub_reg,
                         plan_ma_comm_t *comm,
                         plan_path_t *path);
/** Process incoming trace-path message.
 *  Returns 0 if the path was fully recovered, -1 on error and 1 if the
 *  message was processed and send to another agent. */
static int tracePathProcessMsg(plan_ma_msg_t *msg,
                               const plan_search_t *search,
                               int state_pool_pub_reg,
                               plan_ma_comm_t *comm,
                               plan_path_t *path);


/** Solution verification object */
struct _solution_verify_t {
    plan_ma_snapshot_t snapshot;
    plan_ma_search_t *ma;
    plan_cost_t lowest_cost;
    plan_ma_msg_t *init_msg;
    int ack;
};
typedef struct _solution_verify_t solution_verify_t;

#define SOLUTION_VERIFY(s) bor_container_of((s), solution_verify_t, snapshot)
#if 0
#define DBG_SOLUTION_VERIFY(ver, M) \
    fprintf(stderr, "[%d] T%ld " M " lowest_cost: %d, ack: %d, goal-cost: %d\n", \
            (ver)->ma->comm->node_id, (ver)->snapshot.token, \
            (ver)->lowest_cost, (ver)->ack, \
            ((ver)->init_msg ? planMAMsgPublicStateCost((ver)->init_msg) : -1))
#else
#define DBG_SOLUTION_VERIFY(ver, M)
#endif

/** Starts verification of a solution */
static void solutionVerify(plan_ma_search_t *ma, plan_state_id_t goal);
static solution_verify_t *solutionVerifyNew(plan_ma_search_t *ma,
                                            plan_ma_msg_t *msg,
                                            int initiator);
static void solutionVerifyDel(plan_ma_snapshot_t *s);
static void solutionVerifyUpdate(plan_ma_snapshot_t *s,
                                 plan_ma_msg_t *msg);
static void solutionVerifyInitMsg(plan_ma_snapshot_t *s,
                                  plan_ma_msg_t *msg);
static void solutionVerifyResponseMsg(plan_ma_snapshot_t *s,
                                      plan_ma_msg_t *msg);
static int solutionVerifyMarkFinalize(plan_ma_snapshot_t *s);
static void solutionVerifyResponseFinalize(plan_ma_snapshot_t *s);


/** Struct for verification of global dead end using distributed snapshot */
struct _dead_end_verify_t {
    plan_ma_snapshot_t snapshot;
    plan_ma_search_t *ma;
    plan_ma_msg_t *init_msg;
    int ack;
};
typedef struct _dead_end_verify_t dead_end_verify_t;

#define DEAD_END_VERIFY(s) bor_container_of((s), dead_end_verify_t, snapshot)

/** Starts verification of global dead end */
static void deadEndVerify(plan_ma_search_t *ma);
static dead_end_verify_t *deadEndVerifyNew(plan_ma_search_t *ma,
                                           plan_ma_msg_t *msg,
                                           int initiator);
static void deadEndVerifyDel(plan_ma_snapshot_t *s);
static void deadEndVerifyResponse(plan_ma_snapshot_t *s, plan_ma_msg_t *msg);
static void deadEndVerifyInitMsg(plan_ma_snapshot_t *s, plan_ma_msg_t *msg);
static void deadEndVerifyMarkMsg(plan_ma_snapshot_t *s, plan_ma_msg_t *msg);
static int deadEndVerifyMarkFinalize(plan_ma_snapshot_t *s);
static void deadEndVerifyResponseFinalize(plan_ma_snapshot_t *s);




void planMASearchParamsInit(plan_ma_search_params_t *params)
{
    bzero(params, sizeof(*params));
}

plan_ma_search_t *planMASearchNew(plan_ma_search_params_t *params)
{
    plan_ma_search_t *ma_search;
    pub_state_data_t msg_init;

    ma_search = BOR_ALLOC(plan_ma_search_t);
    ma_search->search = params->search;
    ma_search->comm = params->comm;
    ma_search->ma_state = planMAStateNew(ma_search->search->state_pool,
                                         ma_search->comm->node_size,
                                         ma_search->comm->node_id);
    ma_search->solution_verify = params->verify_solution;

    msg_init.agent_id = -1;
    msg_init.state_id = PLAN_NO_STATE;
    ma_search->pub_state_reg = planStatePoolDataReserve(ma_search->search->state_pool,
                                                        sizeof(pub_state_data_t),
                                                        NULL, &msg_init);
    planMASnapshotRegInit(&ma_search->snapshot, ma_search->comm->node_size);

    planPathInit(&ma_search->path);
    ma_search->res = -1;
    ma_search->goal = PLAN_NO_STATE;
    ma_search->goal_cost = PLAN_COST_MAX;
    ma_search->blocked = 0;
    ma_search->terminate = 0;
    ma_search->term.is_initiator = 0;
    ma_search->term.initiator_id = INT_MAX;

    ma_search->heur = NULL;
    if (ma_search->search->heur->ma){
        ma_search->heur = ma_search->search->heur;
        planHeurMAInit(ma_search->heur, ma_search->comm->node_size,
                       ma_search->comm->node_id, ma_search->ma_state);
    }

    return ma_search;
}

void planMASearchDel(plan_ma_search_t *ma_search)
{
    planMAStateDel(ma_search->ma_state);
    planMASnapshotRegFree(&ma_search->snapshot);
    planPathFree(&ma_search->path);
    BOR_FREE(ma_search);
}

int planMASearchRun(plan_ma_search_t *ma, plan_path_t *path)
{
    plan_path_t dummy_path;

    planSearchSetPostStep(ma->search, searchPostStep, ma);
    planSearchSetExpandedNode(ma->search, searchExpandedNode, ma);
    planSearchSetReachedGoal(ma->search, searchReachedGoal, ma);
    planSearchSetMAHeur(ma->search, searchMAHeur, ma);

    planPathInit(&dummy_path);
    planSearchRun(ma->search, &dummy_path);
    planPathFree(&dummy_path);

    if (ma->res == PLAN_SEARCH_FOUND)
        planPathCopy(path, &ma->path);

    // TODO: Recover original search callbacks

    return ma->res;
}

void planMASearchAbort(plan_ma_search_t *search)
{
    planSearchAbort(search->search);
}


static int searchPostStep(plan_search_t *search, int res, void *ud)
{
    plan_ma_search_t *ma = ud;
    plan_ma_msg_t *msg = NULL;

    if (res == PLAN_SEARCH_FOUND){
        res = PLAN_SEARCH_CONT;

    }else if (res == PLAN_SEARCH_ABORT){
        // Initialize termination of a whole cluster
        terminate(ma);

    }else if (res == PLAN_SEARCH_NOT_FOUND){
        // Block until some message unblocks the process
        ma->blocked = 1;
        msg = planMACommRecvBlock(ma->comm, DEAD_END_BLOCK_TIME);
        while (msg == NULL){
            if (ma->comm->node_id == 0)
                deadEndVerify(ma);
            msg = planMACommRecvBlock(ma->comm, DEAD_END_BLOCK_TIME);
        }
        processMsg(ma, msg);
        planMAMsgDel(msg);
        res = PLAN_SEARCH_CONT;
        ma->blocked = 0;
    }

    // Process all messages -- non-blocking
    while (!ma->terminate && (msg = planMACommRecv(ma->comm)) != NULL){
        processMsg(ma, msg);
        planMAMsgDel(msg);
    }

    // If we are in termination process, ignore all messages except
    // terminate messages
    while (ma->terminate == 1 && (msg = planMACommRecvBlock(ma->comm, -1)) != NULL){
        if (planMAMsgType(msg) == PLAN_MA_MSG_TERMINATE
                && terminateMsg(ma, msg) != 0){
            ma->terminate = 2;
        }

        planMAMsgDel(msg);
    }

    // If we were terminated, set-up result flag and terminate underlying
    // search algorithm.
    if (ma->terminate == 2){
        if (ma->res != PLAN_SEARCH_FOUND)
            ma->res = PLAN_SEARCH_NOT_FOUND;
        res = ma->res;
    }

    return res;
}

static void searchExpandedNode(plan_search_t *search,
                               plan_state_space_node_t *node, void *ud)
{
    plan_ma_search_t *ma = ud;
    publicStateSend(ma, node);
}

static void searchReachedGoal(plan_search_t *search,
                              plan_state_space_node_t *node, void *ud)
{
    plan_ma_search_t *ma = ud;

    if (node->cost < ma->goal_cost || node->state_id == ma->goal){
        ma->goal = node->state_id;
        ma->goal_cost = node->cost;

        if (ma->solution_verify){
            solutionVerify(ma, ma->goal);
        }else{
            tracePath(ma, ma->goal);
        }
    }
}


static void searchMAHeur(plan_search_t *search, plan_heur_t *heur,
                         plan_state_id_t state_id, plan_heur_res_t *res,
                         void *userdata)
{
    plan_ma_search_t *ma = (plan_ma_search_t *)userdata;
    plan_ma_msg_t *msg;
    int ret;

    if (ma->heur != heur){
        fprintf(stderr, "[%d] MASearch Error: Heur objects differ, something is"
                        " probably wrong!!\n", ma->comm->node_id);
        res->heur = PLAN_HEUR_DEAD_END;
        return;
    }

    ret = planHeurMANode(heur, ma->comm, state_id, search, res);
    while (ret == -1
            && !ma->terminate
            && (msg = planMACommRecvBlock(ma->comm, 0)) != NULL){
        if (planMAMsgType(msg) == PLAN_MA_MSG_HEUR
                && planMAMsgHeurType(msg) == PLAN_MA_MSG_HEUR_UPDATE){
            ret = planHeurMAUpdate(ma->heur, ma->comm, msg, res);
        }else{
            processMsg(ma, msg);
        }
        planMAMsgDel(msg);
    }
}

static void processMsg(plan_ma_search_t *ma, plan_ma_msg_t *msg)
{
    int type, snapshot_type;
    int res;
    plan_ma_snapshot_t *snapshot = NULL;
    solution_verify_t *ver;
    dead_end_verify_t *dead_end_ver;

    type = planMAMsgType(msg);
    if (type == PLAN_MA_MSG_TERMINATE){
        ma->terminate = 1;
        if (terminateMsg(ma, msg) != 0)
            ma->terminate = 2;
        return;
    }

    if (!planMASnapshotRegEmpty(&ma->snapshot)
            || type == PLAN_MA_MSG_SNAPSHOT){

        // Process message in snapshot object(s)
        if (planMASnapshotRegMsg(&ma->snapshot, msg) != 0){
            // Create snapshot object if the message wasn't accepted (and
            // thus we know the message is of type PLAN_MA_MSG_SNAPSHOT).
            snapshot_type = planMAMsgSnapshotType(msg);
            if (snapshot_type == PLAN_MA_MSG_SOLUTION_VERIFICATION){
                ver = solutionVerifyNew(ma, msg, 0);
                snapshot = &ver->snapshot;

            }else if (snapshot_type == PLAN_MA_MSG_DEAD_END_VERIFICATION){
                dead_end_ver = deadEndVerifyNew(ma, msg, 0);
                snapshot = &dead_end_ver->snapshot;
            }

            if (snapshot){
                planMASnapshotRegAdd(&ma->snapshot, snapshot);
                planMASnapshotRegMsg(&ma->snapshot, msg);
            }
        }
    }

    if (type == PLAN_MA_MSG_PUBLIC_STATE){
        publicStateRecv(ma, msg);

    }else if (type == PLAN_MA_MSG_TRACE_PATH){
        res = tracePathProcessMsg(msg, ma->search, ma->pub_state_reg,
                                  ma->comm, &ma->path);
        if (res == 0){
            ma->res = PLAN_SEARCH_FOUND;
            terminate(ma);
        }else if (res == -1){
            ma->res = PLAN_SEARCH_ABORT;
            terminate(ma);
        }

    }else if (type == PLAN_MA_MSG_HEUR){
        if (planMAMsgHeurType(msg) == PLAN_MA_MSG_HEUR_REQUEST && ma->heur){
            planHeurMARequest(ma->heur, ma->comm, msg);
        }else{
            fprintf(stderr, "[%d] MASearch Error: Unexpected heur message"
                            " (%d) from %d.\n",
                    ma->comm->node_id, planMAMsgHeurType(msg),
                    planMAMsgAgent(msg));
        }
    }

    return;
}

static int publicStateSet(plan_ma_state_t *ma_state,
                          plan_ma_msg_t *msg,
                          plan_state_space_node_t *node)
{
    if (planMAStateSetMAMsg(ma_state, node->state_id, msg) != 0)
        return -1;
    planMAMsgSetStateCost(msg, node->cost);
    planMAMsgSetStateHeur(msg, node->heuristic);
    return 0;
}

static int publicStateSet2(plan_ma_state_t *ma_state,
                           plan_ma_msg_t *msg,
                           plan_state_space_t *state_space,
                           plan_state_id_t state_id)
{
    plan_state_space_node_t *node;
    node = planStateSpaceNode(state_space, state_id);
    return publicStateSet(ma_state, msg, node);
}

static void publicStateSend(plan_ma_search_t *ma,
                            plan_state_space_node_t *node)
{
    int i;
    plan_ma_msg_t *msg;

    if (node->op == NULL || node->op->is_private)
        return;

    // Don't send states that are worse than the best goal so far
    if (node->cost >= ma->goal_cost)
        return;

    msg = planMAMsgNew(PLAN_MA_MSG_PUBLIC_STATE, 0, ma->comm->node_id);
    publicStateSet(ma->ma_state, msg, node);

    for (i = 0; i < ma->comm->node_size; ++i){
        if (i != ma->comm->node_id)
            planMACommSendToNode(ma->comm, i, msg);
    }
    planMAMsgDel(msg);
}

static void publicStateRecv(plan_ma_search_t *ma,
                            plan_ma_msg_t *msg)
{
    int cost, heur;
    pub_state_data_t *pub_state;
    plan_state_id_t state_id;
    plan_state_space_node_t *node;

    // Unroll data from the message
    cost         = planMAMsgStateCost(msg);
    heur         = planMAMsgStateHeur(msg);

    // Skip nodes that are worse than the best goal state so far
    if (cost >= ma->goal_cost)
        return;

    // Insert packed state into state-pool if not already inserted
    state_id = planMAStateInsertFromMAMsg(ma->ma_state, msg);

    // Get public state reference data
    pub_state = planStatePoolData(ma->search->state_pool,
                                  ma->pub_state_reg, state_id);

    // Get corresponding node
    node = planStateSpaceNode(ma->search->state_space, state_id);

    // TODO: Heuristic re-computation

    if (planStateSpaceNodeIsNew(node) || node->cost > cost){
        // Insert node into open-list of not already there
        node->parent_state_id = PLAN_NO_STATE;
        node->op              = NULL;
        node->cost            = cost;

        if (node->heuristic == -1){
            node->heuristic = heur;
        }else{
            node->heuristic = BOR_MAX(heur, node->heuristic);
        }

        pub_state->agent_id = planMAMsgAgent(msg);
        pub_state->state_id = planMAMsgStateId(msg);

        planSearchInsertNode(ma->search, node);
    }
}


static void terminate(plan_ma_search_t *ma)
{
    plan_ma_msg_t *msg;

    if (ma->terminate)
        return;

    msg = planMAMsgNew(PLAN_MA_MSG_TERMINATE,
                       PLAN_MA_MSG_TERMINATE_REQUEST,
                       ma->comm->node_id);
    planMAMsgSetTerminateAgent(msg, ma->comm->node_id);
    planMAMsgSetSearchRes(msg, ma->res);
    planMAMsgTracePathAppendPath(msg, &ma->path);
    planMACommSendInRing(ma->comm, msg);
    planMAMsgDel(msg);

    ma->terminate = 1;
    ma->term.is_initiator = 1;
}

static void terminateSendFinalFin(plan_ma_search_t *ma_search)
{
    plan_ma_msg_t *term_msg;

    term_msg = planMAMsgNew(PLAN_MA_MSG_TERMINATE,
                            PLAN_MA_MSG_TERMINATE_FINAL_FIN,
                            ma_search->comm->node_id);
    planMACommSendToAll(ma_search->comm, term_msg);
    planMAMsgDel(term_msg);
}

static void terminateSendFinal(plan_ma_search_t *ma_search, int send_plan)
{
    plan_ma_msg_t *term_msg;
    int i, to;

    if (ma_search->term.final_ack_target == 0)
        return;

    term_msg = planMAMsgNew(PLAN_MA_MSG_TERMINATE,
                            PLAN_MA_MSG_TERMINATE_FINAL,
                            ma_search->comm->node_id);
    if (send_plan){
        planMAMsgSetSearchRes(term_msg, ma_search->res);
        planMAMsgTracePathAppendPath(term_msg, &ma_search->path);
    }

    to = ma_search->comm->node_id + 1;
    to = to % ma_search->comm->node_size;
    for (i = 0; i < ma_search->term.final_ack_target; ++i){
        planMACommSendToNode(ma_search->comm, to, term_msg);
        to = (to + 1) % ma_search->comm->node_size;
    }
    planMAMsgDel(term_msg);
}

static void terminateSendFinalAck(plan_ma_search_t *ma_search)
{
    plan_ma_msg_t *term_msg;
    int i, to, node_size;

    if (ma_search->term.final_target == 0)
        return;

    term_msg = planMAMsgNew(PLAN_MA_MSG_TERMINATE,
                            PLAN_MA_MSG_TERMINATE_FINAL_ACK,
                            ma_search->comm->node_id);

    node_size = ma_search->comm->node_size;
    to = ma_search->comm->node_id - 1;
    to = (to + node_size) % node_size;
    for (i = 0; i < ma_search->term.final_target; ++i){
        planMACommSendToNode(ma_search->comm, to, term_msg);
        to = (to + node_size - 1) % node_size;
    }
    planMAMsgDel(term_msg);
}

static void terminatePrepareTargets(plan_ma_search_t *ma_search,
                                    int initiator_id)
{
    int target;

    if (initiator_id == -1){
        ma_search->term.is_first = 1;
        ma_search->term.is_last = 0;
        ma_search->term.final_counter = 0;
        ma_search->term.final_target = 0;
        ma_search->term.final_ack_counter = 0;
        ma_search->term.final_ack_target = ma_search->comm->node_size - 1;

    }else{
        ma_search->term.is_first = 0;
        ma_search->term.is_last = 0;
        ma_search->term.final_counter = 0;
        ma_search->term.final_ack_counter = 0;

        target = ma_search->comm->node_id - initiator_id;
        target = target + ma_search->comm->node_size;
        target = target % ma_search->comm->node_size;
        ma_search->term.final_target = target;
        target = ma_search->comm->node_size - 1 - target;
        ma_search->term.final_ack_target = target;
        if (ma_search->term.final_ack_target == 0)
            ma_search->term.is_last = 1;
    }
}

static int terminateMsg(plan_ma_search_t *ma_search,
                        plan_ma_msg_t *msg)
{
    int subtype = planMAMsgSubType(msg);
    int agent_id;

    if (subtype == PLAN_MA_MSG_TERMINATE_FINAL_FIN){
        return -1;

    }else if (subtype == PLAN_MA_MSG_TERMINATE_FINAL){
        if (planMAMsgAgent(msg) == ma_search->term.initiator_id){
            ma_search->res = planMAMsgSearchRes(msg);
            if (!planPathEmpty(&ma_search->path)){
                planPathFree(&ma_search->path);
                planPathInit(&ma_search->path);
            }
            planMAMsgTracePathExtractPath(msg, &ma_search->path);
        }
        ++ma_search->term.final_counter;

    }else if (subtype == PLAN_MA_MSG_TERMINATE_FINAL_ACK){
        ++ma_search->term.final_ack_counter;

    }else{ // PLAN_MA_MSG_TERMINATE_REQUEST
        agent_id = planMAMsgTerminateAgent(msg);
        if (agent_id == ma_search->comm->node_id){
            // The REQ message circled and arrived back to the initiator.
            // Initialize flushing message queues.
            ma_search->term.initiator_id = agent_id;
            terminatePrepareTargets(ma_search, -1);
            terminateSendFinal(ma_search, 1);

        }else{
            if (ma_search->term.is_initiator
                    && agent_id > ma_search->comm->node_id)
                return 0;

            if (agent_id < ma_search->term.initiator_id){
                ma_search->term.initiator_id = agent_id;
                terminatePrepareTargets(ma_search, agent_id);
            }
            planMACommSendInRing(ma_search->comm, msg);
        }
    }

    if (ma_search->term.final_counter == ma_search->term.final_target){
        if (ma_search->term.is_last){
            terminateSendFinalAck(ma_search);
        }else if (!ma_search->term.is_first){
            terminateSendFinal(ma_search, 0);
        }
        ma_search->term.final_counter = -1;
    }

    if (ma_search->term.final_ack_counter == ma_search->term.final_ack_target){
        if (!ma_search->term.is_first && !ma_search->term.is_last){
            terminateSendFinalAck(ma_search);
        }
        ma_search->term.final_ack_counter = -1;
    }

    if (ma_search->term.final_ack_counter == -1
            && ma_search->term.final_counter == -1
            && ma_search->term.is_first){
        terminateSendFinalFin(ma_search);
        return -1;
    }

    return 0;
}


static void tracePath(plan_ma_search_t *ma, plan_state_id_t goal_state)
{
    int trace_path;

    trace_path = tracePathInit(ma->search, goal_state,
                               ma->pub_state_reg, ma->comm, &ma->path);
    if (trace_path == -1){
        ma->res = PLAN_SEARCH_ABORT;
        terminate(ma);

    }else if (trace_path == 0){
        ma->res = PLAN_SEARCH_FOUND;
        terminate(ma);
    }
}

static int tracePathInit(const plan_search_t *search,
                         plan_state_id_t goal_state,
                         int state_pool_pub_reg,
                         plan_ma_comm_t *comm,
                         plan_path_t *path_out)
{
    plan_state_id_t init_state;
    plan_ma_msg_t *msg;
    const pub_state_data_t *pub_state;
    plan_path_t path;

    planPathInit(&path);
    init_state = planSearchExtractPath(search, goal_state, &path);
    if (init_state == search->initial_state){
        planPathCopy(path_out, &path);
        planPathFree(&path);
        return 0;

    }else if (init_state == PLAN_NO_STATE){
        planPathFree(&path);
        return -1;
    }

    pub_state = planStatePoolData(search->state_pool, state_pool_pub_reg,
                                  init_state);
    if (pub_state->agent_id == -1){
        planPathFree(&path);
        return -1;
    }

    msg = planMAMsgNew(PLAN_MA_MSG_TRACE_PATH, 0, comm->node_id);
    planMAMsgTracePathAppendPath(msg, &path);
    planMAMsgSetStateId(msg, pub_state->state_id);
    planMACommSendToNode(comm, pub_state->agent_id, msg);
    planMAMsgDel(msg);

    planPathFree(&path);
    return 1;
}

static int tracePathProcessMsg(plan_ma_msg_t *msg,
                               const plan_search_t *search,
                               int state_pool_pub_reg,
                               plan_ma_comm_t *comm,
                               plan_path_t *path_out)
{
    plan_state_id_t state_id;
    const pub_state_data_t *pub_state;
    int init_agent;
    plan_path_t path;

    // Get state id from which to trace path. If this agent was the
    // initiator (thus state_id == -1) return extracted path.
    state_id = planMAMsgStateId(msg);
    if (state_id == -1){
        planMAMsgTracePathExtractPath(msg, path_out);
        return 0;
    }

    // Trace next part of the path
    planPathInit(&path);
    state_id = planSearchExtractPath(search, state_id, &path);
    if (state_id == PLAN_NO_STATE){
        planPathFree(&path);
        return -1;
    }

    // Add the partial path to the message
    planMAMsgTracePathAppendPath(msg, &path);
    planPathFree(&path);

    // If the path was traced to the initial state check whether this agent
    // is the initiator.
    if (state_id == search->initial_state){
        init_agent = planMAMsgInitAgent(msg);

        if (init_agent == comm->node_id){
            // If the current agent is the initiator, extract path and
            // return zero
            planMAMsgTracePathExtractPath(msg, path_out);
            return 0;

        }else{
            // Send the message to the initiator
            planMAMsgSetStateId(msg, -1);
            planMACommSendToNode(comm, init_agent, msg);
            return 1;
        }
    }

    // Find out owner of the public state
    pub_state = planStatePoolData(search->state_pool, state_pool_pub_reg,
                                  state_id);
    if (pub_state->agent_id == -1){
        return -1;
    }

    // Send the message to the owner of the public state
    planMAMsgSetStateId(msg, pub_state->state_id);
    planMACommSendToNode(comm, pub_state->agent_id, msg);
    return 1;
}



static void solutionVerify(plan_ma_search_t *ma, plan_state_id_t goal)
{
    plan_ma_msg_t *msg;
    solution_verify_t *ver;

    // Create snapshot-init message
    msg = planMAMsgNew(PLAN_MA_MSG_SNAPSHOT, PLAN_MA_MSG_SNAPSHOT_INIT,
                       ma->comm->node_id);
    planMAMsgSetSnapshotType(msg, PLAN_MA_MSG_SOLUTION_VERIFICATION);
    publicStateSet2(ma->ma_state, msg, ma->search->state_space, goal);

    // Create snapshot object and register it
    ver = solutionVerifyNew(ma, msg, 1);
    planMASnapshotRegAdd(&ma->snapshot, &ver->snapshot);

    DBG_SOLUTION_VERIFY(ver, "solutionVerify");

    // Send message to all agents
    planMACommSendToAll(ma->comm, msg);
    planMAMsgDel(msg);
}

static solution_verify_t *solutionVerifyNew(plan_ma_search_t *ma,
                                            plan_ma_msg_t *msg,
                                            int initiator)
{
    solution_verify_t *ver;
    plan_ma_msg_t *mark_msg;

    ver = BOR_ALLOC(solution_verify_t);
    ver->ma = ma;
    ver->init_msg = NULL;
    ver->ack = 1;

    // Initialize to lowest currently known value
    ver->lowest_cost = planSearchTopNodeCost(ma->search);
    ver->lowest_cost = BOR_MIN(ver->lowest_cost, ma->goal_cost);

    if (initiator){
        ver->init_msg = planMAMsgClone(msg);
        _planMASnapshotInit(&ver->snapshot, planMAMsgSnapshotToken(msg),
                            ma->comm->node_id, ma->comm->node_size,
                            solutionVerifyDel,
                            solutionVerifyUpdate,
                            NULL,
                            NULL,
                            solutionVerifyResponseMsg,
                            NULL,
                            solutionVerifyResponseFinalize);
        DBG_SOLUTION_VERIFY(ver, "new");
    }else{
        _planMASnapshotInit(&ver->snapshot, planMAMsgSnapshotToken(msg),
                            ma->comm->node_id, ma->comm->node_size,
                            solutionVerifyDel,
                            solutionVerifyUpdate,
                            solutionVerifyInitMsg,
                            NULL,
                            NULL,
                            solutionVerifyMarkFinalize,
                            NULL);
        DBG_SOLUTION_VERIFY(ver, "new");

        // Send mark message to all agents
        mark_msg = planMAMsgSnapshotNewMark(msg, ma->comm->node_id);
        planMACommSendToAll(ma->comm, mark_msg);
        planMAMsgDel(mark_msg);
    }

    return ver;
}

static void solutionVerifyDel(plan_ma_snapshot_t *s)
{
    solution_verify_t *ver = SOLUTION_VERIFY(s);
    DBG_SOLUTION_VERIFY(ver, "del");
    _planMASnapshotFree(s);
    if (ver->init_msg)
        planMAMsgDel(ver->init_msg);
    BOR_FREE(ver);
}

static void solutionVerifyUpdate(plan_ma_snapshot_t *s,
                                 plan_ma_msg_t *msg)
{
    solution_verify_t *ver = SOLUTION_VERIFY(s);
    plan_cost_t cost;

    if (planMAMsgType(msg) != PLAN_MA_MSG_PUBLIC_STATE)
        return;

    // Update lowest cost from the public state received before snapshot-mark
    cost = planMAMsgStateCost(msg);
    ver->lowest_cost = BOR_MIN(ver->lowest_cost, cost);
    DBG_SOLUTION_VERIFY(ver, "update");
}

static void solutionVerifyInitMsg(plan_ma_snapshot_t *s,
                                  plan_ma_msg_t *msg)
{
    solution_verify_t *ver = SOLUTION_VERIFY(s);
    ver->init_msg = planMAMsgClone(msg);
    DBG_SOLUTION_VERIFY(ver, "init-msg");
}

static void solutionVerifyResponseMsg(plan_ma_snapshot_t *s,
                                      plan_ma_msg_t *msg)
{
    solution_verify_t *ver = SOLUTION_VERIFY(s);
    ver->ack &= planMAMsgSnapshotAck(msg);
    DBG_SOLUTION_VERIFY(ver, "response-msg");
}

static void reinsertNAckedSolution(plan_ma_search_t *ma,
                                   plan_ma_msg_t *msg)
{
    int cost, heur;
    pub_state_data_t *pub_state;
    plan_state_id_t state_id;
    plan_state_space_node_t *node;

    // Unroll data from the message
    cost         = planMAMsgStateCost(msg);
    heur         = planMAMsgStateHeur(msg);

    // Skip nodes that are worse than the best goal state so far
    if (cost > ma->goal_cost)
        return;

    // Insert packed state into state-pool if not already inserted
    state_id = planMAStateInsertFromMAMsg(ma->ma_state, msg);

    // Get corresponding node
    node = planStateSpaceNode(ma->search->state_space, state_id);

    // Set up node's data if not already created better state
    if (planStateSpaceNodeIsNew(node) || node->cost > cost){
        node->parent_state_id = PLAN_NO_STATE;
        node->op              = NULL;
        node->cost            = cost;
        node->heuristic       = heur;

        // Get public state reference data and set them if not already set
        pub_state = planStatePoolData(ma->search->state_pool,
                                      ma->pub_state_reg, state_id);
        pub_state->agent_id = planMAMsgAgent(msg);
        pub_state->state_id = planMAMsgStateId(msg);
    }

    planSearchInsertNode(ma->search, node);
}

static int solutionVerifyMarkFinalize(plan_ma_snapshot_t *s)
{
    solution_verify_t *ver = SOLUTION_VERIFY(s);
    plan_ma_msg_t *msg;
    int ack;

    ack = 0;
    if (ver->lowest_cost >= planMAMsgStateCost(ver->init_msg))
        ack = 1;

    DBG_SOLUTION_VERIFY(ver, "mark-final");

    // Construct response message and send it to the initiator
    msg = planMAMsgSnapshotNewResponse(ver->init_msg, ver->ma->comm->node_id);
    planMAMsgSetSnapshotAck(msg, ack);
    planMACommSendToNode(ver->ma->comm, planMAMsgAgent(ver->init_msg), msg);
    planMAMsgDel(msg);

    if (ack == 0){
        reinsertNAckedSolution(ver->ma, ver->init_msg);
    }

    return -1;
}

static void solutionVerifyResponseFinalize(plan_ma_snapshot_t *s)
{
    solution_verify_t *ver = SOLUTION_VERIFY(s);
    plan_state_space_node_t *node;
    plan_cost_t goal_cost;
    plan_state_id_t goal_id;

    goal_cost = planMAMsgStateCost(ver->init_msg);

    // Ignore the solution if we have already seen better
    if (ver->ma->goal_cost < goal_cost)
        return;

    if (ver->ack && ver->lowest_cost >= goal_cost){
        // All other agents ack'ed the solution and this agent also has
        // processed all states with lower cost, so we are done.
        ver->ma->goal_cost = goal_cost;
        ver->ma->goal = planMAMsgStateId(ver->init_msg);
        DBG_SOLUTION_VERIFY(ver, "response-final-trace-path");
        tracePath(ver->ma, planMAMsgStateId(ver->init_msg));

    }else if (ver->lowest_cost < goal_cost){
        goal_id = planMAMsgStateId(ver->init_msg);
        node = planStateSpaceNode(ver->ma->search->state_space, goal_id);
        planSearchInsertNode(ver->ma->search, node);
        DBG_SOLUTION_VERIFY(ver, "response-final-ins");
    }
}

static void deadEndVerify(plan_ma_search_t *ma)
{
    plan_ma_msg_t *msg;
    dead_end_verify_t *ver;

    // Create snapshot-init message
    msg = planMAMsgNew(PLAN_MA_MSG_SNAPSHOT, PLAN_MA_MSG_SNAPSHOT_INIT,
                       ma->comm->node_id);
    planMAMsgSetSnapshotType(msg, PLAN_MA_MSG_DEAD_END_VERIFICATION);

    // Create snapshot object and register it
    ver = deadEndVerifyNew(ma, msg, 1);
    planMASnapshotRegAdd(&ma->snapshot, &ver->snapshot);

    // Send message to all agents
    planMACommSendToAll(ma->comm, msg);
    planMAMsgDel(msg);
}

static dead_end_verify_t *deadEndVerifyNew(plan_ma_search_t *ma,
                                           plan_ma_msg_t *msg,
                                           int initiator)
{
    dead_end_verify_t *ver;
    plan_ma_msg_t *mark_msg;

    ver = BOR_ALLOC(dead_end_verify_t);
    ver->ma = ma;
    ver->ack = 1;
    ver->init_msg = NULL;

    if (initiator){
        ver->init_msg = planMAMsgClone(msg);
        _planMASnapshotInit(&ver->snapshot, planMAMsgSnapshotToken(msg),
                            ma->comm->node_id, ma->comm->node_size,
                            deadEndVerifyDel,
                            NULL,
                            NULL,
                            NULL,
                            deadEndVerifyResponse,
                            NULL,
                            deadEndVerifyResponseFinalize);
    }else{
        _planMASnapshotInit(&ver->snapshot, planMAMsgSnapshotToken(msg),
                            ma->comm->node_id, ma->comm->node_size,
                            deadEndVerifyDel,
                            NULL,
                            deadEndVerifyInitMsg,
                            deadEndVerifyMarkMsg,
                            NULL,
                            deadEndVerifyMarkFinalize,
                            NULL);

        // Send mark message to all agents
        mark_msg = planMAMsgSnapshotNewMark(msg, ma->comm->node_id);
        planMACommSendToAll(ma->comm, mark_msg);
        planMAMsgDel(mark_msg);
    }

    return ver;
}

static void deadEndVerifyDel(plan_ma_snapshot_t *s)
{
    dead_end_verify_t *ver = DEAD_END_VERIFY(s);
    _planMASnapshotFree(s);
    if (ver->init_msg)
        planMAMsgDel(ver->init_msg);
    BOR_FREE(ver);
}

static void deadEndVerifyInitMsg(plan_ma_snapshot_t *s, plan_ma_msg_t *msg)
{
    dead_end_verify_t *ver = DEAD_END_VERIFY(s);
    ver->init_msg = planMAMsgClone(msg);
    ver->ack &= ver->ma->blocked;
}

static void deadEndVerifyMarkMsg(plan_ma_snapshot_t *s, plan_ma_msg_t *msg)
{
    dead_end_verify_t *ver = DEAD_END_VERIFY(s);
    ver->ack &= ver->ma->blocked;
}

static int deadEndVerifyMarkFinalize(plan_ma_snapshot_t *s)
{
    dead_end_verify_t *ver = DEAD_END_VERIFY(s);
    plan_ma_msg_t *msg;

    msg = planMAMsgSnapshotNewResponse(ver->init_msg, ver->ma->comm->node_id);
    planMAMsgSetSnapshotAck(msg, ver->ack);
    planMACommSendToNode(ver->ma->comm, planMAMsgAgent(ver->init_msg), msg);
    planMAMsgDel(msg);

    return -1;
}


static void deadEndVerifyResponse(plan_ma_snapshot_t *s, plan_ma_msg_t *msg)
{
    dead_end_verify_t *ver = DEAD_END_VERIFY(s);
    ver->ack &= planMAMsgSnapshotAck(msg);
    ver->ack &= ver->ma->blocked;
}

static void deadEndVerifyResponseFinalize(plan_ma_snapshot_t *s)
{
    dead_end_verify_t *ver = DEAD_END_VERIFY(s);
    if (!ver->ack)
        terminate(ver->ma);
}
