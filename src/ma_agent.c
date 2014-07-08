#include <boruvka/alloc.h>
#include "plan/ma_agent.h"

plan_ma_agent_t *planMAAgentNew(plan_problem_agent_t *prob,
                                plan_search_t *search,
                                plan_ma_comm_queue_t *comm)
{
    plan_ma_agent_t *agent;

    agent = BOR_ALLOC(plan_ma_agent_t);
    agent->prob = prob;
    agent->search = search;
    agent->comm = comm;
    agent->packed_state_size = planSearchPackedStateSize(search);
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

    statebuf = planSearchPackedState(agent->search, node->state_id);
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

    planMAMsgGetPublicState(msg, &agent_id,
                            agent->packed_state,
                            agent->packed_state_size,
                            &cost, &heuristic);

    planSearchInjectState(agent->search, agent->packed_state,
                          cost, heuristic);
    // TODO: Insert message into register associated with state_id
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
    int count;

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
        while ((msg = planMACommQueueRecvBlock(agent->comm)) != NULL){
            if (planMAMsgIsTerminate(msg)){
                // 3. Send TERMINATE_ACK
                sendTerminateAck(agent);
            }
            planMAMsgDel(msg);
        }
    }
}

int planMAAgentRun(plan_ma_agent_t *agent)
{
    plan_search_t *search = agent->search;
    plan_search_step_change_t step_change;
    plan_state_space_node_t *node;
    plan_ma_msg_t *msg;
    int res, i;

    planSearchStepChangeInit(&step_change);

    res = planSearchInitStep(search);
    while (res == PLAN_SEARCH_CONT){
        while ((msg = planMACommQueueRecv(agent->comm)) != NULL){
            if (planMAMsgIsPublicState(msg)){
                injectPublicState(agent, msg);

            }else if (planMAMsgIsTerminateType(msg)){
                if (agent->comm->arbiter){
                    // The arbiter should ignore all signals except
                    // TERMINATE_REQUEST because TERMINATE is allowed to send
                    // only arbiter itself and TERMINATE_ACK should be received
                    // in terminate() method.
                    if (planMAMsgIsTerminateRequest(msg)){
                        terminate(agent);
                        res = PLAN_SEARCH_NOT_FOUND;
                    }

                }else{
                    // The non-arbiter node should accept only TERMINATE
                    // signal, send ACK to him and pretend to the caller that
                    // this node is about to terminate.
                    if (planMAMsgIsTerminate(msg)){
                        sendTerminateAck(agent);
                        res = PLAN_SEARCH_NOT_FOUND;
                    }
                }
            }

            planMAMsgDel(msg);
        }

        if (res != PLAN_SEARCH_CONT)
            break;

        // perform one step of algorithm
        res = planSearchStep(search, &step_change);
        if (res == PLAN_SEARCH_FOUND){
            terminate(agent);
            break;
        }

        // Check all closed nodes and send the states created by public
        // operator to the peers.
        for (i = 0; i < step_change.closed_node_size; ++i){
            node = step_change.closed_node[i];
            if (node->op && !planOperatorIsPrivate(node->op)){
                sendNode(agent, node);
            }
        }
    }

    planSearchStepChangeFree(&step_change);
    fprintf(stderr, "res: %d\n", res);
    return res;
}

