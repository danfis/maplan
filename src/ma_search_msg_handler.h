#ifndef __PLAN_MA_SEARCH_MSG_HANDLER_H__
#define __PLAN_MA_SEARCH_MSG_HANDLER_H__

#include <plan/ma_comm.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_ma_search_msg_handler_t {
    plan_ma_comm_t *comm;

    int termination; /*!< True if termination is in progress */
};
typedef struct _plan_ma_search_msg_handler_t plan_ma_search_msg_handler_t;

/**
 * Initializes msg handler.
 */
void planMASearchMsgHandlerInit(plan_ma_search_msg_handler_t *h,
                                plan_ma_comm_t *comm);

/**
 * Frees msg handler.
 */
void planMASearchMsgHandlerFree(plan_ma_search_msg_handler_t *h);

/**
 * Process one message.
 * Returns 0 if caller should proceed or should terminate.
 */
int planMASearchMsgHandler(plan_ma_search_msg_handler_t *h,
                           plan_ma_msg_t *msg);


/**
 * Initialize termination of all agents.
 */
void planMASearchTerminate(plan_ma_comm_t *comm);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_MA_SEARCH_MSG_HANDLER_H__ */
