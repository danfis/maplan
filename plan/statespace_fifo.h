#ifndef __PLAN_STATESPACE_FIFO_H__
#define __PLAN_STATESPACE_FIFO_H__

#include <boruvka/list.h>
#include <plan/state.h>
#include <plan/operator.h>

/**
 * FIFO State Space
 * =================
 */

#define PLAN_STATE_SPACE_NODE_NEW    0
#define PLAN_STATE_SPACE_NODE_OPEN   1
#define PLAN_STATE_SPACE_NODE_CLOSED 2

struct _plan_state_space_fifo_node_t {
    plan_state_id_t state_id;        /*!< ID of the corresponding state */
    plan_state_id_t parent_state_id; /*!< ID of the parent state */
    plan_operator_t *op;             /*!< Creating operator */

    int state;                       /*!< One of PLAN_STATE_SPACE_NODE_*
                                          states */
    bor_list_t fifo;                 /*!< Connector to the fifo queue */
};
typedef struct _plan_state_space_fifo_node_t plan_state_space_fifo_node_t;

struct _plan_state_space_fifo_t {
    plan_state_pool_t *state_pool;
    size_t data_id;
    bor_list_t fifo;
};
typedef struct _plan_state_space_fifo_t plan_state_space_fifo_t;

/**
 * Creates a new FIFO state space that uses provided state_pool.
 */
plan_state_space_fifo_t *planStateSpaceFifoNew(plan_state_pool_t *state_pool);

/**
 * Frees all allocated resources of the state space.
 */
void planStateSpaceFifoDel(plan_state_space_fifo_t *ss);

/**
 * Returns a node corresponding to the state ID.
 */
plan_state_space_fifo_node_t *planStateSpaceFifoNode(plan_state_space_fifo_t *,
                                                     plan_state_id_t state_id);

/**
 * Returns next state waiting in fifo queue and change its state to closed.
 */
plan_state_space_fifo_node_t *planStateSpaceFifoPop(plan_state_space_fifo_t *);

/**
 * Opens the given node, i.e., push it into fifo queue.
 * Returns 0 on success, -1 if the node is already in open or closed
 * state.
 */
int planStateSpaceFifoOpen(plan_state_space_fifo_t *ss,
                           plan_state_space_fifo_node_t *node);

/**
 * Change all open nodes to new nodes.
 */
void planStateSpaceFifoClear(plan_state_space_fifo_t *ss);

/**
 * Close all open nodes.
 */
void planStateSpaceFifoCloseAll(plan_state_space_fifo_t *ss);


_bor_inline int planStateSpaceFifoNodeIsNew(const plan_state_space_fifo_node_t *n);
_bor_inline int planStateSpaceFifoNodeIsOpen(const plan_state_space_fifo_node_t *n);
_bor_inline int planStateSpaceFifoNodeIsClosed(const plan_state_space_fifo_node_t *n);

_bor_inline int planStateSpaceFifoNodeIsNew(const plan_state_space_fifo_node_t *n)
{
    return n->state == PLAN_STATE_SPACE_NODE_NEW;
}

_bor_inline int planStateSpaceFifoNodeIsOpen(const plan_state_space_fifo_node_t *n)
{
    return n->state == PLAN_STATE_SPACE_NODE_OPEN;
}

_bor_inline int planStateSpaceFifoNodeIsClosed(const plan_state_space_fifo_node_t *n)
{
    return n->state == PLAN_STATE_SPACE_NODE_CLOSED;
}

#endif /* __PLAN_STATESPACE_H__ */
