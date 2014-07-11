#ifndef __PLAN_MA_H__
#define __PLAN_MA_H__

#include <plan/ma_agent.h>

int planMARun(int agent_size, plan_problem_agent_t *agent_prob,
              plan_search_t **search, plan_path_t *path);

#endif /* __PLAN_MA_H__ */
