#include <boruvka/alloc.h>

#include "ma_search_th.h"
#include "ma_search_common.h"

/** Reference data for the received public states */
struct _pub_state_data_t {
    int agent_id;             /*!< ID of the source agent */
    plan_state_id_t state_id; /*!< ID of the state in remote agent's state
                                   pool. This is used for back-tracking. */
};
typedef struct _pub_state_data_t pub_state_data_t;

/** Solution verification object */
struct _solution_verify_t {
    plan_ma_snapshot_t snapshot;
    plan_ma_search_th_t *th;
    plan_cost_t lowest_cost;
    int initiator;
    plan_ma_msg_t *init_msg;
    int ack;
};
typedef struct _solution_verify_t solution_verify_t;

static void *searchThread(void *data);
static int searchThreadPostStep(plan_search_t *search, int res, void *ud);
static void searchThreadExpandedNode(plan_search_t *search,
                                     plan_state_space_node_t *node, void *ud);
static void searchThreadReachedGoal(plan_search_t *search,
                                    plan_state_space_node_t *node, void *ud);
static void processMsg(plan_ma_search_th_t *th, plan_ma_msg_t *msg);
static void publicStateSend(plan_ma_search_th_t *th,
                            plan_state_space_node_t *node);
static void publicStateRecv(plan_ma_search_th_t *th,
                            plan_ma_msg_t *msg);

/** Starts trace-path process */
static void tracePath(plan_ma_search_th_t *th, plan_state_id_t goal_state);
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

#define SOLUTION_VERIFY(s) bor_container_of((s), solution_verify_t, snapshot)
#if 0
#define DBG_SOLUTION_VERIFY(ver, M) \
    fprintf(stderr, "[%d] T%ld " M " lowest_cost: %d, ack: %d, goal-cost: %d\n", \
            (ver)->th->comm->node_id, (ver)->snapshot.token, \
            (ver)->lowest_cost, (ver)->ack, \
            ((ver)->init_msg ? planMAMsgPublicStateCost((ver)->init_msg) : -1))
#else
#define DBG_SOLUTION_VERIFY(ver, M)
#endif

