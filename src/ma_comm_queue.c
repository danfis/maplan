#include <limits.h>
#include <stdio.h>
#include <boruvka/alloc.h>
#include <boruvka/fifo-sem.h>

#include "plan/ma_comm.h"

struct _plan_ma_comm_queue_node_t {
    bor_fifo_sem_t queue;
};

struct _msg_buf_t {
    uint8_t *buf;
    size_t size;
};
typedef struct _msg_buf_t msg_buf_t;

struct _plan_ma_comm_queue_t {
    plan_ma_comm_t comm;
    plan_ma_comm_queue_pool_t pool; /*!< Corresponding pool */
};

#define COMM_FROM_PARENT(parent) \
    bor_container_of((parent), plan_ma_comm_queue_t, comm)

static int planMACommQueueSendToNode(plan_ma_comm_t *comm,
                                     int node_id,
                                     const plan_ma_msg_t *msg);
static plan_ma_msg_t *planMACommQueueRecv(plan_ma_comm_t *comm);
static plan_ma_msg_t *planMACommQueueRecvBlock(plan_ma_comm_t *comm,
                                               int timeout_in_ms);

plan_ma_comm_queue_pool_t *planMACommQueuePoolNew(int num_nodes)
{
    int i;
    plan_ma_comm_queue_pool_t *pool;
    plan_ma_comm_queue_node_t *node;

    pool = BOR_ALLOC(plan_ma_comm_queue_pool_t);
    pool->node_size = num_nodes;

    pool->node = BOR_ALLOC_ARR(plan_ma_comm_queue_node_t, pool->node_size);
    for (i = 0; i < pool->node_size; ++i){
        node = pool->node + i;
        if (borFifoSemInit(&node->queue, sizeof(msg_buf_t)) != 0)
            return NULL;
    }

    pool->queue = BOR_ALLOC_ARR(plan_ma_comm_queue_t, pool->node_size);
    for (i = 0; i < pool->node_size; ++i){
        _planMACommInit(&pool->queue[i].comm,
                        i, pool->node_size,
                        NULL,
                        planMACommQueueSendToNode,
                        planMACommQueueRecv,
                        planMACommQueueRecvBlock);
        pool->queue[i].pool = *pool;
    }

    return pool;
}

void planMACommQueuePoolDel(plan_ma_comm_queue_pool_t *pool)
{
    msg_buf_t *buf;
    plan_ma_comm_queue_node_t *node;
    int i;

    for (i = 0; i < pool->node_size; ++i){
        node = pool->node + i;

        while (!borFifoEmpty(&node->queue.fifo)){
            buf = borFifoFront(&node->queue.fifo);
            BOR_FREE(buf->buf);
            borFifoPop(&node->queue.fifo);
        }
        borFifoSemFree(&node->queue);

        _planMACommFree(&pool->queue[i].comm);
    }

    BOR_FREE(pool->node);
    BOR_FREE(pool->queue);
    BOR_FREE(pool);
}

plan_ma_comm_t *planMACommQueue(plan_ma_comm_queue_pool_t *pool,
                                int node_id)
{
    return &pool->queue[node_id].comm;
}

static int planMACommQueueSendToNode(plan_ma_comm_t *_comm,
                                     int node_id,
                                     const plan_ma_msg_t *msg)
{
    plan_ma_comm_queue_t *comm = COMM_FROM_PARENT(_comm);
    msg_buf_t buf;
    plan_ma_comm_queue_node_t *node;

    buf.buf = planMAMsgPacked(msg, &buf.size);
    node = comm->pool.node + node_id;

    borFifoSemPush(&node->queue, &buf);
    return 0;
}


static plan_ma_msg_t *planMACommQueueRecv(plan_ma_comm_t *_comm)
{
    plan_ma_comm_queue_t *comm = COMM_FROM_PARENT(_comm);
    plan_ma_comm_queue_node_t *node = comm->pool.node + comm->comm.node_id;
    msg_buf_t buf;
    plan_ma_msg_t *msg;

    if (borFifoSemPop(&node->queue, &buf) != 0)
        return NULL;

    msg = planMAMsgUnpacked(buf.buf, buf.size);
    BOR_FREE(buf.buf);
    return msg;
}

static plan_ma_msg_t *planMACommQueueRecvBlock(plan_ma_comm_t *_comm,
                                               int timeout_in_ms)
{
    plan_ma_comm_queue_t *comm = COMM_FROM_PARENT(_comm);
    plan_ma_comm_queue_node_t *node = comm->pool.node + comm->comm.node_id;
    msg_buf_t buf;
    plan_ma_msg_t *msg;

    if (timeout_in_ms > 0){
        if (borFifoSemPopBlockTimeout(&node->queue, timeout_in_ms, &buf) != 0)
            return NULL;
    }else{
        if (borFifoSemPopBlock(&node->queue, &buf) != 0)
            return NULL;
    }

    msg = planMAMsgUnpacked(buf.buf, buf.size);
    BOR_FREE(buf.buf);
    return msg;
}
