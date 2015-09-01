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

#include "plan/search.h"
#include "plan/list.h"

struct _plan_search_astar_t {
    plan_search_t search;

    plan_list_t *list; /*!< Open-list */
    int pathmax;       /*!< Use pathmax correction */
};
typedef struct _plan_search_astar_t plan_search_astar_t;

#define SEARCH_FROM_PARENT(parent) \
    bor_container_of((parent), plan_search_astar_t, search)

/** Frees allocated resorces */
static void planSearchAStarDel(plan_search_t *_search);
/** Initializes search. This must be call exactly once. */
static int planSearchAStarInit(plan_search_t *_search);
/** Performes one step in the algorithm. */
static int planSearchAStarStep(plan_search_t *_search);
/** Inserts node into open-list */
static void planSearchAStarInsertNode(plan_search_t *search,
                                      plan_state_space_node_t *node);
static plan_cost_t planSearchAStarTopNodeCost(const plan_search_t *search);


void planSearchAStarParamsInit(plan_search_astar_params_t *p)
{
    bzero(p, sizeof(*p));
    planSearchParamsInit(&p->search);
}

plan_search_t *planSearchAStarNew(const plan_search_astar_params_t *params)
{
    plan_search_astar_t *astar;

    astar = BOR_ALLOC(plan_search_astar_t);

    _planSearchInit(&astar->search, &params->search,
                    planSearchAStarDel,
                    planSearchAStarInit,
                    planSearchAStarStep,
                    planSearchAStarInsertNode,
                    planSearchAStarTopNodeCost);

    astar->list     = planListTieBreaking(2);
    astar->pathmax  = params->pathmax;

    return &astar->search;
}

static void planSearchAStarDel(plan_search_t *search)
{
    plan_search_astar_t *astar = SEARCH_FROM_PARENT(search);

    _planSearchFree(search);
    if (astar->list)
        planListDel(astar->list);
    BOR_FREE(astar);
}

static int astarInsertState(plan_search_astar_t *astar,
                            plan_state_space_node_t *node,
                            plan_op_t *op,
                            plan_state_space_node_t *parent_node)
{
    plan_cost_t cost[2];
    plan_cost_t heur, g_cost = 0;
    plan_state_id_t parent_state_id = PLAN_NO_STATE;
    plan_search_t *search = &astar->search;
    int res;

    if (parent_node){
        g_cost += parent_node->cost;
        parent_state_id = parent_node->state_id;
    }

    if (op)
        g_cost += op->cost;

    node->parent_state_id = parent_state_id;
    node->op              = op;
    node->cost            = g_cost;

    // Force to open the node and compute heuristic if necessary
    if (planStateSpaceNodeIsNew(node)){
        planStateSpaceOpen(search->state_space, node);

        // TODO: handle re-computing heuristic and max() of heuristics
        // etc...
        res = _planSearchHeur(search, node, &heur, NULL);
        if (res != PLAN_SEARCH_CONT)
            return res;

        if (astar->pathmax && op != NULL && parent_node != NULL){
            heur = BOR_MAX(heur, parent_node->heuristic - op->cost);
        }

    }else{
        if (planStateSpaceNodeIsClosed(node)){
            planStateSpaceReopen(search->state_space, node);
        }

        // The node was already opened, so we have already computed
        // heuristic
        heur = node->heuristic;
    }

    // Set heuristic value to the node
    node->heuristic = heur;

    // Skip dead-end states
    if (heur == PLAN_HEUR_DEAD_END)
        return PLAN_SEARCH_CONT;

    // Set up costs for open-list -- ties are broken by heuristic value
    heur = BOR_MAX(heur, 0);
    cost[0] = g_cost + heur; // f-value: f() = g() + h()
    cost[1] = heur; // tie-breaking value

    // Insert into open-list
    planListPush(astar->list, cost, node->state_id);
    planSearchStatIncGeneratedStates(&search->stat);

    return PLAN_SEARCH_CONT;
}

static int planSearchAStarInit(plan_search_t *search)
{
    plan_search_astar_t *astar = SEARCH_FROM_PARENT(search);
    plan_state_space_node_t *node;

    node = planStateSpaceNode(search->state_space, search->initial_state);
    return astarInsertState(astar, node, NULL, NULL);
}

static int planSearchAStarStep(plan_search_t *search)
{
    plan_search_astar_t *astar = SEARCH_FROM_PARENT(search);
    plan_cost_t cost[2], g_cost;
    plan_state_id_t cur_state, next_state;
    plan_state_space_node_t *cur_node, *next_node;
    int i, op_size, res;
    plan_op_t **op;

    // Get next state from open list
    if (planListPop(astar->list, &cur_state, cost) != 0)
        return PLAN_SEARCH_NOT_FOUND;

    // Get corresponding state space node
    cur_node = planStateSpaceNode(search->state_space, cur_state);

    // Skip already closed nodes
    if (!planStateSpaceNodeIsOpen(cur_node))
        return PLAN_SEARCH_CONT;

    // Close the current state node
    planStateSpaceClose(search->state_space, cur_node);

    // Check whether it is a goal
    if (_planSearchCheckGoal(search, cur_node))
        return PLAN_SEARCH_FOUND;

    // Find all applicable operators
    _planSearchFindApplicableOps(search, cur_state);
    planSearchStatIncExpandedStates(&search->stat);
    _planSearchExpandedNode(search, cur_node);

    // Add states created by applicable operators
    op      = search->app_ops.op;
    op_size = search->app_ops.op_found;
    for (i = 0; i < op_size; ++i){
        // Create a new state
        next_state = planOpApply(op[i], search->state_pool, cur_state);
        // Compute its g() value
        g_cost = cur_node->cost + op[i]->cost;

        // Obtain corresponding node from state space
        next_node = planStateSpaceNode(search->state_space, next_state);

        // Decide whether to insert the state into open-list
        if (planStateSpaceNodeIsNew(next_node)
                || next_node->cost > g_cost){
            res = astarInsertState(astar, next_node, op[i], cur_node);
            if (res != PLAN_SEARCH_CONT)
                return res;
        }
    }
    return PLAN_SEARCH_CONT;
}

static void planSearchAStarInsertNode(plan_search_t *search,
                                      plan_state_space_node_t *node)
{
    plan_search_astar_t *astar = SEARCH_FROM_PARENT(search);
    plan_cost_t heur, cost[2];

    heur = BOR_MAX(node->heuristic, 0);
    cost[0] = node->cost + heur;
    cost[1] = heur;

    if (planStateSpaceNodeIsNew(node)){
        planStateSpaceOpen(search->state_space, node);
    }else{
        planStateSpaceReopen(search->state_space, node);
    }
    planListPush(astar->list, cost, node->state_id);
}

static plan_cost_t planSearchAStarTopNodeCost(const plan_search_t *search)
{
    plan_search_astar_t *astar = SEARCH_FROM_PARENT(search);
    plan_state_id_t state_id;
    plan_cost_t cost[2];

    if (planListTop(astar->list, &state_id, cost) == 0)
        return cost[0] - cost[1];
    return PLAN_COST_MAX;
}
