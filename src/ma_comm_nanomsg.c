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

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <boruvka/alloc.h>
#include "plan/ma_comm.h"

struct _plan_ma_comm_nanomsg_t {
    plan_ma_comm_t comm;
    int recv_sock;
    int *send_sock;
};
typedef struct _plan_ma_comm_nanomsg_t plan_ma_comm_nanomsg_t;
#define COMM(cm) bor_container_of(cm, plan_ma_comm_nanomsg_t, comm)

static void nanomsgDel(plan_ma_comm_t *comm);
static int nanomsgSendToNode(plan_ma_comm_t *comm, int node_id,
                             const plan_ma_msg_t *msg);
static plan_ma_msg_t *nanomsgRecv(plan_ma_comm_t *comm);
static plan_ma_msg_t *nanomsgRecvBlock(plan_ma_comm_t *comm, int timeout_in_ms);

static plan_ma_comm_t *nanomsgNew(int agent_id, int agent_size, char **urls)
{
    plan_ma_comm_nanomsg_t *comm;
    int i;

    comm = BOR_ALLOC(plan_ma_comm_nanomsg_t);
    _planMACommInit(&comm->comm, agent_id, agent_size,
                    nanomsgDel, nanomsgSendToNode,
                    nanomsgRecv, nanomsgRecvBlock);

    comm->recv_sock = nn_socket(AF_SP, NN_PULL);
    if (comm->recv_sock < 0){
        fprintf(stderr, "Nanomsg Error[%d]: Could not create a recv"
                " socket.\n", agent_id);
        BOR_FREE(comm);
        return NULL;
    }

    if (nn_bind(comm->recv_sock, urls[agent_id]) < 0){
        fprintf(stderr, "Error Nanomsg[%d]: Could not bind recv socket to"
                " url: `%s'\n", agent_id, urls[agent_id]);
        return NULL;
    }

    comm->send_sock = BOR_CALLOC_ARR(int, agent_size);
    for (i = 0; i < agent_size; ++i){
        if (i == agent_id)
            continue;

        comm->send_sock[i] = nn_socket(AF_SP, NN_PUSH);
        if (comm->recv_sock < 0){
            fprintf(stderr, "Nanomsg Error[%d]: Could not create a send"
                    " socket to %d.\n", agent_id, i);
            BOR_FREE(comm->send_sock);
            BOR_FREE(comm);
            return NULL;
        }

        if (nn_connect(comm->send_sock[i], urls[i]) < 0){
            fprintf(stderr, "Error Nanomsg[%d]: Could not connect to url:"
                    " `%s'\n", agent_id, urls[i]);
            BOR_FREE(comm->send_sock);
            BOR_FREE(comm);
            return NULL;
        }
    }

    return &comm->comm;
}

static plan_ma_comm_t *inprocIPCNew(int agent_id, int agent_size,
                                    const char *protocol,
                                    const char *prefix)
{
    int urlbuf_size = 256 * agent_size;
    char urlbuf[urlbuf_size];
    char *urls[agent_size];
    int i, size;

    size = 0;
    for (i = 0; i < agent_size; ++i){
        urls[i] = urlbuf + size;
        size += snprintf(urlbuf + size, urlbuf_size - size,
                         "%s://%s%d", protocol, prefix, i);
        size += 1;
    }

    return nanomsgNew(agent_id, agent_size, urls);
}

plan_ma_comm_t *planMACommInprocNew(int agent_id, int agent_size)
{
    return inprocIPCNew(agent_id, agent_size, "inproc", "a");
}

plan_ma_comm_t *planMACommIPCNew(int agent_id, int agent_size,
                                 const char *prefix)
{
    return inprocIPCNew(agent_id, agent_size, "ipc", prefix);
}

plan_ma_comm_t *planMACommTCPNew(int agent_id, int agent_size,
                                 const char **addr)
{
    plan_ma_comm_t *comm;
    char *urls[agent_size];
    int i;

    for (i = 0; i < agent_size; ++i){
        urls[i] = BOR_ALLOC_ARR(char, strlen(addr[i]) + 6 + 1);
        strcpy(urls[i], "tcp://");
        strcpy(urls[i] + 6, addr[i]);
    }

    comm = nanomsgNew(agent_id, agent_size, urls);

    for (i = 0; i < agent_size; ++i){
        BOR_FREE(urls[i]);
    }

    fprintf(stderr, "NN-opened\n");
    fflush(stderr);

    return comm;
}

