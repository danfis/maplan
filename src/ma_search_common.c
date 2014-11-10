#include "ma_search_common.h"

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