/** Starts verification of a solution */
static void solutionVerify(plan_ma_search_th_t *th, plan_state_id_t goal);
static solution_verify_t *solutionVerifyNew(plan_ma_search_th_t *th,
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


void planMASearchThInit(plan_ma_search_th_t *th,
                        plan_search_t *search,
                        plan_ma_comm_t *comm,
                        plan_path_t *path)
{
    pub_state_data_t msg_init;

    th->search = search;
    th->comm = comm;
    th->path = path;
    th->solution_verify = 1; // TODO: Make it an option

    msg_init.agent_id = -1;
    msg_init.state_id = PLAN_NO_STATE;
    th->pub_state_reg = planStatePoolDataReserve(search->state_pool,
                                                 sizeof(pub_state_data_t),
                                                 NULL, &msg_init);
    borFifoSemInit(&th->msg_queue, sizeof(plan_ma_msg_t *));
    planMASnapshotRegInit(&th->snapshot, th->comm->node_size);

    th->res = -1;
    th->goal = PLAN_NO_STATE;
    th->goal_cost = PLAN_COST_MAX;
}

void planMASearchThFree(plan_ma_search_th_t *th)
{
    plan_ma_msg_t *msg;

    // Empty queue
    while (borFifoSemPop(&th->msg_queue, &msg) == 0){
        planMAMsgDel(msg);
    }
    borFifoSemFree(&th->msg_queue);
    planMASnapshotRegFree(&th->snapshot);
}

void planMASearchThRun(plan_ma_search_th_t *th)
{
    pthread_create(&th->thread, NULL, searchThread, th);
}

void planMASearchThJoin(plan_ma_search_th_t *th)
{
    plan_ma_msg_t *msg = NULL;

    // Push an empty message to unblock message queue
    borFifoSemPush(&th->msg_queue, &msg);

    pthread_join(th->thread, NULL);
}

static void *searchThread(void *data)
{
    plan_ma_search_th_t *th = data;

    planSearchSetPostStep(th->search, searchThreadPostStep, th);
    planSearchSetExpandedNode(th->search, searchThreadExpandedNode, th);
    planSearchSetReachedGoal(th->search, searchThreadReachedGoal, th);
    planSearchRun(th->search, th->path);

    return NULL;
}

static int searchThreadPostStep(plan_search_t *search, int res, void *ud)
{
    plan_ma_search_th_t *th = ud;
    plan_ma_msg_t *msg = NULL;

    if (res == PLAN_SEARCH_FOUND){
        res = PLAN_SEARCH_CONT;

    }else if (res == PLAN_SEARCH_ABORT){
        // Initialize termination of a whole cluster
        planMASearchTerminate(th->comm);

        // Ignore all messages except terminate (which is empty)
        do {
            borFifoSemPopBlock(&th->msg_queue, &msg);
            if (msg)
                planMAMsgDel(msg);
        } while (msg);

    }else if (res == PLAN_SEARCH_NOT_FOUND){
        // Block until some message unblocks the process
        borFifoSemPopBlock(&th->msg_queue, &msg);
        if (msg){
            processMsg(th, msg);
            planMAMsgDel(msg);
            res = PLAN_SEARCH_CONT;
        }else{
            return PLAN_SEARCH_ABORT;
        }
    }

    // Process all messages
    while (borFifoSemPop(&th->msg_queue, &msg) == 0){
        // Empty message means to terminate
        if (!msg)
            return PLAN_SEARCH_ABORT;

        // Process message
        processMsg(th, msg);
        planMAMsgDel(msg);
    }

    return res;
}

static void searchThreadExpandedNode(plan_search_t *search,
                                     plan_state_space_node_t *node, void *ud)
{
    plan_ma_search_th_t *th = ud;
    publicStateSend(th, node);
}

static void searchThreadReachedGoal(plan_search_t *search,
                                    plan_state_space_node_t *node, void *ud)
{
    plan_ma_search_th_t *th = ud;

    if (node->cost < th->goal_cost || node->state_id == th->goal){
        th->goal = node->state_id;
        th->goal_cost = node->cost;

        if (th->solution_verify){
            solutionVerify(th, th->goal);
        }else{
            tracePath(th, th->goal);
        }
    }
}

static void processMsg(plan_ma_search_th_t *th, plan_ma_msg_t *msg)
{
    int type, snapshot_type;
    int res;
    plan_ma_snapshot_t *snapshot = NULL;
    solution_verify_t *ver;

    if (th->res != PLAN_SEARCH_NOT_FOUND)
        return;

    type = planMAMsgType(msg);

    if (!planMASnapshotRegEmpty(&th->snapshot)
            || type == PLAN_MA_MSG_SNAPSHOT){

        // Process message in snapshot object(s)
        if (planMASnapshotRegMsg(&th->snapshot, msg) != 0){
            // Create snapshot object if the message wasn't accepted (and
            // thus we know the message is of type PLAN_MA_MSG_SNAPSHOT).
            snapshot_type = planMAMsgSnapshotType(msg);
            if (snapshot_type == PLAN_MA_MSG_SOLUTION_VERIFICATION){
                ver = solutionVerifyNew(th, msg, 0);
                snapshot = &ver->snapshot;
            }

            if (snapshot){
                planMASnapshotRegAdd(&th->snapshot, snapshot);
                planMASnapshotRegMsg(&th->snapshot, msg);
            }
        }
    }

    if (type == PLAN_MA_MSG_PUBLIC_STATE){
        publicStateRecv(th, msg);

    }else if (type == PLAN_MA_MSG_TRACE_PATH){
        res = tracePathProcessMsg(msg, th->search, th->pub_state_reg,
                                  th->comm, th->path);
        if (res == 0){
            th->res = PLAN_SEARCH_FOUND;
            planMASearchTerminate(th->comm);
        }else if (res == -1){
            th->res = PLAN_SEARCH_ABORT;
            planMASearchTerminate(th->comm);
        }

    }
}

static int publicStateSet(plan_ma_msg_t *msg,
                          plan_state_pool_t *state_pool,
                          plan_state_space_node_t *node)
{
    const void *statebuf;
    size_t statebuf_size;

    statebuf = planStatePoolGetPackedState(state_pool, node->state_id);
    statebuf_size = planStatePackerBufSize(state_pool->packer);
    if (statebuf == NULL)
        return -1;

    planMAMsgPublicStateSetState(msg, statebuf, statebuf_size,
                                 node->state_id, node->cost,
                                 node->heuristic);
    return 0;
}

static int publicStateSet2(plan_ma_msg_t *msg,
                           plan_state_pool_t *state_pool,
                           plan_state_space_t *state_space,
                           plan_state_id_t state_id)
{
    plan_state_space_node_t *node;
    node = planStateSpaceNode(state_space, state_id);
    return publicStateSet(msg, state_pool, node);
}

static void publicStateSend(plan_ma_search_th_t *th,
                            plan_state_space_node_t *node)
{
    int i, len;
    const int *peers;
    const pub_state_data_t *pub_state;
    plan_ma_msg_t *msg;

    if (node->op == NULL || planOpExtraMAOpIsPrivate(node->op))
        return;

    // Don't send states that are worse than the best goal so far
    if (node->cost >= th->goal_cost)
        return;

    // Do not resend public states from other agents
    pub_state = planStatePoolData(th->search->state_pool,
                                  th->pub_state_reg, node->state_id);
    if (pub_state->agent_id != -1)
        return;

    peers = planOpExtraMAOpRecvAgents(node->op, &len);
    if (len == 0)
        return;

    msg = planMAMsgNew(PLAN_MA_MSG_PUBLIC_STATE, 0, th->comm->node_id);
    publicStateSet(msg, th->search->state_pool, node);

    for (i = 0; i < len; ++i){
        if (peers[i] != th->comm->node_id)
            planMACommSendToNode(th->comm, peers[i], msg);
    }
    planMAMsgDel(msg);
}

static void publicStateRecv(plan_ma_search_th_t *th,
                            plan_ma_msg_t *msg)
{
    int cost, heur;
    pub_state_data_t *pub_state;
    plan_state_id_t state_id;
    plan_state_space_node_t *node;
    const void *packed_state;

    // Unroll data from the message
    packed_state = planMAMsgPublicStateStateBuf(msg);
    cost         = planMAMsgPublicStateCost(msg);
    heur         = planMAMsgPublicStateHeur(msg);

    // Skip nodes that are worse than the best goal state so far
    if (cost >= th->goal_cost)
        return;

    // Insert packed state into state-pool if not already inserted
    state_id = planStatePoolInsertPacked(th->search->state_pool,
                                         packed_state);

    // Get public state reference data
    pub_state = planStatePoolData(th->search->state_pool,
                                  th->pub_state_reg, state_id);

    // Get corresponding node
    node = planStateSpaceNode(th->search->state_space, state_id);

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
        pub_state->state_id = planMAMsgPublicStateStateId(msg);

        planSearchInsertNode(th->search, node);
    }
}


