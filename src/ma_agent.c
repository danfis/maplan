#include <boruvka/alloc.h>
#include "plan/ma_agent.h"

struct _pub_state_data_t {
    int agent_id;
    plan_cost_t cost;
};
typedef struct _pub_state_data_t pub_state_data_t;

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
    agent->packed_state = BOR_ALLOC_ARR(char, agent->packed_state_size);

    return agent;
}

void planMAAgentDel(plan_ma_agent_t *agent)
{
    if (agent->packed_state)
        BOR_FREE(agent->packed_state);
    BOR_FREE(agent);
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
                            node->cost, node->heuristic);
    res = planMACommQueueSendToAll(agent->comm, msg);
    planMAMsgDel(msg);

    return res;
}

static void injectPublicState(plan_ma_agent_t *agent,
                              plan_ma_msg_t *msg)
{
    int agent_id;
    int cost, heuristic;
    pub_state_data_t *pub_state_data;
    plan_state_id_t state_id;

    // Unroll data from the message
    planMAMsgGetPublicState(msg, &agent_id,
                            agent->packed_state,
                            agent->packed_state_size,
                            &cost, &heuristic);

    // Insert packed state into state-pool if not already inserted
    state_id = planStatePoolInsertPacked(agent->state_pool,
                                         agent->packed_state);

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
        pub_state_data->agent_id = agent_id;
        pub_state_data->cost     = cost;
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
            }

        }else{
            // The non-arbiter node should accept only TERMINATE
            // signal, send ACK to him and pretend to the caller that
            // this node is about to terminate.
            if (planMAMsgIsTerminate(msg)){
                sendTerminateAck(agent);
            }
        }

        res = PLAN_SEARCH_ABORT;
    }

    planMAMsgDel(msg);

    return res;
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

