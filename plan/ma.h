#ifndef __PLAN_MA_H__
#define __PLAN_MA_H__

#include <plan/ma_agent.h>

int planMARun(int agent_size, plan_search_t **search,
              plan_ma_agent_path_op_t **path, int *path_size);

#endif /* __PLAN_MA_H__ */
