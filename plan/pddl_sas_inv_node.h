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

#ifndef __PLAN_PDDL_SAS_INV_NODE_H__
#define __PLAN_PDDL_SAS_INV_NODE_H__

#include <plan/pddl_sas_inv_fact.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_sas_inv_edge_t {
    int *node;
    int size;
    int alloc;
};
typedef struct _plan_pddl_sas_inv_edge_t plan_pddl_sas_inv_edge_t;

struct _plan_pddl_sas_inv_edges_t {
    plan_pddl_sas_inv_edge_t *edge;
    int edge_size;
    int edge_alloc;
};
typedef struct _plan_pddl_sas_inv_edges_t plan_pddl_sas_inv_edges_t;

struct _plan_pddl_sas_inv_node_t {
    int id;
    int repr;         /*!< Representative node -- node that represents the
                           same invariant */
    int new_id;       /*!< Assigned node ID in the new graph */
    int node_size;    /*!< Number of all nodes */
    int fact_size;    /*!< Number of all facts */
    int *inv;         /*!< True for facts that are invariant candidates */
    int *conflict;    /*!< True for the node that is in conflict */
    int *must;        /*!< True for the node that must be in invariant with
                           this node. */

    // TODO: Separate structure
    int *edge_presence; /*!< Number of edges in which this node is present. */
    int sum_edge_presence; /*!< Number of all edges this node is present in. */

    plan_pddl_sas_inv_edges_t edges;
};
typedef struct _plan_pddl_sas_inv_node_t plan_pddl_sas_inv_node_t;

struct _plan_pddl_sas_inv_nodes_t {
    plan_pddl_sas_inv_node_t *node;
    int node_size;
    int fact_size;
};
typedef struct _plan_pddl_sas_inv_nodes_t plan_pddl_sas_inv_nodes_t;

/**
 * Initialize list of nodes from the list of facts.
 */
void planPDDLSasInvNodesInitFromFacts(plan_pddl_sas_inv_nodes_t *nodes,
                                      const plan_pddl_sas_inv_facts_t *fs);

/**
 * Frees allocated memory.
 */
void planPDDLSasInvNodesFree(plan_pddl_sas_inv_nodes_t *nodes);

/**
 * Reinitializes nodes so that:
 *     1. nodes that cannot form an invariant are removed,
 *     2. each node gathers edges from all nodes in its .must[]
 *     3. duplicates are removed
 *     4. TODO: create invariants from nodes that have no edges and that
 *              are not in any edge of any other node.
 *        TODO: callback?
 * Remove nodes that cannot for an invariant anymore.
 */
void planPDDLSasInvNodesReinit(plan_pddl_sas_inv_nodes_t *ns);

/**
 * Split nodes and create a new strucutre.
 */
void planPDDLSasInvNodesSplit(plan_pddl_sas_inv_nodes_t *ns);

/**
 * Returns number of facts in the .inv[].
 */
int planPDDLSasInvNodeInvSize(const plan_pddl_sas_inv_node_t *node);

/**
 * Sort and unique edges in the node.
 */
void planPDDLSasInvNodeUniqueEdges(plan_pddl_sas_inv_node_t *node);

/**
 * Removes covered edges and returns 1 if any edge was removed.
 */
int planPDDLSasInvNodeRemoveCoveredEdges(plan_pddl_sas_inv_node_t *node);

/**
 * Removes conflict nodes from node's edges.
 * If an edge becomes empty a special conflict-node is added to the .must[].
 * Returns 1 on change.
 */
int planPDDLSasInvNodeRemoveConflictNodesFromEdges(plan_pddl_sas_inv_node_t *node);

/**
 * Add nodes from the edges that have only one node to the .must[].
 */
int planPDDLSasInvNodeAddMustFromSingleEdges(plan_pddl_sas_inv_node_t *node);

/**
 * Prune super-edges with their sub-edges.
 */
int planPDDLSasInvNodePruneSuperEdges(plan_pddl_sas_inv_node_t *node,
                                      plan_pddl_sas_inv_nodes_t *ns);

/**
 * Prune super-edges with the node.
 */
int planPDDLSasInvNodePruneSuperEdges(plan_pddl_sas_inv_node_t *node,
                                      plan_pddl_sas_inv_nodes_t *ns);

/**
 * Adds all nodes from .must[] of all nodes that are already in
 * node.must[].
 */
int planPDDLSasInvNodeAddMustsOfMusts(plan_pddl_sas_inv_node_t *node,
                                      const plan_pddl_sas_inv_nodes_t *ns);

/**
 * Merges conflicts from nodes that are in .must[].
 */
int planPDDLSasInvNodeAddConflictsOfMusts(plan_pddl_sas_inv_node_t *node,
                                          plan_pddl_sas_inv_nodes_t *ns);

void planPDDLSasInvNodePrint(const plan_pddl_sas_inv_node_t *node, FILE *fout);
void planPDDLSasInvNodesPrint(const plan_pddl_sas_inv_nodes_t *ns, FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_SAS_INV_NODE_H__ */
