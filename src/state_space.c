/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#include <boruvka/alloc.h>

#include "plan/state_space.h"

void planStateSpaceNodeInit(plan_state_space_node_t *n)
{
    n->state_id        = PLAN_NO_STATE;
    n->parent_state_id = PLAN_NO_STATE;
    n->state           = PLAN_STATE_SPACE_NODE_NEW;
    n->op              = NULL;
    n->cost            = -1;
    n->heuristic       = -1;
}

plan_state_space_t *planStateSpaceNew(plan_state_pool_t *state_pool)
{
    plan_state_space_t *ss;
    plan_state_space_node_t nodeinit;

    ss = BOR_ALLOC(plan_state_space_t);

    ss->state_pool = state_pool;

    nodeinit.state_id        = PLAN_NO_STATE;
    nodeinit.parent_state_id = PLAN_NO_STATE;
    nodeinit.op              = NULL;
    nodeinit.cost            = -1;
    nodeinit.heuristic       = -1;
    nodeinit.state           = PLAN_STATE_SPACE_NODE_NEW;
    ss->data_id = planStatePoolDataReserve(state_pool,
                                           sizeof(plan_state_space_node_t),
                                           NULL, &nodeinit);

    return ss;
}

void planStateSpaceDel(plan_state_space_t *ss)
{
    BOR_FREE(ss);
}

plan_state_space_node_t *planStateSpaceNode(plan_state_space_t *ss,
                                            plan_state_id_t state_id)
{
    plan_state_space_node_t *n;
    n = planStatePoolData(ss->state_pool, ss->data_id, state_id);
    n->state_id = state_id;
    return n;
}

int planStateSpaceOpen(plan_state_space_t *ss,
                       plan_state_space_node_t *node)
{
    if (!planStateSpaceNodeIsNew(node))
        return -1;

    node->state = PLAN_STATE_SPACE_NODE_OPEN;
    return 0;
}

plan_state_space_node_t *planStateSpaceOpen2(plan_state_space_t *ss,
                                             plan_state_id_t state_id,
                                             plan_state_id_t parent_state_id,
                                             plan_op_t *op,
                                             plan_cost_t cost,
                                             plan_cost_t heuristic)
{
    plan_state_space_node_t *node;

    node = planStateSpaceNode(ss, state_id);
    if (!planStateSpaceNodeIsNew(node))
        return NULL;

    node->parent_state_id = parent_state_id;
    node->op              = op;
    node->cost            = cost;
    node->heuristic       = heuristic;

    planStateSpaceOpen(ss, node);

    return node;
}

int planStateSpaceReopen(plan_state_space_t *ss,
                         plan_state_space_node_t *node)
{
    if (!planStateSpaceNodeIsClosed(node))
        return -1;

    node->state = PLAN_STATE_SPACE_NODE_NEW;

    return planStateSpaceOpen(ss, node);
}

plan_state_space_node_t *planStateSpaceReopen2(plan_state_space_t *ss,
                                               plan_state_id_t state_id,
                                               plan_state_id_t parent_state_id,
                                               plan_op_t *op,
                                               plan_cost_t cost,
                                               plan_cost_t heuristic)
{
    plan_state_space_node_t *node;

    node = planStateSpaceNode(ss, state_id);
    if (!planStateSpaceNodeIsClosed(node))
        return NULL;

    node->state = PLAN_STATE_SPACE_NODE_NEW;

    return planStateSpaceReopen2(ss, state_id, parent_state_id, op,
                                 cost, heuristic);
}

int planStateSpaceClose(plan_state_space_t *ss,
                         plan_state_space_node_t *node)
{
    if (!planStateSpaceNodeIsOpen(node))
        return -1;

    node->state = PLAN_STATE_SPACE_NODE_CLOSED;
    return 0;
}

plan_state_space_node_t *planStateSpaceClose2(plan_state_space_t *ss,
                                              plan_state_id_t state_id)
{
    plan_state_space_node_t *node;

    node = planStateSpaceNode(ss, state_id);
    if (!planStateSpaceNodeIsOpen(node))
        return NULL;

    node->state = PLAN_STATE_SPACE_NODE_CLOSED;
    return node;
}
