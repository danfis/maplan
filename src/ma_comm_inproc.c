#include <limits.h>
#include <stdio.h>
#include <boruvka/alloc.h>
#include <boruvka/fifo-sem.h>

#include "plan/ma_comm.h"

struct _plan_ma_comm_inproc_t {
    plan_ma_comm_t comm;
    bor_fifo_sem_t *write;
    bor_fifo_sem_t *read;
};
typedef struct _plan_ma_comm_inproc_t plan_ma_comm_inproc_t;
#define INPROC(comm) bor_container_of((comm), plan_ma_comm_inproc_t, comm)

struct _plan_ma_comm_inproc_pool_t {
    bor_fifo_sem_t *queue; /*!< Array of communication queues */
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
    pool->queue = BOR_ALLOC_ARR(bor_fifo_sem_t, agent_size);
    pool->size = agent_size;

    for (i = 0; i < pool->size; ++i){
        if (borFifoSemInit(pool->queue + i, sizeof(plan_ma_msg_t *)) != 0)
            return NULL;
    }
    return pool;
}

void planMACommInprocPoolDel(plan_ma_comm_inproc_pool_t *pool)
{
    int i;
    plan_ma_msg_t *msg;

    for (i = 0; i < pool->size; ++i){
        while (!borFifoSemEmpty(pool->queue + i)){
            borFifoSemPop(pool->queue + i, &msg);
            planMAMsgDel(msg);
        }
        borFifoSemFree(pool->queue + i);
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
    borFifoSemPush(inproc->write + node_id, &msgout);
    return 0;
}

static plan_ma_msg_t *inprocRecv(plan_ma_comm_t *comm)
{
    plan_ma_comm_inproc_t *inproc = INPROC(comm);
    plan_ma_msg_t *msg;

    if (borFifoSemPop(inproc->read, &msg) == 0)
        return msg;
    return NULL;
}

static plan_ma_msg_t *inprocRecvBlock(plan_ma_comm_t *comm, int timeout_in_ms)
{
    plan_ma_comm_inproc_t *inproc = INPROC(comm);
    plan_ma_msg_t *msg;

    if (timeout_in_ms <= 0){
        if (borFifoSemPopBlock(inproc->read, &msg) == 0)
            return msg;
        return NULL;
    }else{
        if (borFifoSemPopBlockTimeout(inproc->read, timeout_in_ms, &msg) == 0)
            return msg;
        return NULL;
    }
}
