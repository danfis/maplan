#ifndef __PLAN_MA_SEARCH_COMMON_H__
#define __PLAN_MA_SEARCH_COMMON_H__

#include <plan/ma_comm.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Initializes termination of a whole agent cluster.
 */
void planMASearchTerminate(plan_ma_comm_t *comm);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_MA_SEARCH_COMMON_H__ */
