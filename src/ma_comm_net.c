#include <string.h>
#include <zmq.h>
#include <boruvka/alloc.h>

#include "plan/ma_comm_net.h"

void planMACommNetConfInit(plan_ma_comm_net_conf_t *cfg)
{
    bzero(cfg, sizeof(*cfg));
    cfg->node_id = -1;
}

void planMACommNetConfFree(plan_ma_comm_net_conf_t *cfg)
{
    int i;

    for (i = 0; i < cfg->node_size; ++i)
        BOR_FREE(cfg->addr[i]);
    planMACommNetConfInit(cfg);
}

int planMACommNetConfAddNode(plan_ma_comm_net_conf_t *cfg,
                             const char *addr)
{
    ++cfg->node_size;
    cfg->addr = BOR_REALLOC_ARR(cfg->addr, char *, cfg->node_size);
    cfg->addr[cfg->node_size - 1] = strdup(addr);
    return cfg->node_size - 1;
}


plan_ma_comm_net_t *planMACommNetNew(const plan_ma_comm_net_conf_t *cfg)
{
    plan_ma_comm_net_t *comm;
    int i, linger, high_water_mark;
    char addr[1024];
    void *sock;

    if (cfg->node_id < 0 || cfg->node_id >= cfg->node_size)
        return NULL;

    // Prepare addr string
    strcpy(addr, "tcp://");

    comm = BOR_ALLOC(plan_ma_comm_net_t);
    comm->node_id = cfg->node_id;
    comm->node_size = cfg->node_size;

    comm->context = zmq_ctx_new();
    if (comm->context == NULL){
        fprintf(stderr, "Error: Could not create zeromq context.\n");
        BOR_FREE(comm);
        return NULL;
    }

    comm->readsock = zmq_socket(comm->context, ZMQ_DEALER);
    if (comm->readsock == NULL){
        fprintf(stderr, "Error: Could not create zeromq read socket.\n");
        zmq_ctx_destroy(comm->context);
        BOR_FREE(comm);
        return NULL;
    }

    strcpy(addr + 6, cfg->addr[cfg->node_id]);
    if (zmq_bind(comm->readsock, addr) != 0){
        fprintf(stderr, "Error: Could not bind read socket to `%s'\n", addr);
        zmq_ctx_destroy(comm->context);
        BOR_FREE(comm);
        return NULL;
    }

    comm->writesock = BOR_ALLOC_ARR(void *, comm->node_size);
    for (i = 0; i < cfg->node_size; ++i){
        if (i == cfg->node_id){
            comm->writesock[i] = NULL;
            continue;
        }

        sock = zmq_socket(comm->context, ZMQ_DEALER);
        if (sock == NULL){
            fprintf(stderr, "Error: Could not create zeromq write socket.\n");
            zmq_ctx_destroy(comm->context);
            BOR_FREE(comm->writesock);
            BOR_FREE(comm);
            return NULL;
        }

        linger = 0;
        if (zmq_setsockopt(sock, ZMQ_LINGER, &linger, sizeof(int)) != 0){
            fprintf(stderr, "Error: Could not set linger option.\n");
            zmq_ctx_destroy(comm->context);
            BOR_FREE(comm->writesock);
            BOR_FREE(comm);
            return NULL;
        }

        // set unlimited high water mark for all outgoing connections
        high_water_mark = 0;
        if (zmq_setsockopt(sock, ZMQ_SNDHWM, &high_water_mark, sizeof(int)) != 0){
            fprintf(stderr, "Error: Could not set high-water-mark option.\n");
            zmq_ctx_destroy(comm->context);
            BOR_FREE(comm->writesock);
            BOR_FREE(comm);
            return NULL;
        }

        strcpy(addr + 6, cfg->addr[i]);
        if (zmq_connect(sock, addr) != 0){
            fprintf(stderr, "Error: Could not connect to remote peer `%s'\n",
                    addr);
            zmq_ctx_destroy(comm->context);
            BOR_FREE(comm->writesock);
            BOR_FREE(comm);
            return NULL;
        }

        comm->writesock[i] = sock;
    }

    return comm;
}

