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
            }

            planMAMsgDel(msg);
        }

        // perform one step of algorithm
        res = planSearchStep(search, &step_change);

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

