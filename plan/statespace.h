#ifndef __PLAN_STATESPACE_H__
#define __PLAN_STATESPACE_H__

#include <boruvka/pairheap.h>

#include <plan/state.h>
#include <plan/operator.h>

#define PLAN_STATE_SPACE_NODE_NEW    0
#define PLAN_STATE_SPACE_NODE_OPEN   1
#define PLAN_STATE_SPACE_NODE_CLOSED 2

struct _plan_state_space_node_t {
    plan_state_id_t state_id;        /*!< ID of the corresponding state */
    plan_state_id_t parent_state_id; /*!< ID of the parent state */
    plan_operator_t *op;             /*!< Creating operator */
    unsigned cost;                   /*!< Cost of the path from the initial
                                          state to this state. */
    unsigned heuristic;              /*!< Value of the heuristic */

    /* private: */
    int state;                       /*!< One of PLAN_STATE_SPACE_NODE_*
                                          states */
};
typedef struct _plan_state_space_node_t plan_state_space_node_t;

struct _plan_state_space_t {
    plan_state_pool_t *state_pool;
    size_t data_id;
};
typedef struct _plan_state_space_t plan_state_space_t;

void planStateSpaceNodeInit(plan_state_space_node_t *n);

/**
 * Creates a state space.
 */
plan_state_space_t *planStateSpaceNew(plan_state_pool_t *state_pool);

/**
 * Free state space structure.
 */
void planStateSpaceDel(plan_state_space_t *ss);

/**
 * Returns a node corresponding to the state ID.
 */
plan_state_space_node_t *planStateSpaceNode(plan_state_space_t *,
                                            plan_state_id_t state_id);

/**
 * Opens the given node.
 * Returns 0 on success, -1 if the node is already in open or closed
 * state.
 */
int planStateSpaceOpen(plan_state_space_t *ss,
                       plan_state_space_node_t *node);

/**
 * Opens a node correspodning to the state_id and fills it with provided
 * data. Returns the opened node or returns NULL if the node was already
 * opened or closed.
 */
plan_state_space_node_t *planStateSpaceOpen2(plan_state_space_t *ss,
                                             plan_state_id_t state_id,
                                             plan_state_id_t parent_state_id,
                                             plan_operator_t *op,
                                             unsigned cost,
                                             unsigned heuristic);

/**
 * Reopens a closed node.
 * Returns 0 on success, -1 if the node wasn't in closed state.
 */
int planStateSpaceReopen(plan_state_space_t *ss,
                         plan_state_space_node_t *node);

/**
 * Sets properties of the node and reopens it.
 * Returns the node if successful or NULL if the node wasn't in closed
 * state.
 */
plan_state_space_node_t *planStateSpaceReopen2(plan_state_space_t *ss,
                                               plan_state_id_t state_id,
                                               plan_state_id_t parent_state_id,
                                               plan_operator_t *op,
                                               unsigned cost,
                                               unsigned heuristic);

/**
 * Closes open node.
 * Returns 0 on success, -1 if the node wasn't in open state.
 */
int planStateSpaceClose(plan_state_space_t *ss,
                        plan_state_space_node_t *node);

/**
 * Close node identified by its state_id.
 * Returns the node if successful or NULL if the node wasn't in open
 * state.
 */
plan_state_space_node_t *planStateSpaceClose2(plan_state_space_t *ss,
                                              plan_state_id_t state_id);


/**
 * Returns true if the node is in NEW state.
 */
_bor_inline int planStateSpaceNodeIsNew(const plan_state_space_node_t *n);

/**
 * Returns true if the node is in OPEN state.
 */
_bor_inline int planStateSpaceNodeIsOpen(const plan_state_space_node_t *n);

/**
 * Returns true if the node is in CLOSED state.
 */
_bor_inline int planStateSpaceNodeIsClosed(const plan_state_space_node_t *n);

/**
 * Returns true if the node corresponding to the given state id is in NEW
 * state.
 */
_bor_inline int planStateSpaceNodeIsNew2(plan_state_space_t *ss,
                                         plan_state_id_t state_id);

/**
 * Returns true if the node corresponding to the given state id is in OPEN
 * state.
 */
_bor_inline int planStateSpaceNodeIsOpen2(plan_state_space_t *ss,
                                          plan_state_id_t state_id);

/**
 * Returns true if the node corresponding to the given state id is in
 * CLOSED state.
 */
_bor_inline int planStateSpaceNodeIsClosed2(plan_state_space_t *ss,
                                            plan_state_id_t state_id);



/**** INLINES ****/
_bor_inline int planStateSpaceNodeIsNew(const plan_state_space_node_t *n)
{
    return n->state == PLAN_STATE_SPACE_NODE_NEW;
}

_bor_inline int planStateSpaceNodeIsOpen(const plan_state_space_node_t *n)
{
    return n->state == PLAN_STATE_SPACE_NODE_OPEN;
}

_bor_inline int planStateSpaceNodeIsClosed(const plan_state_space_node_t *n)
{
    return n->state == PLAN_STATE_SPACE_NODE_CLOSED;
}

_bor_inline int planStateSpaceNodeIsNew2(plan_state_space_t *ss,
                                             plan_state_id_t state_id)
{
    plan_state_space_node_t *n;
    n = planStateSpaceNode(ss, state_id);
    return planStateSpaceNodeIsNew(n);
}

_bor_inline int planStateSpaceNodeIsOpen2(plan_state_space_t *ss,
                                              plan_state_id_t state_id)
{
    plan_state_space_node_t *n;
    n = planStateSpaceNode(ss, state_id);
    return planStateSpaceNodeIsOpen(n);
}

_bor_inline int planStateSpaceNodeIsClosed2(plan_state_space_t *ss,
                                                plan_state_id_t state_id)
{
    plan_state_space_node_t *n;
    n = planStateSpaceNode(ss, state_id);
    return planStateSpaceNodeIsClosed(n);
}

#endif /* __PLAN_STATESPACE_H__ */