void planMACommNetDel(plan_ma_comm_net_t *comm)
{
    int i;

    for (i = 0; i < comm->node_size; ++i){
        if (comm->writesock[i])
            zmq_close(comm->writesock[i]);
    }

    if (comm->readsock)
        zmq_close(comm->readsock);

    if (comm->context)
        zmq_ctx_destroy(comm->context);
    BOR_FREE(comm);
}


int planMACommNetSendToAll(plan_ma_comm_net_t *comm,
                           const plan_ma_msg_t *msg)
{
    int i;

    for (i = 0; i < comm->node_size; ++i){
        if (i != comm->node_id){
            if (planMACommNetSendToNode(comm, i, msg) != 0)
                return -1;
        }
    }
    return 0;
}

int planMACommNetSendToNode(plan_ma_comm_net_t *comm,
                            int node_id,
                            const plan_ma_msg_t *msg)
{
    void *buf;
    size_t size;
    int send_count;

    buf = planMAMsgPacked(msg, &size);
    send_count = zmq_send(comm->writesock[node_id], buf, size, 0);
    if (send_count != (int)size){
        fprintf(stderr, "Error ZeroMQ[%d]: Could not send message to %d.\n",
                comm->node_id, node_id);
        if (buf)
            BOR_FREE(buf);
        return -1;
    }

    if (buf)
        BOR_FREE(buf);
    return 0;
}

int planMACommNetSendInRing(plan_ma_comm_net_t *comm,
                            const plan_ma_msg_t *msg)
{
    int next_node_id;
    next_node_id = (comm->node_id + 1) % comm->node_size;
    return planMACommNetSendToNode(comm, next_node_id, msg);
}

static plan_ma_msg_t *recv(plan_ma_comm_net_t *comm, int flag)
{
    plan_ma_msg_t *msg;
    zmq_msg_t zmsg;
    int recv_count, timeout;
    size_t size;

    // initialize zeromq message
    if (zmq_msg_init(&zmsg) != 0){
        fprintf(stderr, "Error ZeroMQ[%d]: Could not initialize message.\n",
                comm->node_id);
        return NULL;
    }

    // receive data from remote
    recv_count = zmq_msg_recv(&zmsg, comm->readsock, flag);
    if (recv_count <= 0){
        if (errno != EAGAIN){
            zmq_getsockopt(comm->readsock, ZMQ_RCVTIMEO, &timeout, &size);
            if (timeout == -1)
                fprintf(stderr, "Error ZeroMQ[%d]: Error during receiving"
                                " message: %d\n", comm->node_id, errno);
        }

        zmq_msg_close(&zmsg);
        return NULL;
    }

    msg = planMAMsgUnpacked(zmq_msg_data(&zmsg), zmq_msg_size(&zmsg));
    zmq_msg_close(&zmsg);
    return msg;
}

plan_ma_msg_t *planMACommNetRecv(plan_ma_comm_net_t *comm)
{
    return recv(comm, 0);
}

plan_ma_msg_t *planMACommNetRecvBlock(plan_ma_comm_net_t *comm)
{
    return recv(comm, ZMQ_DONTWAIT);
}

plan_ma_msg_t *planMACommNetRecvBlockTimeout(plan_ma_comm_net_t *comm,
                                             int timeout_in_ms)
{
    plan_ma_msg_t *msg;
    int milisec;

    // Set timeout on socket
    milisec = timeout_in_ms;
    zmq_setsockopt(comm->readsock, ZMQ_RCVTIMEO, &milisec, sizeof(int));

    // Receive as if it was blocking operation
    msg = recv(comm, 0);

    // Set back infinite wait (blocking)
    milisec = -1;
    zmq_setsockopt(comm->readsock, ZMQ_RCVTIMEO, &milisec, sizeof(int));
    return msg;
}
