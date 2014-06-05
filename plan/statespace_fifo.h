#ifndef __PLAN_STATESPACE_FIFO_H__
#define __PLAN_STATESPACE_FIFO_H__

#include <boruvka/list.h>
#include <plan/statespace.h>

/**
 * FIFO State Space
 * =================
 */

struct _plan_state_space_fifo_node_t {
    plan_state_space_node_t node;
    bor_list_t fifo; /*!< Connector to the fifo queue */
};
typedef struct _plan_state_space_fifo_node_t plan_state_space_fifo_node_t;

struct _plan_state_space_fifo_t {
    plan_state_space_t state_space;
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
_bor_inline plan_state_space_fifo_node_t *planStateSpaceFifoNode(
            plan_state_space_fifo_t *, plan_state_id_t state_id);

/**
 * Returns next state waiting in fifo queue and change its state to closed.
 */
_bor_inline plan_state_space_fifo_node_t *planStateSpaceFifoPop(
            plan_state_space_fifo_t *);

/**
 * Opens the given node, i.e., push it into fifo queue.
 * Returns 0 on success, -1 if the node is already in open or closed
 * state.
 */
_bor_inline int planStateSpaceFifoOpen(plan_state_space_fifo_t *ss,
                                       plan_state_space_fifo_node_t *node);

/**
 * Opens a node correspodning to the state_id and fills it with provided
 * data. Returns the opened node or returns NULL if the node was already
 * opened or closed.
 */
_bor_inline plan_state_space_fifo_node_t *planStateSpaceFifoOpen2(
                plan_state_space_fifo_t *ss,
                plan_state_id_t state_id,
                plan_state_id_t parent_state_id,
                plan_operator_t *op,
                unsigned cost,
                unsigned heuristic);

/**
 * Change all open nodes to new nodes.
 */
_bor_inline void planStateSpaceFifoClear(plan_state_space_fifo_t *ss);

/**
 * Close all open nodes.
 */
_bor_inline void planStateSpaceFifoCloseAll(plan_state_space_fifo_t *ss);


_bor_inline int planStateSpaceFifoNodeIsNew2(plan_state_space_fifo_t *ss,
                                             plan_state_id_t state_id);

_bor_inline int planStateSpaceFifoNodeIsNew(const plan_state_space_fifo_node_t *n);
_bor_inline int planStateSpaceFifoNodeIsOpen(const plan_state_space_fifo_node_t *n);
_bor_inline int planStateSpaceFifoNodeIsClosed(const plan_state_space_fifo_node_t *n);


/**** INLINES ****/
_bor_inline plan_state_space_fifo_node_t *planStateSpaceFifoNode(
            plan_state_space_fifo_t *ss, plan_state_id_t state_id)
{
    plan_state_space_node_t *node;
    node = planStateSpaceNode(&ss->state_space, state_id);
    return bor_container_of(node, plan_state_space_fifo_node_t, node);
}

_bor_inline plan_state_space_fifo_node_t *planStateSpaceFifoPop(
            plan_state_space_fifo_t *ss)
{
    plan_state_space_node_t *node;
    node = planStateSpacePop(&ss->state_space);
    return bor_container_of(node, plan_state_space_fifo_node_t, node);
}

_bor_inline int planStateSpaceFifoOpen(plan_state_space_fifo_t *ss,
                                       plan_state_space_fifo_node_t *node)
{
    return planStateSpaceOpen(&ss->state_space, &node->node);
}

_bor_inline plan_state_space_fifo_node_t *planStateSpaceFifoOpen2(
                plan_state_space_fifo_t *ss,
                plan_state_id_t state_id,
                plan_state_id_t parent_state_id,
                plan_operator_t *op,
                unsigned cost,
                unsigned heuristic)
{
    plan_state_space_node_t *node;
    node = planStateSpaceOpen2(&ss->state_space, state_id, parent_state_id,
                               op, cost, heuristic);
    return bor_container_of(node, plan_state_space_fifo_node_t, node);
}

_bor_inline void planStateSpaceFifoClear(plan_state_space_fifo_t *ss)
{
    planStateSpaceClear(&ss->state_space);
}

_bor_inline void planStateSpaceFifoCloseAll(plan_state_space_fifo_t *ss)
{
    planStateSpaceCloseAll(&ss->state_space);
}

_bor_inline int planStateSpaceFifoNodeIsNew2(plan_state_space_fifo_t *ss,
                                             plan_state_id_t state_id)
{
    return planStateSpaceNodeIsNew2(&ss->state_space, state_id);
}

_bor_inline int planStateSpaceFifoNodeIsNew(const plan_state_space_fifo_node_t *n)
{
    return planStateSpaceNodeIsNew(&n->node);
}

_bor_inline int planStateSpaceFifoNodeIsOpen(const plan_state_space_fifo_node_t *n)
{
    return planStateSpaceNodeIsOpen(&n->node);
}

_bor_inline int planStateSpaceFifoNodeIsClosed(const plan_state_space_fifo_node_t *n)
{
    return planStateSpaceNodeIsClosed(&n->node);
}
#endif /* __PLAN_STATESPACE_H__ */