static void tracePath(plan_ma_search_th_t *th, plan_state_id_t goal_state)
{
    int trace_path;

    trace_path = tracePathInit(th->search, goal_state,
                               th->pub_state_reg, th->comm, th->path);
    if (trace_path == -1){
        th->res = PLAN_SEARCH_ABORT;
        planMASearchTerminate(th->comm);

    }else if (trace_path == 0){
        th->res = PLAN_SEARCH_FOUND;
        planMASearchTerminate(th->comm);
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
    if (init_state == 0){
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
    planMAMsgTracePathAddPath(msg, &path);
    planMAMsgTracePathSetStateId(msg, pub_state->state_id);
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
    state_id = planMAMsgTracePathStateId(msg);
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
    planMAMsgTracePathAddPath(msg, &path);
    planPathFree(&path);

    // If the path was traced to the initial state check whether this agent
    // is the initiator.
    if (state_id == search->initial_state){
        init_agent = planMAMsgTracePathInitAgent(msg);

        if (init_agent == comm->node_id){
            // If the current agent is the initiator, extract path and
            // return zero
            planMAMsgTracePathExtractPath(msg, path_out);
            return 0;

        }else{
            // Send the message to the initiator
            planMAMsgTracePathSetStateId(msg, -1);
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
    planMAMsgTracePathSetStateId(msg, pub_state->state_id);
    planMACommSendToNode(comm, pub_state->agent_id, msg);
    return 1;
}


static void solutionVerify(plan_ma_search_th_t *th, plan_state_id_t goal)
{
    plan_ma_msg_t *msg;
    solution_verify_t *ver;

    // Create snapshot-init message
    msg = planMAMsgNew(PLAN_MA_MSG_SNAPSHOT, PLAN_MA_MSG_SNAPSHOT_INIT,
                       th->comm->node_id);
    planMAMsgSnapshotSetType(msg, PLAN_MA_MSG_SOLUTION_VERIFICATION);
    publicStateSet2(msg, th->search->state_pool, th->search->state_space, goal);

    // Create snapshot object
    ver = solutionVerifyNew(th, msg, 1);
    planMASnapshotRegAdd(&th->snapshot, &ver->snapshot);

    DBG_SOLUTION_VERIFY(ver, "solutionVerify");

    // Send message to all agents
    planMACommSendToAll(th->comm, msg);
    planMAMsgDel(msg);
}

static solution_verify_t *solutionVerifyNew(plan_ma_search_th_t *th,
                                            plan_ma_msg_t *msg,
                                            int initiator)
{
    solution_verify_t *ver;
    plan_ma_msg_t *mark_msg;

    ver = BOR_ALLOC(solution_verify_t);
    ver->th = th;
    ver->initiator = initiator;
    ver->init_msg = NULL;
    ver->lowest_cost = planSearchTopNodeCost(th->search);
    ver->lowest_cost = BOR_MIN(ver->lowest_cost, th->goal_cost);
    ver->ack = 1;

    if (initiator){
        ver->init_msg = planMAMsgClone(msg);
        _planMASnapshotInit(&ver->snapshot, planMAMsgSnapshotToken(msg),
                            th->comm->node_id, th->comm->node_size,
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
                            th->comm->node_id, th->comm->node_size,
                            solutionVerifyDel,
                            solutionVerifyUpdate,
                            solutionVerifyInitMsg,
                            NULL,
                            NULL,
                            solutionVerifyMarkFinalize,
                            NULL);
        DBG_SOLUTION_VERIFY(ver, "new");

        // Send mark message to all agents
        mark_msg = planMAMsgSnapshotNewMark(msg, th->comm->node_id);
        planMACommSendToAll(th->comm, mark_msg);
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

    cost = planMAMsgPublicStateCost(msg);
    ver->lowest_cost = BOR_MIN(ver->lowest_cost, cost);
    DBG_SOLUTION_VERIFY(ver, "update");
}

static void solutionVerifyInitMsg(plan_ma_snapshot_t *s,
                                  plan_ma_msg_t *msg)
{
    solution_verify_t *ver = SOLUTION_VERIFY(s);
    if (ver->init_msg){
        // TODO
        fprintf(stderr, "[%d] Error: Already received snapshot-init message.\n",
                ver->th->comm->node_id);
        exit(-1);
    }
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

static int solutionVerifyMarkFinalize(plan_ma_snapshot_t *s)
{
    solution_verify_t *ver = SOLUTION_VERIFY(s);
    plan_ma_msg_t *msg;
    int ack;

    if (!ver->init_msg){
        // TODO
        fprintf(stderr, "Error: init msg not set!!\n");
        exit(-1);
    }

    ack = 0;
    if (ver->lowest_cost >= planMAMsgPublicStateCost(ver->init_msg))
        ack = 1;

    DBG_SOLUTION_VERIFY(ver, "mark-final");

    // Construct response message and send it to the initiator
    msg = planMAMsgSnapshotNewResponse(ver->init_msg, ver->th->comm->node_id);
    planMAMsgSnapshotSetAck(msg, ack);
    planMACommSendToNode(ver->th->comm, planMAMsgAgent(ver->init_msg), msg);
    planMAMsgDel(msg);

    return -1;
}

static void solutionVerifyResponseFinalize(plan_ma_snapshot_t *s)
{
    solution_verify_t *ver = SOLUTION_VERIFY(s);
    plan_state_space_node_t *node;
    plan_cost_t goal_cost;
    plan_state_id_t goal_id;

    goal_cost = planMAMsgPublicStateCost(ver->init_msg);
    if (ver->ack && ver->lowest_cost >= goal_cost){
        ver->th->goal_cost = goal_cost;
        ver->th->goal = planMAMsgPublicStateStateId(ver->init_msg);
        DBG_SOLUTION_VERIFY(ver, "response-final-trace-path");
        tracePath(ver->th, planMAMsgPublicStateStateId(ver->init_msg));
    }else{
        goal_id = planMAMsgPublicStateStateId(ver->init_msg);
        node = planStateSpaceNode(ver->th->search->state_space, goal_id);
        planSearchInsertNode(ver->th->search, node);
        DBG_SOLUTION_VERIFY(ver, "response-final-ins");
    }

}

