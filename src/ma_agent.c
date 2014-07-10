#include <boruvka/alloc.h>
#include "plan/ma_agent.h"

/** Reference data for the received public states */
struct _pub_state_data_t {
    int agent_id;             /*!< ID of the source agent */
    plan_cost_t cost;         /*!< Cost of the path to this state as found
                                   by the remote agent. */
    plan_state_id_t state_id; /*!< ID of the state in remote agent's state
                                   pool. This is used for back-tracking. */
};
typedef struct _pub_state_data_t pub_state_data_t;

/** Frees allocated agent path */
static void agentFreePath(plan_ma_agent_path_op_t *path, int path_size);
/** Send the given node to all peers as public state */
static int sendNode(plan_ma_agent_t *agent, plan_state_space_node_t *node);
/** Injects public state corresponding to the message to the search
 *  algorithm */
static void injectPublicState(plan_ma_agent_t *agent, plan_ma_msg_t *msg);
/** Sends TERMINATE_ACK to the arbiter */
static void sendTerminateAck(plan_ma_agent_t *agent);
/** Performs terminate operation. */
static void terminate(plan_ma_agent_t *agent);
/** Back tracks path from the specified state */
static int backTrackPath(plan_search_t *search, plan_state_id_t from_state,
                         plan_path_t *path);
/** Updates path in the given message with operators in the path */
static void updateTracedPath(plan_path_t *path, plan_ma_msg_t *msg);
/** Updates message with the traced path and return the agent where the
 *  message should be sent.
 *  If the message shouldn't be sent to any agent -1 is returned in case of
 *  error or -2 if tracing is done. */
static int updateTracePathMsg(plan_ma_agent_t *agent, plan_ma_msg_t *msg);
/** Reads path stored in message into internal storage */
static void readMsgPath(plan_ma_agent_t *agent, plan_ma_msg_t *msg);
/** Process trace-path type message and returns PLAN_SEARCH_* status */
static int processTracePath(plan_ma_agent_t *agent, plan_ma_msg_t *msg);
/** Process one message and returns PLAN_SEARCH_* status */
static int processMsg(plan_ma_agent_t *agent, plan_ma_msg_t *msg);
/** Performs trace-path operation, it is assumed that agent->search has
 *  found the solution. */
static void tracePath(plan_ma_agent_t *agent);


plan_ma_agent_t *planMAAgentNew(plan_problem_agent_t *prob,
                                plan_search_t *search,
                                plan_ma_comm_queue_t *comm)
{
    plan_ma_agent_t *agent;
    pub_state_data_t msg_init;

    if (search->state_pool != prob->prob.state_pool){
        fprintf(stderr, "Error: Search algorithm uses different state"
                        " pool/registry than the agent. This is not right!\n");
        return NULL;
    }

    agent = BOR_ALLOC(plan_ma_agent_t);
    agent->prob = prob;
    agent->search = search;
    agent->comm = comm;

    agent->state_pool = agent->prob->prob.state_pool;
    msg_init.agent_id = -1;
    msg_init.cost = PLAN_COST_MAX;
    agent->pub_state_reg_id = planStatePoolDataReserve(agent->state_pool,
                                                       sizeof(pub_state_data_t),
                                                       NULL, &msg_init);

    agent->packed_state_size = planStatePackerBufSize(agent->state_pool->packer);

    agent->path = NULL;
    agent->path_size = 0;
    agent->terminated = 0;

    return agent;
}

void planMAAgentDel(plan_ma_agent_t *agent)
{
    if (agent->path)
        agentFreePath(agent->path, agent->path_size);
    BOR_FREE(agent);
}

