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


int planMAAgentRun(plan_ma_agent_t *agent)
{
    plan_search_t *search = agent->search;
    int res;

    res = search->init_fn(search);
    while (res == PLAN_SEARCH_CONT){
        res = search->step_fn(search);
    }

    fprintf(stderr, "res: %d\n", res);
    return res;
}

