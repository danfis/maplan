#include <limits.h>
#include <stdio.h>
#include <boruvka/alloc.h>

#include "plan/ma_comm.h"

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

/** Recieve message in blocking or non-blocking mode */
static plan_ma_msg_t *recv(plan_ma_comm_queue_t *comm, int block,
                           const struct timespec *timeout);

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
        borFifoInit(&node->fifo, sizeof(msg_buf_t));

        if (pthread_mutex_init(&node->lock, NULL) != 0){
            fprintf(stderr, "Error: Could not initialize mutex!\n");
            return NULL;
        }

        if (sem_init(&node->full, 0, 0) != 0){
            fprintf(stderr, "Error: Could not initialize semaphore (full)!\n");
            return NULL;
        }

        if (sem_init(&node->empty, 0, SEM_VALUE_MAX) != 0){
            fprintf(stderr, "Error: Could not initialize semaphore (empty)!\n");
            return NULL;
        }
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

        while (!borFifoEmpty(&node->fifo)){
            buf = borFifoFront(&node->fifo);
            BOR_FREE(buf->buf);
            borFifoPop(&node->fifo);
        }
        pthread_mutex_destroy(&node->lock);
        sem_destroy(&node->full);
        sem_destroy(&node->empty);

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

    // reserve item in queue
    sem_wait(&node->empty);

    // add message to the queue
    pthread_mutex_lock(&node->lock);
    borFifoPush(&node->fifo, &buf);
    pthread_mutex_unlock(&node->lock);

    // unblock waiting thread
    sem_post(&node->full);

    return 0;
}


static plan_ma_msg_t *planMACommQueueRecv(plan_ma_comm_t *_comm)
{
    plan_ma_comm_queue_t *comm = COMM_FROM_PARENT(_comm);
    return recv(comm, 0, NULL);
}

static plan_ma_msg_t *recvBlockTimeout(plan_ma_comm_queue_t *comm,
                                       int timeout_in_ms)
{
    struct timespec tm;
    int sec;
    long nsec;

    sec = timeout_in_ms / 1000;
    nsec = (timeout_in_ms % 1000L) * 1000L * 1000L;

    clock_gettime(CLOCK_REALTIME, &tm);
    tm.tv_sec += sec;
    tm.tv_nsec += nsec;
    if (tm.tv_nsec > 1000L * 1000L * 1000L){
        tm.tv_nsec %= 1000L * 1000L * 1000L;
        tm.tv_sec += 1;
    }
    return recv(comm, 1, &tm);
}

static plan_ma_msg_t *planMACommQueueRecvBlock(plan_ma_comm_t *_comm,
                                               int timeout_in_ms)
{
    plan_ma_comm_queue_t *comm = COMM_FROM_PARENT(_comm);

    if (timeout_in_ms == 0)
        return recv(comm, 1, NULL);
    return recvBlockTimeout(comm, timeout_in_ms);
}


static plan_ma_msg_t *recv(plan_ma_comm_queue_t *comm, int block,
                           const struct timespec *timeout)
{
    plan_ma_comm_queue_node_t *node = comm->pool.node + comm->comm.node_id;
    msg_buf_t *buf;
    uint8_t *packed;
    size_t size;
    plan_ma_msg_t *msg;

    if (block){
        // wait for available messages
        if (timeout){
            if (sem_timedwait(&node->full, timeout) != 0)
                return NULL;
        }else{
            sem_wait(&node->full);
        }
    }else{
        // wait for available messages or exit if there is none
        if (sem_trywait(&node->full) != 0)
            return NULL;
    }

    // pick up message and remove the message from fifo
    pthread_mutex_lock(&node->lock);
    buf = borFifoFront(&node->fifo);
    packed = buf->buf;
    size = buf->size;

    borFifoPop(&node->fifo);
    pthread_mutex_unlock(&node->lock);

    // free room for next message
    sem_post(&node->empty);

    // unpack message and return it
    msg = planMAMsgUnpacked(packed, size);
    BOR_FREE(packed);

    return msg;
}
