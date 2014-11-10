#include <boruvka/alloc.h>

#include "plan/ma_search.h"

#include "ma_search_msg_handler.h"
#include "ma_search_th.h"

/** Maximal time (in ms) for which is multi-agent thread blocked when
 *  dead-end is reached. This constant is here basicaly to prevent
 *  dead-lock when all threads are in dead-end. So, it should be set to
 *  fairly high number. */
#define SEARCH_MA_MAX_BLOCK_TIME (600 * 1000) // 10 minutes

void planMASearchParamsInit(plan_ma_search_params_t *params)
{
    bzero(params, sizeof(*params));
}

plan_ma_search_t *planMASearchNew(plan_ma_search_params_t *params)
{
    plan_ma_search_t *ma_search;

    ma_search = BOR_ALLOC(plan_ma_search_t);
    ma_search->search = params->search;
    ma_search->comm = params->comm;
    ma_search->terminated = 0;
    return ma_search;
}

void planMASearchDel(plan_ma_search_t *ma_search)
{
    BOR_FREE(ma_search);
}

int planMASearchRun(plan_ma_search_t *ma_search, plan_path_t *path)
{
    plan_ma_search_th_t th;
    plan_ma_search_msg_handler_t handler;
    plan_ma_msg_t *msg = NULL;
    int res;

    ma_search->terminated = 0;

    // Initialize message handler
    planMASearchMsgHandlerInit(&handler, ma_search->comm);

    // Start search algorithm
    planMASearchThInit(&th, ma_search->search, ma_search->comm, path);
    planMASearchThRun(&th);

    // The main thread is here for processing messages
    while (1){
        // Wait for the next message
        msg = planMACommRecvBlock(ma_search->comm, SEARCH_MA_MAX_BLOCK_TIME);
        if (msg == NULL){
            // TODO
            break;

        }else{
            if (planMASearchMsgHandler(&handler, msg) != 0)
                break;
        }

        planMAMsgDel(msg);
    }

    if (msg)
        planMAMsgDel(msg);

    // Wait for search algorithm to end
    planMASearchThJoin(&th);
    res = th.res;

    // Free resources
    planMASearchThFree(&th);

    planMASearchMsgHandlerFree(&handler);
    return res;
}
