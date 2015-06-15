/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#ifndef __PLAN_MA_COMM_H__
#define __PLAN_MA_COMM_H__

#include <plan/ma_msg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Disable usage of an extra thread for receiving messages.
 */
#define PLAN_MA_COMM_TCP_NO_THREAD 0x1u

typedef struct _plan_ma_comm_inproc_pool_t plan_ma_comm_inproc_pool_t;

typedef struct _plan_ma_comm_t plan_ma_comm_t;

typedef void (*plan_ma_comm_del_fn)(plan_ma_comm_t *comm);
typedef int (*plan_ma_comm_send_to_node_fn)(plan_ma_comm_t *comm, int node_id,
                                            const plan_ma_msg_t *msg);
typedef plan_ma_msg_t *(*plan_ma_comm_recv_fn)(plan_ma_comm_t *comm);
typedef plan_ma_msg_t *(*plan_ma_comm_recv_block_fn)(plan_ma_comm_t *comm,
                                                     int timeout_in_ms);
struct _plan_ma_comm_t {
    int node_id;
    int node_size;

    plan_ma_comm_del_fn del_fn;
    plan_ma_comm_send_to_node_fn send_to_node_fn;
    plan_ma_comm_recv_fn recv_fn;
    plan_ma_comm_recv_block_fn recv_block_fn;
};


/**
 * Creates a pool of inproc communication channels.
 */
plan_ma_comm_inproc_pool_t *planMACommInprocPoolNew(int agent_size);

/**
 * Deletes an inproc pool.
 */
void planMACommInprocPoolDel(plan_ma_comm_inproc_pool_t *pool);

/**
 * Creates an intra-process communication channel between specified agent
 * and all other agents. The channel is local to the process, i.e., the
 * agents must run in separate threads.
 */
plan_ma_comm_t *planMACommInprocNew(plan_ma_comm_inproc_pool_t *pool,
                                    int agent_id);

/**
 * Creates an inter-process communication channel between specified agent
 * and all other agents. The channel is based on TCP protocol, i.e., agents
 * can run on any cluster of machines connected with network.
 * A list of addresses of each agent must be provided (thus addr must have
 * at least agent_size items).
 */
plan_ma_comm_t *planMACommTCPNew(int agent_id, int agent_size,
                                 const char **addr, unsigned flags);


/**
 * Destroys a communication channel.
 */
_bor_inline void planMACommDel(plan_ma_comm_t *comm);

/**
 * Returns ID of the node.
 */
_bor_inline int planMACommId(const plan_ma_comm_t *comm);

/**
 * Returns number of nodes in cluster.
 */
_bor_inline int planMACommSize(const plan_ma_comm_t *comm);

/**
 * Sends the message to all nodes.
 * Returns 0 on success.
 */
_bor_inline int planMACommSendToAll(plan_ma_comm_t *comm,
                                    const plan_ma_msg_t *msg);

/**
 * Sends the message to the specified node.
 * Returns 0 on success.
 */
_bor_inline int planMACommSendToNode(plan_ma_comm_t *comm, int node_id,
                                     const plan_ma_msg_t *msg);

/**
 * Sends the message to the next node in ring.
 * Returns 0 on success.
 */
_bor_inline int planMACommSendInRing(plan_ma_comm_t *comm,
                                     const plan_ma_msg_t *msg);

/**
 * Receives a next message in non-blocking mode.
 * It is caller's responsibility to destroy the returned message.
 */
_bor_inline plan_ma_msg_t *planMACommRecv(plan_ma_comm_t *comm);

/**
 * Receives a next message in blocking mode.
 * It is caller's responsibility to destroy the returned message.
 * If timeout_in_ms is set to non-zero value, the function blocks only for
 * the specified amount of time.
 */
_bor_inline plan_ma_msg_t *planMACommRecvBlock(plan_ma_comm_t *comm,
                                               int timeout_in_ms);


void _planMACommInit(plan_ma_comm_t *comm, int agent_id, int agent_size,
                     plan_ma_comm_del_fn del_fn,
                     plan_ma_comm_send_to_node_fn send_to_node_fn,
                     plan_ma_comm_recv_fn recv_fn,
                     plan_ma_comm_recv_block_fn recv_block_fn);

/**** INLINES: ****/
_bor_inline void planMACommDel(plan_ma_comm_t *comm)
{
    comm->del_fn(comm);
}

_bor_inline int planMACommId(const plan_ma_comm_t *comm)
{
    return comm->node_id;
}

_bor_inline int planMACommSize(const plan_ma_comm_t *comm)
{
    return comm->node_size;
}

_bor_inline int planMACommSendToAll(plan_ma_comm_t *comm,
                                    const plan_ma_msg_t *msg)
{
    int i;
    for (i = 0; i < comm->node_size; ++i){
        if (i == comm->node_id)
            continue;

        if (planMACommSendToNode(comm, i, msg) != 0)
            return -1;
    }
    return 0;
}

_bor_inline int planMACommSendToNode(plan_ma_comm_t *comm, int node_id,
                                     const plan_ma_msg_t *msg)
{
    return comm->send_to_node_fn(comm, node_id, msg);
}

_bor_inline int planMACommSendInRing(plan_ma_comm_t *comm,
                                     const plan_ma_msg_t *msg)
{
    int node_id;
    node_id = (comm->node_id + 1) % comm->node_size;
    return planMACommSendToNode(comm, node_id, msg);
}

_bor_inline plan_ma_msg_t *planMACommRecv(plan_ma_comm_t *comm)
{
    if (comm->recv_fn)
        return comm->recv_fn(comm);
    return comm->recv_block_fn(comm, 0);
}

_bor_inline plan_ma_msg_t *planMACommRecvBlock(plan_ma_comm_t *comm,
                                               int timeout_in_ms)
{
    return comm->recv_block_fn(comm, timeout_in_ms);
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_COMM_H__ */
