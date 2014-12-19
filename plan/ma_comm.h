#ifndef __PLAN_MA_COMM_H__
#define __PLAN_MA_COMM_H__

#include <plan/config.h>
#include <plan/ma_msg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Forward declaration */
typedef struct _plan_ma_comm_t plan_ma_comm_t;
typedef struct _plan_ma_comm_queue_pool_t plan_ma_comm_queue_pool_t;
typedef struct _plan_ma_comm_queue_t plan_ma_comm_queue_t;
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



#ifdef PLAN_ZMQ
struct _plan_ma_comm_net_conf_t {
    int node_id;   /*!< Id of the current node */
    char **addr;   /*!< TCP address of all nodes in cluster */
    int node_size; /*!< Number of nodes in cluster */
};
typedef struct _plan_ma_comm_net_conf_t plan_ma_comm_net_conf_t;


/**
 * Initialize config structure.
 */
void planMACommNetConfInit(plan_ma_comm_net_conf_t *cfg);

/**
 * Frees resources allocate with connection with conf structure.
 */
void planMACommNetConfFree(plan_ma_comm_net_conf_t *cfg);

/**
 * Adds address of one node and returns its ID.
 */
int planMACommNetConfAddNode(plan_ma_comm_net_conf_t *cfg,
                             const char *addr);


/**
 * Creates a communication channel based on network connection (TCP).
 */
plan_ma_comm_t *planMACommNetNew(const plan_ma_comm_net_conf_t *cfg);
#endif /* PLAN_ZMQ */

#ifdef PLAN_NANOMSG
/**
 * Creates a communication channel between specified agent and all other
 * agents. The channel is local to the process, i.e., the agents must run
 * in separate threads.
 */
plan_ma_comm_t *planMACommInprocNew(int agent_id, int agent_size);

#endif /* PLAN_NANOMSG */


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
_bor_inline int planMACommSendToNode(plan_ma_comm_t *comm,
                                     int node_id,
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

/**
 * Destructor.
 */
typedef void (*plan_ma_comm_del_fn)(plan_ma_comm_t *comm);

/**
 * Sends the message to the specified node.
 * Returns 0 on success.
 */
typedef int (*plan_ma_comm_send_to_node_fn)(plan_ma_comm_t *comm,
                                            int node_id,
                                            const plan_ma_msg_t *msg);


/**
 * Receives a next message in non-blocking mode.
 * It is caller's responsibility to destroy the returned message.
 */
typedef plan_ma_msg_t *(*plan_ma_comm_recv_fn)(plan_ma_comm_t *comm);

/**
 * Receives a next message in blocking mode.
 * It is caller's responsibility to destroy the returned message.
 */
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

        if (comm->send_to_node_fn(comm, i, msg) != 0)
            return -1;
    }
    return 0;
}

_bor_inline int planMACommSendToNode(plan_ma_comm_t *comm,
                                     int node_id,
                                     const plan_ma_msg_t *msg)
{
    if (node_id == comm->node_id)
        return -1;
    return comm->send_to_node_fn(comm, node_id, msg);
}

_bor_inline int planMACommSendInRing(plan_ma_comm_t *comm,
                                     const plan_ma_msg_t *msg)
{
    int node_id;
    node_id = (comm->node_id + 1) % comm->node_size;
    return comm->send_to_node_fn(comm, node_id, msg);
}

_bor_inline plan_ma_msg_t *planMACommRecv(plan_ma_comm_t *comm)
{
    return comm->recv_fn(comm);
}

_bor_inline plan_ma_msg_t *planMACommRecvBlock(plan_ma_comm_t *comm,
                                               int timeout_in_ms)
{
    return comm->recv_block_fn(comm, timeout_in_ms);
}


/**** INTERNALS: ****/
void _planMACommInit(plan_ma_comm_t *comm,
                     int node_id,
                     int node_size,
                     plan_ma_comm_del_fn del_fn,
                     plan_ma_comm_send_to_node_fn send_to_node_fn,
                     plan_ma_comm_recv_fn recv_fn,
                     plan_ma_comm_recv_block_fn recv_block_fn);

void _planMACommFree(plan_ma_comm_t *comm);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_COMM_H__ */
