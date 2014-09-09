#ifndef __PLAN_MA_COMM_NET_H__
#define __PLAN_MA_COMM_NET_H__

#include <plan/ma_msg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


struct _plan_ma_comm_net_conf_t {
    int node_id;   /*!< Id of the current node */
    char **addr;   /*!< TCP address of all nodes in cluster */
    int node_size; /*!< Number of nodes in cluster */
};
typedef struct _plan_ma_comm_net_conf_t plan_ma_comm_net_conf_t;

struct _plan_ma_comm_net_t {
    int node_id;
    int node_size;
    void *context;
    void *readsock;
    void **writesock;
};
typedef struct _plan_ma_comm_net_t plan_ma_comm_net_t;


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


plan_ma_comm_net_t *planMACommNetNew(const plan_ma_comm_net_conf_t *cfg);
void planMACommNetDel(plan_ma_comm_net_t *comm);


/**
 * Returns ID of the node.
 */
_bor_inline int planMACommNetId(const plan_ma_comm_net_t *comm);

/**
 * Sends the message to all nodes.
 * Returns 0 on success.
 */
int planMACommNetSendToAll(plan_ma_comm_net_t *comm,
                           const plan_ma_msg_t *msg);

/**
 * Sends the message to the specified node.
 * Returns 0 on success.
 */
int planMACommNetSendToNode(plan_ma_comm_net_t *comm,
                            int node_id,
                            const plan_ma_msg_t *msg);

/**
 * Sends the message to the next node in ring.
 * Returns 0 on success.
 */
int planMACommNetSendInRing(plan_ma_comm_net_t *comm,
                            const plan_ma_msg_t *msg);

/**
 * Receives a next message in non-blocking mode.
 * It is caller's responsibility to destroy the returned message.
 */
plan_ma_msg_t *planMACommNetRecv(plan_ma_comm_net_t *comm);

/**
 * Receives a next message in blocking mode.
 * It is caller's responsibility to destroy the returned message.
 */
plan_ma_msg_t *planMACommNetRecvBlock(plan_ma_comm_net_t *comm);

/**
 * Same as planMACommNetRecvBlock() but unblocks after specified amount
 * of time if no message was received (and that case returns NULL).
 */
plan_ma_msg_t *planMACommNetRecvBlockTimeout(plan_ma_comm_net_t *comm,
                                             int timeout_in_ms);



/**** INLINES: ****/
_bor_inline int planMACommNetId(const plan_ma_comm_net_t *comm)
{
    return comm->node_id;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_COMM_NET_H__ */
