#include "ma_search_th.h"
#include "ma_search_common.h"
#include "ma_search_trace_path.h"

/** Reference data for the received public states */
struct _pub_state_data_t {
    int agent_id;             /*!< ID of the source agent */
    plan_cost_t cost;         /*!< Cost of the path to this state as found
                                   by the remote agent. */
    plan_state_id_t state_id; /*!< ID of the state in remote agent's state
                                   pool. This is used for back-tracking. */
};
typedef struct _pub_state_data_t pub_state_data_t;

static void *searchThread(void *data);
static int searchThreadPostStep(plan_search_t *search, int res, void *ud);
static void searchThreadExpandedNode(plan_search_t *search,
                                     plan_state_space_node_t *node, void *ud);
static void processMsg(plan_ma_search_th_t *th, plan_ma_msg_t *msg);
static void publicStateSend(plan_ma_search_th_t *th,
                            plan_state_space_node_t *node);
static void publicStateRecv(plan_ma_search_th_t *th,
                            plan_ma_msg_t *msg);

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

void planMASearchThInit(plan_ma_search_th_t *th,
                        plan_search_t *search,
                        plan_ma_comm_t *comm,
                        plan_path_t *path)
{
    pub_state_data_t msg_init;

    th->search = search;
    th->comm = comm;
    th->path = path;

    msg_init.agent_id = -1;
    msg_init.cost = PLAN_COST_MAX;
    msg_init.state_id = PLAN_NO_STATE;
    th->pub_state_reg = planStatePoolDataReserve(search->state_pool,
                                                 sizeof(pub_state_data_t),
                                                 NULL, &msg_init);
    borFifoSemInit(&th->msg_queue, sizeof(plan_ma_msg_t *));
    th->accept_public_state = 1;

    th->res = -1;
}

void planMASearchThFree(plan_ma_search_th_t *th)
{
    plan_ma_msg_t *msg;

    // Empty queue
    while (borFifoSemPop(&th->msg_queue, &msg) == 0){
        planMAMsgDel(msg);
    }
    borFifoSemFree(&th->msg_queue);
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
    fprintf(stderr, "Th[%d] START %d %d\n", th->comm->node_id,
            th->res, planPathEmpty(th->path));
    planSearchRun(th->search, th->path);
    fprintf(stderr, "Th[%d] END %d %d\n", th->comm->node_id,
            th->res, planPathEmpty(th->path));
    fflush(stderr);

    return NULL;
}