int planMAAgentRun(plan_ma_agent_t *agent)
{
    plan_search_t *search = agent->search;
    plan_search_step_change_t step_change;
    plan_ma_msg_t *msg;
    int res, i;

    planSearchStepChangeInit(&step_change);

    res = planSearchInitStep(search);
    while (res == PLAN_SEARCH_CONT){
        // Process all pending messages
        while ((msg = planMACommQueueRecv(agent->comm)) != NULL){
            res = processMsg(agent, msg);
        }

        // Again check the status because the message could change it
        if (res != PLAN_SEARCH_CONT)
            break;

        // Perform one step of algorithm.
        res = planSearchStep(search, &step_change);

        // If the solution was found, terminate agent cluster and exit.
        if (res == PLAN_SEARCH_FOUND){
            tracePath(agent);
            terminate(agent);
            break;
        }

        // Check all closed nodes and send the states created by public
        // operator to the peers.
        for (i = 0; i < step_change.closed_node_size; ++i){
            sendNode(agent, step_change.closed_node[i]);
        }

        // If this agent reached dead-end, wait either for terminate signal
        // or for some public state it can continue from.
        if (res == PLAN_SEARCH_NOT_FOUND){
            fprintf(stderr, "[%d] Going to block\n", agent->comm->node_id);
            if ((msg = planMACommQueueRecvBlock(agent->comm)) != NULL){
                res = processMsg(agent, msg);
            }
        }
    }

    planSearchStepChangeFree(&step_change);
    fprintf(stderr, "[%d] res: %d\n", agent->comm->node_id, res);
    return res;
}



static void agentFreePath(plan_ma_agent_path_op_t *path, int path_size)
{
    int i;

    for (i = 0; i < path_size; ++i){
        if (path[i].name)
            BOR_FREE(path[i].name);
    }
    BOR_FREE(path);
}

static int sendNode(plan_ma_agent_t *agent, plan_state_space_node_t *node)
{
    plan_ma_msg_t *msg;
    const void *statebuf;
    int res;

    // We are interested only in the states created by public operator
    if (!node->op || planOperatorIsPrivate(node->op))
        return -2;

    statebuf = planStatePoolGetPackedState(agent->state_pool, node->state_id);
    if (statebuf == NULL)
        return -1;

    msg = planMAMsgNew();
    planMAMsgSetPublicState(msg, agent->prob->id,
                            statebuf, agent->packed_state_size,
                            node->state_id,
                            node->cost, node->heuristic);
    res = planMACommQueueSendToAll(agent->comm, msg);
    planMAMsgDel(msg);

    return res;
}

static void injectPublicState(plan_ma_agent_t *agent, plan_ma_msg_t *msg)
{
    int cost, heuristic;
    pub_state_data_t *pub_state_data;
    plan_state_id_t state_id;
    const void *packed_state;

    // Unroll data from the message
    packed_state = planMAMsgPublicStateStateBuf(msg);
    cost         = planMAMsgPublicStateCost(msg);
    heuristic    = planMAMsgPublicStateHeur(msg);

    // Insert packed state into state-pool if not already inserted
    state_id = planStatePoolInsertPacked(agent->state_pool,
                                         packed_state);

    // Get public state reference data
    pub_state_data = planStatePoolData(agent->state_pool,
                                       agent->pub_state_reg_id,
                                       state_id);

    // This state was already inserted in past, so set the reference
    // data only if the cost is smaller
    // Set the reference data only if the new cost is smaller than the
    // current one. This means that either the state is brand new or the
    // previously inserted state had bigger cost.
    if (pub_state_data->cost > cost){
        pub_state_data->agent_id = planMAMsgPublicStateAgent(msg);
        pub_state_data->cost     = cost;
        pub_state_data->state_id = planMAMsgPublicStateStateId(msg);
    }

    // Inject state into search algorithm
    planSearchInjectState(agent->search, state_id, cost, heuristic);
}

static void sendTerminateAck(plan_ma_agent_t *agent)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew();
    planMAMsgSetTerminateAck(msg);
    planMACommQueueSendToArbiter(agent->comm, msg);
    planMAMsgDel(msg);
}

