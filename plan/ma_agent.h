#ifndef __PLAN_MA_AGENT_H__
#define __PLAN_MA_AGENT_H__

#include <plan/search.h>
#include <plan/ma_comm_queue.h>

struct _plan_ma_agent_t {
    plan_problem_agent_t *prob;
    plan_search_t *search;
    plan_ma_comm_queue_t *comm;
    pthread_t thread;
};
typedef struct _plan_ma_agent_t plan_ma_agent_t;

plan_ma_agent_t *planMAAgentNew(plan_problem_agent_t *prob,
                                plan_search_t *search,
                                plan_ma_comm_queue_t *comm);

void planMAAgentDel(plan_ma_agent_t *agent);

int planMAAgentRun(plan_ma_agent_t *agent);

#endif /* __PLAN_MA_AGENT_H__ */