static int searchThreadPostStep(plan_search_t *search, int res, void *ud)
{
    plan_ma_search_th_t *th = ud;
    plan_ma_msg_t *msg = NULL;
    int type = -1;
    int trace_path;

    fprintf(stderr, "Th[%d] PostStep: %d\n", th->comm->node_id, res);
    fflush(stderr);
    if (res == PLAN_SEARCH_FOUND){
        fprintf(stderr, "Th[%d] FOUND %d\n", th->comm->node_id,
                th->search->goal_state);
        fflush(stderr);
        // TODO: Verify solution

        // TODO: Trace path
        trace_path = tracePathInit(th->search, th->search->goal_state,
                                   th->pub_state_reg, th->comm, th->path);
        th->accept_public_state = 0;
        if (trace_path == -1){
            planMASearchTerminate(th->comm);
            th->res = PLAN_SEARCH_ABORT;

        }else if (trace_path == 0){
            planMASearchTerminate(th->comm);
            th->res = PLAN_SEARCH_FOUND;
        }

        res = PLAN_SEARCH_CONT;

    }else if (res == PLAN_SEARCH_ABORT){
        fprintf(stderr, "Th[%d] ABORT\n", th->comm->node_id);
        fflush(stderr);
        // Initialize termination of a whole cluster
        planMASearchTerminate(th->comm);

        // Ignore all messages except terminate (which is empty)
        do {
            fprintf(stderr, "Th[%d] Block\n", th->comm->node_id);
            fflush(stderr);
            borFifoSemPopBlock(&th->msg_queue, &msg);
            fprintf(stderr, "Th[%d] Unblock\n", th->comm->node_id);
            fflush(stderr);
            if (msg)
                planMAMsgDel(msg);
        } while (msg);

    }else if (res == PLAN_SEARCH_NOT_FOUND){
        fprintf(stderr, "Th[%d] NOT FOUND\n", th->comm->node_id);
        fflush(stderr);

        // Block until public-state or terminate isn't received
        do {
            fprintf(stderr, "Th[%d] Block\n", th->comm->node_id);
            fflush(stderr);
            borFifoSemPopBlock(&th->msg_queue, &msg);
            fprintf(stderr, "Th[%d] Unblock\n", th->comm->node_id);
            fflush(stderr);

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

    fprintf(stderr, "Th[%d] res: %d\n", th->comm->node_id, res);
    fflush(stderr);
    return res;
}

static void searchThreadExpandedNode(plan_search_t *search,
                                     plan_state_space_node_t *node, void *ud)
{
    plan_ma_search_th_t *th = ud;
    publicStateSend(th, node);
}

static void processMsg(plan_ma_search_th_t *th, plan_ma_msg_t *msg)
{
    int type; 
    int res;

    type = planMAMsgType(msg);
    fprintf(stderr, "Th[%d] msg: %d\n", th->comm->node_id, type);
    if (type == PLAN_MA_MSG_PUBLIC_STATE
            && th->accept_public_state){
        publicStateRecv(th, msg);

    }else if (type == PLAN_MA_MSG_TRACE_PATH){
        th->accept_public_state = 0;

        fprintf(stderr, "Th[%d] TRACE PATH\n", th->comm->node_id);
        res = tracePathProcessMsg(msg, th->search, th->pub_state_reg,
                                  th->comm, th->path);
        fprintf(stderr, "Th[%d] TRACE res: %d\n", th->comm->node_id, res);
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
        fprintf(stderr, "Th[%d] State pub state %d to %d\n",
                th->comm->node_id, node->state_id, peers[i]);
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

    /*TODO
    // Skip nodes that are worse than the best goal state so far
    if (search->ma_ack_solution && cost > search->best_goal_cost)
        return -1;
    */

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
        fprintf(stderr, "Th[%d] INS %d from %d as %d\n",
                th->comm->node_id, planMAMsgPublicStateStateId(msg),
                planMAMsgAgent(msg), node->state_id);
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
        fprintf(stderr, "Th[%d] SET %d from %d as %d\n",
                th->comm->node_id, planMAMsgPublicStateStateId(msg),
                planMAMsgAgent(msg), node->state_id);
        pub_state->agent_id = planMAMsgAgent(msg);
        pub_state->cost     = cost;
        pub_state->state_id = planMAMsgPublicStateStateId(msg);
    }
}


static int tracePathInit(const plan_search_t *search,
                         plan_state_id_t goal_state,
                         int state_pool_pub_reg,
                         plan_ma_comm_t *comm,
                         plan_path_t *path)
{
    plan_state_id_t init_state;
    plan_ma_msg_t *msg;
    const pub_state_data_t *pub_state;

    init_state = planSearchExtractPath(search, goal_state, path);
    if (init_state == 0)
        return 0;
    if (init_state == PLAN_NO_STATE){
        planPathFree(path);
        return -1;
    }

    pub_state = planStatePoolData(search->state_pool, state_pool_pub_reg,
                                  init_state);
    if (pub_state->agent_id == -1){
        planPathFree(path);
        return -1;
    }

    msg = planMAMsgNew(PLAN_MA_MSG_TRACE_PATH, 0, comm->node_id);
    planMAMsgTracePathAddPath(msg, path);
    planMAMsgTracePathSetStateId(msg, pub_state->state_id);
    planMACommSendToNode(comm, pub_state->agent_id, msg);
    planMAMsgDel(msg);
    fprintf(stderr, "Th[%d] INIT TRACE PATH (-> %d)\n",
            comm->node_id, pub_state->agent_id);

    planPathFree(path);
    return 1;
}

static int tracePathProcessMsg(plan_ma_msg_t *msg,
                               const plan_search_t *search,
                               int state_pool_pub_reg,
                               plan_ma_comm_t *comm,
                               plan_path_t *path)
{
    plan_state_id_t state_id;
    const pub_state_data_t *pub_state;
    int init_agent;

    // Get state id from which to trace path. If this agent was the
    // initiator (thus state_id == -1) return extracted path.
    state_id = planMAMsgTracePathStateId(msg);
    fprintf(stderr, "Th[%d] trace-path, state-id: %d\n",
            comm->node_id, state_id);
    if (state_id == -1){
        planMAMsgTracePathExtractPath(msg, path);
        return 0;
    }

    // Trace next part of the path
    state_id = planSearchExtractPath(search, state_id, path);
    fprintf(stderr, "Th[%d] Ex trace-path, state-id: %d\n",
            comm->node_id, state_id);
    if (state_id == PLAN_NO_STATE){
        planPathFree(path);
        return -1;
    }

    // Add the partial path to the message
    planMAMsgTracePathAddPath(msg, path);

    // If the path was traced to the initial state check whether this agent
    // is the initiator.
    if (state_id == search->initial_state){
        init_agent = planMAMsgTracePathInitAgent(msg);

        if (init_agent == comm->node_id){
            // If the current agent is the initiator, extract path and
            // return zero
            planPathFree(path);
            planMAMsgTracePathExtractPath(msg, path);
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
        planPathFree(path);
        return -1;
    }

    // Send the message to the owner of the public state
    fprintf(stderr, "Th[%d] TRACE Send to %d\n", comm->node_id,
            pub_state->agent_id);
    planMAMsgTracePathSetStateId(msg, pub_state->state_id);
    planMACommSendToNode(comm, pub_state->agent_id, msg);
    planPathFree(path);
    return 1;
}