static void terminate(plan_ma_agent_t *agent)
{
    plan_ma_msg_t *msg;
    int count, ack;

    if (agent->terminated)
        return;

    if (agent->comm->arbiter){
        // If this is arbiter just send TERMINATE signal and wait for ACK
        // from all peers.
        // Because the TERMINATE_ACK is sent only as a response to TERMINATE
        // signal and because TERMINATE signal can send only the single
        // arbiter from exactly this place it is enough just to count
        // number of ACKs.

        msg = planMAMsgNew();
        planMAMsgSetTerminate(msg);
        planMACommQueueSendToAll(agent->comm, msg);
        planMAMsgDel(msg);

        count = planMACommQueueNumPeers(agent->comm);
        while (count > 0
                && (msg = planMACommQueueRecvBlock(agent->comm)) != NULL){

            if (planMAMsgIsTerminateAck(msg))
                --count;
            planMAMsgDel(msg);
        }
    }else{
        // If this node is not arbiter send TERMINATE_REQUEST, wait for
        // TERMINATE signal, ACK it and then termination is finished.

        // 1. Send TERMINATE_REQUEST
        msg = planMAMsgNew();
        planMAMsgSetTerminateRequest(msg);
        planMACommQueueSendToArbiter(agent->comm, msg);
        planMAMsgDel(msg);

        // 2. Wait for TERMINATE
        ack = 0;
        while (!ack && (msg = planMACommQueueRecvBlock(agent->comm)) != NULL){
            if (planMAMsgIsTerminate(msg)){
                // 3. Send TERMINATE_ACK
                sendTerminateAck(agent);
                ack = 1;
            }
            planMAMsgDel(msg);
        }
    }

    agent->terminated = 1;
}

static int backTrackPath(plan_search_t *search, plan_state_id_t from_state,
                         plan_path_t *path)
{
    planSearchBackTrackPathFrom(search, from_state, path);
    if (planPathEmpty(path)){
        fprintf(stderr, "Error: Could not trace any portion of the"
                        " path.\n");
        return -1;
    }

    return 0;
}

static void updateTracedPath(plan_path_t *path, plan_ma_msg_t *msg)
{
    plan_path_op_t *op;
    bor_list_t *lst;

    for (lst = borListPrev(path); lst != path; lst = borListPrev(lst)){
        op = BOR_LIST_ENTRY(lst, plan_path_op_t, path);
        planMAMsgTracePathAddOperator(msg, op->op->name, op->op->cost);
    }
}

/** Updates message with the traced path and return the agent where the
 *  message should be sent.
 *  If the message shouldn't be sent to any agent -1 is returned in case of
 *  error or -2 if tracing is done. */
static int updateTracePathMsg(plan_ma_agent_t *agent, plan_ma_msg_t *msg)
{
    plan_path_t path;
    plan_state_id_t from_state;
    plan_path_op_t *first_op;
    pub_state_data_t *pub_state;
    int target_agent_id;

    fprintf(stderr, "[%d]: RECV TRACE_PATH\n", agent->comm->node_id);

    // Early exit if the path was already traced
    if (planMAMsgTracePathIsDone(msg))
        return -2;

    fprintf(stderr, "[%d]: NOT DONE\n", agent->comm->node_id);

    // Get state from which start back-tracking
    from_state = planMAMsgTracePathStateId(msg);

    // Back-track path and handle possible error
    planPathInit(&path);
    if (backTrackPath(agent->search, from_state, &path) != 0){
        return -1;
    }

    // Update trace path message by adding all operators in reverse
    // order
    updateTracedPath(&path, msg);

    // Check if we don't have the full path already
    first_op = planPathFirstOp(&path);
    if (first_op->from_state == 0){
        // If traced the path to the initial state -- set the traced
        // path as done.
        planMAMsgTracePathSetDone(msg);
        target_agent_id = -2;

    }else{
        // Retrieve public-state related data for the first state in path
        pub_state = planStatePoolData(agent->state_pool,
                                      agent->pub_state_reg_id,
                                      first_op->from_state);
        if (pub_state->agent_id == -1){
            fprintf(stderr, "Error: Trace-back of the path end up in"
                            " non-public state which also isn't the"
                            " initial state!\n");
            return -1;
        }

        planMAMsgTracePathSetStateId(msg, pub_state->state_id);
        target_agent_id = pub_state->agent_id;
    }

    planPathFree(&path);

    return target_agent_id;
}

