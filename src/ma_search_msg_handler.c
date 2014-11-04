#include <stdio.h>
#include "ma_search_msg_handler.h"


void planMASearchTerminate(plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;

    fprintf(stderr, "[%d] TERMINATE\n", comm->node_id);
    fflush(stderr);

    msg = planMAMsgNew(PLAN_MA_MSG_TERMINATE,
                       PLAN_MA_MSG_TERMINATE_REQUEST,
                       comm->node_id);
    planMAMsgTerminateSetAgent(msg, comm->node_id);
    planMACommSendToAll(comm, msg);
    planMAMsgDel(msg);
}

static int terminateHandler(plan_ma_search_msg_handler_t *h,
                            plan_ma_msg_t *msg)
{
    int subtype = planMAMsgSubType(msg);
    int agent_id;
    plan_ma_msg_t *term_msg;

    if (subtype == PLAN_MA_MSG_TERMINATE_FINAL){
        // When TERMINATE signal is received, just terminate
        return -1;

    }else{ // PLAN_MA_MSG_TERMINATE_REQUEST
        h->termination = 1;

        // If the TERMINATE_REQUEST is received it can be either the
        // original TERMINATE_REQUEST from this agent or it can be
        // request from some other agent that want to terminate in the
        // same time.
        agent_id = planMAMsgTerminateAgent(msg);
        if (agent_id == h->comm->node_id){
            // If our terminate-request circled in the whole ring and
            // came back, we can safely send TERMINATE signal to all
            // other agents and terminate.
            term_msg = planMAMsgNew(PLAN_MA_MSG_TERMINATE,
                                    PLAN_MA_MSG_TERMINATE_FINAL,
                                    h->comm->node_id);
            planMACommSendToAll(h->comm, term_msg);
            planMAMsgDel(term_msg);
            return -1;

        }else{
            // The request is from some other agent, just is send it to
            // the next agent in ring
            planMACommSendInRing(h->comm, msg);
        }
    }

    return 0;
}

void planMASearchMsgHandlerInit(plan_ma_search_msg_handler_t *h,
                                plan_ma_comm_t *comm)
{
    h->comm = comm;

    h->termination = 0;
}

void planMASearchMsgHandlerFree(plan_ma_search_msg_handler_t *h)
{
}

int planMASearchMsgHandler(plan_ma_search_msg_handler_t *h,
                           plan_ma_msg_t *msg)
{
    int type;
    int ret = 0;

    type = planMAMsgType(msg);
    fprintf(stderr, "[%d] Msg: %d\n", h->comm->node_id, type);
    fflush(stderr);

    // During termination ignore all messages
    if (h->termination && type != PLAN_MA_MSG_TERMINATE)
        return 0;

    switch (type){
        case PLAN_MA_MSG_TERMINATE:
            ret = terminateHandler(h, msg);
            break;
    }

    return ret;
}

