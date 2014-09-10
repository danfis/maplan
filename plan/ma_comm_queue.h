#ifndef __PLAN_MA_COMM_QUEUE_H__
#define __PLAN_MA_COMM_QUEUE_H__

#include <pthread.h>
#include <semaphore.h>
#include <boruvka/fifo.h>

#include <plan/ma_comm.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Forward declarations */
typedef struct _plan_ma_comm_queue_pool_t plan_ma_comm_queue_pool_t;
typedef struct _plan_ma_comm_queue_t plan_ma_comm_queue_t;

struct _plan_ma_comm_queue_node_t {
    bor_fifo_t fifo;      /*!< Queue with messages */
    pthread_mutex_t lock; /*!< Mutex-lock for the queue */
    sem_t full;           /*!< Full semaphore for the queue */
    sem_t empty;          /*!< Empty semaphore for the queue */
};
typedef struct _plan_ma_comm_queue_node_t plan_ma_comm_queue_node_t;

struct _plan_ma_comm_queue_pool_t {
    plan_ma_comm_queue_node_t *node; /*!< Array of communaction nodes */
    int node_size;                   /*!< Number of nodes */
    plan_ma_comm_queue_t *queue;
};


/**
 * Creates a pool of communication queues for specified number of nodes.
 */
plan_ma_comm_queue_pool_t *planMACommQueuePoolNew(int num_nodes);

/**
 * Destroys a queue pool.
 */
void planMACommQueuePoolDel(plan_ma_comm_queue_pool_t *pool);

/**
 * Returns a queue corresponding to the specified node.
 * The returned queue is still owned by the pool and the call should NOT
 * call planMACommDel() on it!
 */
plan_ma_comm_t *planMACommQueue(plan_ma_comm_queue_pool_t *pool,
                                int node_id);


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_COMM_QUEUE_H__ */
