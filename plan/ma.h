#ifndef __PLAN_MA_H__
#define __PLAN_MA_H__

#include <plan/search.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Runs multiagent search.
 * Returns one of PLAN_SEARCH_* statuses and if a path was found the path
 * is returned via output argument.
 */
int planMARun(int agent_size, plan_search_t **search, plan_path_t *path);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_H__ */
