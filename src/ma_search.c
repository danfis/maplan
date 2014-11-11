#include <boruvka/alloc.h>

#include "plan/ma_search.h"

#include "ma_search_th.h"
#include "ma_search_common.h"

/** Maximal time (in ms) for which is multi-agent thread blocked when
 *  dead-end is reached. This constant is here basicaly to prevent
 *  dead-lock when all threads are in dead-end. So, it should be set to
 *  fairly high number. */
#define SEARCH_MA_MAX_BLOCK_TIME (600 * 1000) // 10 minutes

/** Process one message. Returns 0 if process should continue, -1 otherwise */
static int processMsg(plan_ma_search_t *ma_search,
                      plan_ma_msg_t *msg,
                      bor_fifo_sem_t *th_queue);
/** Process TERMINATE message */
static int msgTerminate(plan_ma_search_t *ma_search,
                        plan_ma_msg_t *msg);

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
    plan_ma_msg_t *msg = NULL;
    int res, cont = 1;

    ma_search->terminated = 0;
    ma_search->termination = 0;

    // Start search algorithm
    planMASearchThInit(&th, ma_search->search, ma_search->comm, path);
    planMASearchThRun(&th);

    // The main thread is here for processing messages
    while (cont){
        // Wait for the next message
        msg = planMACommRecvBlock(ma_search->comm, SEARCH_MA_MAX_BLOCK_TIME);
        if (msg == NULL){
            // Blocked queue timeouted -- terminate cluster
            planMASearchTerminate(ma_search->comm);

        }else{
            if (processMsg(ma_search, msg, &th.msg_queue) != 0)
                cont = 0;
        }
    }

    // Wait for search algorithm to end
    planMASearchThJoin(&th);
    res = th.res;

    // Free resources
    planMASearchThFree(&th);

    return res;
}

static int processMsg(plan_ma_search_t *ma_search,
                      plan_ma_msg_t *msg,
                      bor_fifo_sem_t *th_queue)
{
    int type = planMAMsgType(msg);
    int res = 0;

    fprintf(stderr, "[%d] msg: %d\n", ma_search->comm->node_id, type);
    fflush(stderr);
    if (type == PLAN_MA_MSG_TERMINATE){
        res = msgTerminate(ma_search, msg);

    }else if (type == PLAN_MA_MSG_PUBLIC_STATE){
        // Forward message to search thread
        borFifoSemPush(th_queue, &msg);
        msg = NULL;
    }

    if (msg)
        planMAMsgDel(msg);
    return res;
}

static int msgTerminate(plan_ma_search_t *ma_search,
                        plan_ma_msg_t *msg)
{
    int subtype = planMAMsgSubType(msg);
    int agent_id;
    plan_ma_msg_t *term_msg;

    fprintf(stderr, "[%d] terminate msg: %d\n",
            ma_search->comm->node_id, subtype);
    fflush(stderr);
    if (subtype == PLAN_MA_MSG_TERMINATE_FINAL){
        // When TERMINATE signal is received, just terminate
        return -1;

    }else{ // PLAN_MA_MSG_TERMINATE_REQUEST
        ma_search->termination = 1;

        // If the TERMINATE_REQUEST is received it can be either the
        // original TERMINATE_REQUEST from this agent or it can be
        // request from some other agent that want to terminate in the
        // same time.
        agent_id = planMAMsgTerminateAgent(msg);
        if (agent_id == ma_search->comm->node_id){
            // If our terminate-request circled in the whole ring and
            // came back, we can safely send TERMINATE signal to all
            // other agents and terminate.
            term_msg = planMAMsgNew(PLAN_MA_MSG_TERMINATE,
                                    PLAN_MA_MSG_TERMINATE_FINAL,
                                    ma_search->comm->node_id);
            planMACommSendToAll(ma_search->comm, term_msg);
            planMAMsgDel(term_msg);
            return -1;

        }else{
            // The request is from some other agent, just is send it to
            // the next agent in ring
            planMACommSendInRing(ma_search->comm, msg);
        }
    }

    return 0;
}
