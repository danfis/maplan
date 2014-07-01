#include <limits.h>
#include <stdio.h>
#include <boruvka/alloc.h>

#include "ma_comm_queue.h"

struct _msg_buf_t {
    uint8_t *buf;
    size_t size;
};
typedef struct _msg_buf_t msg_buf_t;

/** Recieve message in blocking or non-blocking mode */
static PlanMultiAgentMsg *recv(plan_ma_comm_queue_t *comm, int block);

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
        pool->queue[i].pool    = *pool;
        pool->queue[i].node_id = i;
        pool->queue[i].arbiter = 0;
    }
    pool->queue[0].arbiter = 1;

    return pool;
}

void planMACommQueuePoolDel(plan_ma_comm_queue_pool_t *pool)
{
    int i;

    for (i = 0; i < pool->node_size; ++i){
        borFifoFree(&pool->node[i].fifo);
        pthread_mutex_destroy(&pool->node[i].lock);
        sem_destroy(&pool->node[i].full);
        sem_destroy(&pool->node[i].empty);
    }

    BOR_FREE(pool->node);
    BOR_FREE(pool->queue);
    BOR_FREE(pool);
}

plan_ma_comm_queue_t *planMACommQueue(plan_ma_comm_queue_pool_t *pool,
                                      int node_id)
{
    return &pool->queue[node_id];
}

int planMACommQueueSendToAll(plan_ma_comm_queue_t *comm,
                             const PlanMultiAgentMsg *msg)
{
    int i;

    for (i = 0; i < comm->pool.node_size; ++i){
        if (planMACommQueueSendToNode(comm, i, msg) != 0)
            return -1;
    }

    return 0;
}

int planMACommQueueSendToArbiter(plan_ma_comm_queue_t *comm,
                                 const PlanMultiAgentMsg *msg)
{
    if (comm->node_id == 0)
        return -1;

    return planMACommQueueSendToNode(comm, 0, msg);
}

int planMACommQueueSendToNode(plan_ma_comm_queue_t *comm,
                              int node_id,
                              const PlanMultiAgentMsg *msg)
{
    msg_buf_t buf;
    plan_ma_comm_queue_node_t *node;

    buf.size = plan_multi_agent_msg__get_packed_size(msg);
    buf.buf = BOR_ALLOC_ARR(uint8_t, buf.size);
    plan_multi_agent_msg__pack(msg, buf.buf);

    node = comm->pool.node + node_id;

    // reserve item in queue
    sem_wait(&node->empty);

    // add message to the queue
    pthread_mutex_lock(&node->lock);
    borFifoPush(&node->fifo, &buf);
    pthread_mutex_unlock(&node->lock);

    // unblock waiting thread
    sem_wait(&node->full);

    return 0;
}

PlanMultiAgentMsg *planMACommQueueRecv(plan_ma_comm_queue_t *comm)
{
    return recv(comm, 0);
}

PlanMultiAgentMsg *planMACommQueueRecvBlock(plan_ma_comm_queue_t *comm)
{
    return recv(comm, 1);
}


static PlanMultiAgentMsg *recv(plan_ma_comm_queue_t *comm, int block)
{
    plan_ma_comm_queue_node_t *node = comm->pool.node + comm->node_id;
    msg_buf_t *buf;
    uint8_t *packed;
    size_t size;
    PlanMultiAgentMsg *msg;

    if (block){
        // wait for available messages
        sem_wait(&node->full);
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
    msg = plan_multi_agent_msg__unpack(NULL, size, packed);
    return msg;
}
