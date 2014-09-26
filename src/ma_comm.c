#include "plan/ma_comm.h"

static plan_ma_msg_t *waitlistNext(bor_fifo_t *waitlist)
{
    plan_ma_msg_t *msg = NULL;

    if (!borFifoEmpty(waitlist)){
        msg = *(plan_ma_msg_t **)borFifoFront(waitlist);
        borFifoPop(waitlist);
    }

    return msg;
}

plan_ma_msg_t *planMACommRecv(plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg = waitlistNext(&comm->waitlist);
    if (!msg)
        msg = comm->recv_fn(comm);
    return msg;
}

plan_ma_msg_t *planMACommRecvBlock(plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg = waitlistNext(&comm->waitlist);
    if (!msg)
        msg = comm->recv_block_fn(comm);
    return msg;
}

plan_ma_msg_t *planMACommRecvBlockTimeout(plan_ma_comm_t *comm,
                                          int timeout_in_ms)
{
    plan_ma_msg_t *msg = waitlistNext(&comm->waitlist);
    if (!msg)
        msg = comm->recv_block_timeout_fn(comm, timeout_in_ms);
    return msg;
}

plan_ma_msg_t *planMACommRecvBlockType(plan_ma_comm_t *comm, int msg_type)
{
    plan_ma_msg_t *msg;

    while ((msg = comm->recv_block_fn(comm)) != NULL){
        if (planMAMsgIsType(msg, msg_type)){
            return msg;
        }else{
            borFifoPush(&comm->waitlist, &msg);
        }
    }

    return NULL;
}

void _planMACommInit(plan_ma_comm_t *comm,
                     int node_id,
                     int node_size,
                     plan_ma_comm_del_fn del_fn,
                     plan_ma_comm_send_to_node_fn send_to_node_fn,
                     plan_ma_comm_recv_fn recv_fn,
                     plan_ma_comm_recv_block_fn recv_block_fn,
                     plan_ma_comm_recv_block_timeout_fn recv_block_timeout_fn)
{
    comm->node_id = node_id;
    comm->node_size = node_size;
    comm->del_fn = del_fn;
    comm->send_to_node_fn = send_to_node_fn;
    comm->recv_fn = recv_fn;
    comm->recv_block_fn = recv_block_fn;
    comm->recv_block_timeout_fn = recv_block_timeout_fn;

    borFifoInit(&comm->waitlist, sizeof(plan_ma_msg_t *));
}

void _planMACommFree(plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;

    // Clean fifo queue
    while (!borFifoEmpty(&comm->waitlist)){
        msg = borFifoFront(&comm->waitlist);
        borFifoPop(&comm->waitlist);
        planMAMsgDel(msg);
    }
    borFifoFree(&comm->waitlist);
}
