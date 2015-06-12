#include <limits.h>
#include <stdio.h>
#include <boruvka/alloc.h>
#include <boruvka/ring_queue.h>

#include "plan/ma_comm.h"

#define QUEUE_SIZE 1024

struct _plan_ma_comm_inproc_t {
    plan_ma_comm_t comm;
    bor_ring_queue_t *write;
    bor_ring_queue_t *read;
};
typedef struct _plan_ma_comm_inproc_t plan_ma_comm_inproc_t;
#define INPROC(comm) bor_container_of((comm), plan_ma_comm_inproc_t, comm)

struct _plan_ma_comm_inproc_pool_t {
    bor_ring_queue_t *queue; /*!< Array of communication queues */
    int size;
};

/** Comm methods: */
static void inprocDel(plan_ma_comm_t *comm);
static int inprocSendToNode(plan_ma_comm_t *comm, int node_id,
                         const plan_ma_msg_t *msg);
static plan_ma_msg_t *inprocRecv(plan_ma_comm_t *comm);
static plan_ma_msg_t *inprocRecvBlock(plan_ma_comm_t *comm, int timeout_in_ms);

plan_ma_comm_inproc_pool_t *planMACommInprocPoolNew(int agent_size)
{
    plan_ma_comm_inproc_pool_t *pool;
    int i;

    pool = BOR_ALLOC(plan_ma_comm_inproc_pool_t);
    pool->queue = BOR_ALLOC_ARR(bor_ring_queue_t, agent_size);
    pool->size = agent_size;

    for (i = 0; i < pool->size; ++i)
        borRingQueueInit(pool->queue + i, QUEUE_SIZE);
    return pool;
}

void planMACommInprocPoolDel(plan_ma_comm_inproc_pool_t *pool)
{
    int i;
    plan_ma_msg_t *msg;

    for (i = 0; i < pool->size; ++i){
        while ((msg = borRingQueuePop(pool->queue + i)) != NULL)
            planMAMsgDel(msg);
        borRingQueueFree(pool->queue + i);
    }
    if (pool->queue)
        BOR_FREE(pool->queue);
    BOR_FREE(pool);
}

plan_ma_comm_t *planMACommInprocNew(plan_ma_comm_inproc_pool_t *pool,
                                    int agent_id)
{
    plan_ma_comm_inproc_t *inproc;

    inproc = BOR_ALLOC(plan_ma_comm_inproc_t);
    _planMACommInit(&inproc->comm, agent_id, pool->size,
                    inprocDel, inprocSendToNode, inprocRecv,
                    inprocRecvBlock);
    inproc->write = pool->queue;
    inproc->read = pool->queue + agent_id;

    return &inproc->comm;
}

static void inprocDel(plan_ma_comm_t *comm)
{
    plan_ma_comm_inproc_t *inproc = INPROC(comm);
    BOR_FREE(inproc);
}

static int inprocSendToNode(plan_ma_comm_t *comm, int node_id,
                         const plan_ma_msg_t *msg)
{
    plan_ma_comm_inproc_t *inproc = INPROC(comm);
    plan_ma_msg_t *msgout;

    msgout = planMAMsgClone(msg);
    borRingQueuePush(inproc->write + node_id, msgout);
    return 0;
}

static plan_ma_msg_t *inprocRecv(plan_ma_comm_t *comm)
{
    plan_ma_comm_inproc_t *inproc = INPROC(comm);
    return borRingQueuePop(inproc->read);
}

static plan_ma_msg_t *inprocRecvBlock(plan_ma_comm_t *comm, int timeout_in_ms)
{
    plan_ma_comm_inproc_t *inproc = INPROC(comm);
    if (timeout_in_ms <= 0){
        return borRingQueuePopBlock(inproc->read);
    }else{
        return borRingQueuePopBlockTimeout(inproc->read, timeout_in_ms);
    }
}