static void nanomsgDel(plan_ma_comm_t *_comm)
{
    plan_ma_comm_nanomsg_t *comm = COMM(_comm);
    int i;

    fprintf(stderr, "NN-Close\n");
    fflush(stderr);
    nn_shutdown(comm->recv_sock, 0);
    nn_close(comm->recv_sock);
    for (i = 0; i < comm->comm.node_size; ++i){
        if (comm->send_sock[i] > 0){
            nn_shutdown(comm->send_sock[i], 0);
            nn_close(comm->send_sock[i]);
        }
    }
    BOR_FREE(comm->send_sock);
    BOR_FREE(comm);
}

static int nanomsgSendToNode(plan_ma_comm_t *_comm, int node_id,
                             const plan_ma_msg_t *msg)
{
    plan_ma_comm_nanomsg_t *comm = COMM(_comm);
    void *buf;
    size_t size;
    int send_count;
    int ret = 0;

    if (node_id == comm->comm.node_id)
        return -1;

    buf = planMAMsgPacked(msg, &size);
    send_count = nn_send(comm->send_sock[node_id], buf, size, 0);
    if (send_count != (int)size){
        fprintf(stderr, "Error Nanomsg[%d]: Could not send message to %d: %s\n",
                comm->comm.node_id, node_id, nn_strerror(errno));
        ret = -1;
    }

    if (buf)
        BOR_FREE(buf);
    return ret;
}

static plan_ma_msg_t *recv(plan_ma_comm_nanomsg_t *comm, int flag)
{
    void *buf;
    int recv_count;
    plan_ma_msg_t *msg = NULL;

    recv_count = nn_recv(comm->recv_sock, &buf, NN_MSG, flag);
    if (recv_count > 0){
        msg = planMAMsgUnpacked(buf, recv_count);
        nn_freemsg(buf);

    }else if (recv_count == 0){
        fprintf(stderr, "Error Nanomsg[%d]: Received zero-sized message.",
                comm->comm.node_id);
    }

    return msg;
}

static plan_ma_msg_t *nanomsgRecv(plan_ma_comm_t *_comm)
{
    plan_ma_comm_nanomsg_t *comm = COMM(_comm);
    plan_ma_msg_t *msg;

    msg = recv(comm, NN_DONTWAIT);
    if (msg == NULL && errno != EAGAIN){
        fprintf(stderr, "Error Nanomsg[%d]: Error while receiving"
                " message in non-blocking mode (errno: %d): %s\n",
                comm->comm.node_id, errno, nn_strerror(errno));
    }

    return msg;
}

static plan_ma_msg_t *recvBlock(plan_ma_comm_nanomsg_t *comm)
{
    plan_ma_msg_t *msg;

    msg = recv(comm, 0);
    if (msg == NULL){
        fprintf(stderr, "Error Nanomsg[%d]: Error while receiving"
                " message in blocking mode (errno: %d): %s\n",
                comm->comm.node_id, errno, nn_strerror(errno));
    }

    return msg;
}

static plan_ma_msg_t *recvTimeout(plan_ma_comm_nanomsg_t *comm, int timeout)
{
    plan_ma_msg_t *msg;

    nn_setsockopt(comm->recv_sock, NN_SOL_SOCKET, NN_RCVTIMEO,
                  (const void *)&timeout, sizeof(timeout));
    msg = recv(comm, 0);
    if (msg == NULL && errno != EAGAIN){
        fprintf(stderr, "Error Nanomsg[%d]: Error while receiving"
                " message in timeout mode (errno: %d): %s\n",
                comm->comm.node_id, errno, nn_strerror(errno));
    }

    // Reset socket to the blocking mode
    timeout = -1;
    nn_setsockopt(comm->recv_sock, NN_SOL_SOCKET, NN_RCVTIMEO,
                  (const void *)&timeout, sizeof(timeout));

    return msg;
}

static plan_ma_msg_t *nanomsgRecvBlock(plan_ma_comm_t *_comm, int timeout_in_ms)
{
    plan_ma_comm_nanomsg_t *comm = COMM(_comm);
    if (timeout_in_ms <= 0)
        return recvBlock(comm);
    return recvTimeout(comm, timeout_in_ms);
}
