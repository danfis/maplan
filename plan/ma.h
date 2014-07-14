#ifndef __PLAN_MA_H__
#define __PLAN_MA_H__

#include <plan/ma_agent.h>

/**
 * Runs multiagent search.
 * Returns one of PLAN_SEARCH_* statuses and if a path was found the path
 * is returned via output arguments.
 */
int planMARun(int agent_size, plan_search_t **search,
              plan_ma_agent_path_op_t **path, int *path_size);

#endif /* __PLAN_MA_H__ */
