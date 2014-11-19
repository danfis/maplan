#include <boruvka/alloc.h>

#include "ma_search_th.h"
#include "ma_search_common.h"

/** Reference data for the received public states */
struct _pub_state_data_t {
    int agent_id;             /*!< ID of the source agent */
    plan_cost_t cost;         /*!< Cost of the path to this state as found
                                   by the remote agent. */
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
static void tracePath(plan_ma_search_th_t *th);
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
    msg_init.cost = PLAN_COST_MAX;
    msg_init.state_id = PLAN_NO_STATE;
    th->pub_state_reg = planStatePoolDataReserve(search->state_pool,
                                                 sizeof(pub_state_data_t),
                                                 NULL, &msg_init);
    borFifoSemInit(&th->msg_queue, sizeof(plan_ma_msg_t *));
    planMASnapshotRegInit(&th->snapshot, th->comm->node_size);

    th->res = -1;
    th->goal = PLAN_NO_STATE;
    th->goal_cost = PLAN_COST_MAX;
    th->lowest_cost = PLAN_COST_MAX;
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
    int type = -1;

    if (res == PLAN_SEARCH_FOUND){
        if (th->solution_verify){
            solutionVerify(th, th->goal);
        }else{
            tracePath(th);
        }

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
        // Block until public-state or terminate isn't received
        do {
            borFifoSemPopBlock(&th->msg_queue, &msg);
            if (msg){
                type = planMAMsgType(msg);
                processMsg(th, msg);
                planMAMsgDel(msg);
            }
        } while (msg && type != PLAN_MA_MSG_PUBLIC_STATE);

        // Empty message means to terminate
        if (!msg)
            return PLAN_SEARCH_ABORT;
        res = PLAN_SEARCH_CONT;
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
    th->lowest_cost = BOR_MIN(th->lowest_cost, node->cost);
    publicStateSend(th, node);
}

static void searchThreadReachedGoal(plan_search_t *search,
                                    plan_state_space_node_t *node, void *ud)
{
    plan_ma_search_th_t *th = ud;

    if (node->cost < th->goal_cost){
        th->goal = node->state_id;
        th->goal_cost = node->cost;
    }
}

static void processMsg(plan_ma_search_th_t *th, plan_ma_msg_t *msg)
{
    int type, snapshot_type;
    int res;
    plan_ma_snapshot_t *snapshot = NULL;

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
                // TODO
                snapshot = NULL;
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

static void publicStateSend(plan_ma_search_th_t *th,
                            plan_state_space_node_t *node)
{
    const void *statebuf;
    size_t statebuf_size;
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

    statebuf = planStatePoolGetPackedState(th->search->state_pool,
                                           node->state_id);
    statebuf_size = planStatePackerBufSize(th->search->state_pool->packer);
    if (statebuf == NULL)
        return;

    peers = planOpExtraMAOpRecvAgents(node->op, &len);
    if (len == 0)
        return;

    msg = planMAMsgNew(PLAN_MA_MSG_PUBLIC_STATE, 0, th->comm->node_id);
    planMAMsgPublicStateSetState(msg, statebuf, statebuf_size,
                                 node->state_id, node->cost,
                                 node->heuristic);

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

    if (planStateSpaceNodeIsNew(node)){
        // Insert node into open-list of not already there
        node->parent_state_id = PLAN_NO_STATE;
        node->op              = NULL;
        node->cost            = cost;
        node->heuristic       = heur;
        planSearchInsertNode(th->search, node);

    }else if (planStateSpaceNodeIsOpen(node) && node->heuristic > heur){
        // Reinsert node if already in open-list and we got better
        // heuristic
        planSearchInsertNode(th->search, node);

    }else if (planStateSpaceNodeIsClosed(node) && node->cost > cost){
        // If other agent found shorter path, set up the node so that path
        // will be traced trough that agent
        node->parent_state_id = PLAN_NO_STATE;
        node->op              = NULL;
        node->cost            = cost;
    }

    // Set the reference data only if the new cost is smaller than the
    // current one. This means that either the state is brand new or the
    // previously inserted state had bigger cost.
    if (pub_state->cost > cost){
        pub_state->agent_id = planMAMsgAgent(msg);
        pub_state->cost     = cost;
        pub_state->state_id = planMAMsgPublicStateStateId(msg);
    }
}


static void tracePath(plan_ma_search_th_t *th)
{
    int trace_path;

    trace_path = tracePathInit(th->search, th->search->goal_state,
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

    msg = planMAMsgNew(PLAN_MA_MSG_SNAPSHOT, PLAN_MA_MSG_SNAPSHOT_INIT,
                       th->comm->node_id);
    planMAMsgSnapshotSetType(msg, PLAN_MA_MSG_SOLUTION_VERIFICATION);
    // TODO: Create solutionVerifyNew(), set up public state, send to all
    planMAMsgDel(msg);
}

static solution_verify_t *solutionVerifyNew(plan_ma_search_th_t *th,
                                            plan_ma_msg_t *msg,
                                            int initiator)
{
    solution_verify_t *ver;
    int subtype;

    ver = BOR_ALLOC(solution_verify_t);
    ver->th = th;
    ver->initiator = initiator;
    ver->init_msg = NULL;

    subtype = planMAMsgSubType(msg);
    if (subtype == PLAN_MA_MSG_SNAPSHOT_INIT)
        ver->init_msg = planMAMsgClone(msg);

    if (initiator){
        _planMASnapshotInit(&ver->snapshot, planMAMsgSnapshotToken(msg),
                            th->comm->node_id, th->comm->node_size,
                            solutionVerifyDel,
                            solutionVerifyUpdate,
                            NULL,
                            NULL,
                            solutionVerifyResponseMsg,
                            NULL,
                            solutionVerifyResponseFinalize);
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
    }

    return ver;
}

static void solutionVerifyDel(plan_ma_snapshot_t *s)
{
    solution_verify_t *ver = SOLUTION_VERIFY(s);
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
}

static void solutionVerifyInitMsg(plan_ma_snapshot_t *s,
                                  plan_ma_msg_t *msg)
{
    solution_verify_t *ver = SOLUTION_VERIFY(s);
    if (ver->init_msg){
        // TODO
        fprintf(stderr, "Error: Already received snapshot-init message.\n");
        exit(-1);
    }
    ver->init_msg = planMAMsgClone(msg);
}

static void solutionVerifyResponseMsg(plan_ma_snapshot_t *s,
                                      plan_ma_msg_t *msg)
{
    // TODO: get ack or nack
}

static int solutionVerifyMarkFinalize(plan_ma_snapshot_t *s)
{
    // TODO: Check solution and reinsert state if needed
    return -1;
}

static void solutionVerifyResponseFinalize(plan_ma_snapshot_t *s)
{
    // TODO: Verify solution or not
}

