#include "plan/ma_comm.h"

void _planMACommInit(plan_ma_comm_t *comm, int agent_id, int agent_size,
                     plan_ma_comm_del_fn del_fn,
                     plan_ma_comm_send_to_node_fn send_to_node_fn,
                     plan_ma_comm_recv_fn recv_fn,
                     plan_ma_comm_recv_block_fn recv_block_fn)
{
    comm->node_id = agent_id;
    comm->node_size = agent_size;
    comm->del_fn = del_fn;
    comm->send_to_node_fn = send_to_node_fn;
    comm->recv_fn = recv_fn;
    comm->recv_block_fn = recv_block_fn;
}

