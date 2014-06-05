#ifndef __PLAN_STATESPACE_H__
#define __PLAN_STATESPACE_H__

#include <boruvka/pairheap.h>

#include <plan/state.h>
#include <plan/operator.h>

#define PLAN_NODE_STATE_NEW    0
#define PLAN_NODE_STATE_OPEN   1
#define PLAN_NODE_STATE_CLOSED 2

struct _plan_state_space_node_t {
    plan_state_id_t state_id;        /*!< ID of the corresponding state */
    plan_state_id_t parent_state_id; /*!< ID of the parent state */
    int state;                       /*!< One of PLAN_NODE_STATE_* values */
    plan_operator_t *op;             /*!< Creating operator */
    bor_pairheap_node_t heap;        /*!< Connector to an open list */
    unsigned heuristic;              /*!< Value of a heuristic */
    unsigned cost;                   /*!< Cost of the path to this state */
};
typedef struct _plan_state_space_node_t plan_state_space_node_t;

struct _plan_state_space_t {
    bor_pairheap_t *heap;
    plan_state_pool_t *state_pool;
    size_t data_id;
};
typedef struct _plan_state_space_t plan_state_space_t;

/**
 * Creates a new state space that uses provided state_pool.
 */
plan_state_space_t *planStateSpaceNew(plan_state_pool_t *state_pool);

/**
 * Frees all allocated resources of the state space.
 */
void planStateSpaceDel(plan_state_space_t *ss);

/**
 * This function finds the node with minimal heuristic value from all open
 * nodes, close that node and returns it.
 * In case there are no more open nodes, NULL is returned.
 */
plan_state_space_node_t *planStateSpaceExtractMin(plan_state_space_t *ss);

/**
 * Open the state space node.
 * Returns opened node if the node wasn't opened or closed before.
 * If the node was already opened or closed NULL is returned and the node
 * is not touched.
 */
plan_state_space_node_t *planStateSpaceOpenNode(plan_state_space_t *ss,
                                                plan_state_id_t sid,
                                                plan_state_id_t parent_sid,
                                                plan_operator_t *op,
                                                unsigned heuristic);

/**
 * Reopens the node corresponding to the given state. The node must be
 * closed or this function fails returning NULL.
 */
plan_state_space_node_t *planStateSpaceReopenNode(plan_state_space_t *ss,
                                                  plan_state_id_t sid,
                                                  plan_state_id_t parent_sid,
                                                  plan_operator_t *op,
                                                  unsigned heuristic);

/**
 * Close opened state space node. If the node isn't close this function
 * fails returning NULL.
 */
plan_state_space_node_t *planStateSpaceCloseNode(plan_state_space_t *ss,
                                                 plan_state_id_t sid);

/**
 * Returns node corresponding to the state ID.
 */
plan_state_space_node_t *planStateSpaceNode(plan_state_space_t *ss,
                                            plan_state_id_t sid);

/**
 * Change all open nodes to the NEW state.
 */
void planStateSpaceClearOpenNodes(plan_state_space_t *ss);

/**
 * Returns a value heuristic of the state space node.
 */
_bor_inline unsigned planStateSpaceNodeHeuristic(const plan_state_space_node_t *n);

/**
 * Returns corresponding state id.
 */
_bor_inline plan_state_id_t planStateSpaceNodeStateId(
                const plan_state_space_node_t *n);

/**
 * Returns corresponding parent state id.
 */
_bor_inline plan_state_id_t planStateSpaceNodeParentStateId(
                const plan_state_space_node_t *n);

/**
 * Returns name of the creating operator.
 */
_bor_inline const char *planStateSpaceNodeOpName(
                const plan_state_space_node_t *n);

/**
 * Returns true is the node is new.
 */
_bor_inline int planStateSpaceNodeIsNew(const plan_state_space_node_t *n);

/**
 * Returns true is the node is open.
 */
_bor_inline int planStateSpaceNodeIsOpen(const plan_state_space_node_t *n);

/**
 * Returns true is the node is closed.
 */
_bor_inline int planStateSpaceNodeIsClosed(const plan_state_space_node_t *n);

/**
 * Returns true is the node corresponding to the state ID is new.
 */
_bor_inline int planStateSpaceNodeIsNew2(plan_state_space_t *ss,
                                         plan_state_id_t sid);

/**
 * Returns true is the node corresponding to the state ID is open.
 */
_bor_inline int planStateSpaceNodeIsOpen2(plan_state_space_t *ss,
                                          plan_state_id_t sid);

/**
 * Returns true is the node corresponding to the state ID is closed.
 */
_bor_inline int planStateSpaceNodeIsClosed2(plan_state_space_t *ss,
                                            plan_state_id_t sid);


/**** INLINES ****/
_bor_inline unsigned planStateSpaceNodeHeuristic(const plan_state_space_node_t *n)
{
    return n->heuristic;
}

_bor_inline plan_state_id_t planStateSpaceNodeStateId(
                const plan_state_space_node_t *n)
{
    return n->state_id;
}

_bor_inline plan_state_id_t planStateSpaceNodeParentStateId(
                const plan_state_space_node_t *n)
{
    return n->parent_state_id;
}

_bor_inline const char *planStateSpaceNodeOpName(
                const plan_state_space_node_t *n)
{
    if (!n->op)
        return NULL;
    return n->op->name;
}

_bor_inline int planStateSpaceNodeIsNew(const plan_state_space_node_t *n)
{
    return n->state == PLAN_NODE_STATE_NEW;
}

_bor_inline int planStateSpaceNodeIsOpen(const plan_state_space_node_t *n)
{
    return n->state == PLAN_NODE_STATE_OPEN;
}

_bor_inline int planStateSpaceNodeIsClosed(const plan_state_space_node_t *n)
{
    return n->state == PLAN_NODE_STATE_CLOSED;
}

_bor_inline int planStateSpaceNodeIsNew2(plan_state_space_t *ss,
                                         plan_state_id_t sid)
{
    plan_state_space_node_t *node;
    node = planStateSpaceNode(ss, sid);
    return planStateSpaceNodeIsNew(node);
}

_bor_inline int planStateSpaceNodeIsOpen2(plan_state_space_t *ss,
                                          plan_state_id_t sid)
{
    plan_state_space_node_t *node;
    node = planStateSpaceNode(ss, sid);
    return planStateSpaceNodeIsOpen(node);
}

_bor_inline int planStateSpaceNodeIsClosed2(plan_state_space_t *ss,
                                            plan_state_id_t sid)
{
    plan_state_space_node_t *node;
    node = planStateSpaceNode(ss, sid);
    return planStateSpaceNodeIsClosed(node);
}

#endif /* __PLAN_STATESPACE_H__ */