static void readMsgPath(plan_ma_agent_t *agent, plan_ma_msg_t *msg)
{
    int i, len, cost;
    const char *name;

    // get number of operators in path
    len = planMAMsgTracePathNumOperators(msg);

    // allocate memory for the path
    if (agent->path)
        agentFreePath(agent->path, agent->path_size);
    agent->path_size = len;
    agent->path = BOR_ALLOC_ARR(plan_ma_agent_path_op_t, agent->path_size);

    // copy path to the internal structure
    for (i = len - 1; i >= 0; --i){
        name = planMAMsgTracePathOperator(msg, i, &cost);
        agent->path[i].name = strdup(name);
        agent->path[i].cost = cost;
    }
}

static int processTracePath(plan_ma_agent_t *agent, plan_ma_msg_t *msg)
{
    int res, origin_agent;

    res = updateTracePathMsg(agent, msg);
    if (res >= 0){
        planMACommQueueSendToNode(agent->comm, res, msg);
        return PLAN_SEARCH_CONT;

    }else if (res == -1){
        return PLAN_SEARCH_ABORT;

    }else if (res == -2){
        origin_agent = planMAMsgTracePathOriginAgent(msg);

        if (origin_agent != agent->comm->node_id){
            // If this is not the original agent sent the result to the
            // original agent
            planMACommQueueSendToNode(agent->comm, origin_agent, msg);
            return PLAN_SEARCH_CONT;

        }else{
            // If we have received the full traced path which we
            // originated, read the path to the internal structure and
            // report found solution.
            readMsgPath(agent, msg);
            return PLAN_SEARCH_FOUND;
        }
    }

    return PLAN_SEARCH_ABORT;
}

static int processMsg(plan_ma_agent_t *agent, plan_ma_msg_t *msg)
{
    int res = PLAN_SEARCH_CONT;

    if (planMAMsgIsPublicState(msg)){
        injectPublicState(agent, msg);
        res = PLAN_SEARCH_CONT;

    }else if (planMAMsgIsTerminateType(msg)){
        if (agent->comm->arbiter){
            // The arbiter should ignore all signals except
            // TERMINATE_REQUEST because TERMINATE is allowed to send
            // only arbiter itself and TERMINATE_ACK should be received
            // in terminate() method.
            if (planMAMsgIsTerminateRequest(msg)){
                terminate(agent);
                res = PLAN_SEARCH_ABORT;
            }

        }else{
            // The non-arbiter node should accept only TERMINATE
            // signal and send ACK to him.
            if (planMAMsgIsTerminate(msg)){
                sendTerminateAck(agent);
                agent->terminated = 1;
                res = PLAN_SEARCH_ABORT;
            }
        }

    }else if (planMAMsgIsTracePath(msg)){
        res = processTracePath(agent, msg);
    }

    planMAMsgDel(msg);

    return res;
}

static void tracePath(plan_ma_agent_t *agent)
{
    plan_ma_msg_t *msg;
    int res;

    fprintf(stderr, "[%d] Trace path\n", agent->comm->node_id);
    // Construct trace-path message and fake it as it were we received this
    // message and we should process it
    msg = planMAMsgNew();
    planMAMsgSetTracePath(msg, agent->comm->node_id);
    planMAMsgTracePathSetStateId(msg, agent->search->goal_state);

    // Process that message as if it were just received
    res = processTracePath(agent, msg);
    planMAMsgDel(msg);

    // We have found solution, so exit early
    if (res != PLAN_SEARCH_CONT)
        return;

    // Wait for trace-path response or terminate signal
    while ((msg = planMACommQueueRecvBlock(agent->comm)) != NULL){
        fprintf(stderr, "[%d] got msg %d\n", agent->comm->node_id,
                planMAMsgType(msg));
        if (planMAMsgIsTracePath(msg) || planMAMsgIsTerminateType(msg)){
            res = processMsg(agent, msg);
            fprintf(stderr, "[%d] msg-res: %d\n", agent->comm->node_id, res);
            if (res != PLAN_SEARCH_CONT)
                break;
        }else{
            planMAMsgDel(msg);
        }
    }
}

