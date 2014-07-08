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

    return agent;
}



void planMAAgentDel(plan_ma_agent_t *agent)
{
    BOR_FREE(agent);
}


static void nodeClosed(plan_state_space_node_t *node, void *_agent)
{
    plan_ma_agent_t *agent = _agent;
    plan_ma_msg_t *msg;
    const void *statebuf;
    size_t state_size;
    int res;

    // we are interested only into public operators
    if (!node->op || planOperatorIsPrivate(node->op))
        return;

    statebuf = planStatePoolGetPackedState(agent->prob->prob.state_pool,
                                           node->state_id);
    if (statebuf == NULL)
        return;
    // TODO
    state_size = planStatePackerBufSize(agent->prob->prob.state_pool->packer);

    msg = planMAMsgNew();
    planMAMsgSetPublicState(msg, agent->prob->name,
                            statebuf, state_size,
                            node->cost, node->heuristic);
    res = planMACommQueueSendToAll(agent->comm, msg);
    fprintf(stderr, "[%d] Send: %d\n", agent->prob->id, res);
    planMAMsgDel(msg);

    fprintf(stderr, "[%d] Closed: %d\n", agent->prob->id, planStateSpaceNodeIsClosed(node));
}

int planMAAgentRun(plan_ma_agent_t *agent)
{
    plan_search_t *search = agent->search;
    plan_search_run_cb_t orig_run_cb = planSearchGetRunCB(search);
    plan_search_run_cb_t run_cb;
    plan_ma_msg_t *msg;
    int res;

    planSearchRunCBInit(&run_cb);
    run_cb.node_closed = nodeClosed;
    run_cb.data = agent;
    planSearchSetRunCB(search, &run_cb);

    res = search->init_fn(search);
    while (res == PLAN_SEARCH_CONT){
        while ((msg = planMACommQueueRecv(agent->comm)) != NULL){
            fprintf(stderr, "[%d] RECV: %d\n", agent->prob->id,
                    planMAMsgIsPublicState(msg));
            planMAMsgDel(msg);
        }

        res = search->step_fn(search);
    }

    planSearchSetRunCB(search, &orig_run_cb);

    fprintf(stderr, "res: %d\n", res);
    return res;
}

