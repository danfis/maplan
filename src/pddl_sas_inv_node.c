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
#include "plan/pddl_sas_inv_node.h"

#define CONFLICT_NODE_ID 0

/** Initializes a single edge. */
static void edgeInit(plan_pddl_sas_inv_edge_t *e,
                     const int *ids, int size);
/** Frees allocated resources. */
static void edgeFree(plan_pddl_sas_inv_edge_t *e);
/** Sort and unique IDs stored in the edge. */
static void edgeUnique(plan_pddl_sas_inv_edge_t *edge);
/** Returns true if node_id is present in the edge. */
static int edgeHasNode(const plan_pddl_sas_inv_edge_t *e, int node_id);
/** Returns true if the edge is covered by any node from ns_bool[]. */
static int edgeIsCovered(const plan_pddl_sas_inv_edge_t *edge,
                         const int *ns_bool);
/** Returns true if e1 is subset of e2. */
static int edgeIsSubset(const plan_pddl_sas_inv_edge_t *e1,
                        const plan_pddl_sas_inv_edge_t *e2);
/** Removes nodes from the edge. Returns 1 on change. */
static int edgeRemoveNodes(plan_pddl_sas_inv_edge_t *e, const int *ns);

/** Initializes a list of edges. */
static void edgesInit(plan_pddl_sas_inv_edges_t *es);
/** Frees allocated resources. */
static void edgesFree(plan_pddl_sas_inv_edges_t *es);
/** Returns true if any of the edges has the specified node. */
static int edgesHaveNode(const plan_pddl_sas_inv_edges_t *es, int node_id);
/** Adds a new edge constructed from a list of facts. */
static void edgesAddFromFacts(plan_pddl_sas_inv_edges_t *es,
                              const plan_pddl_ground_facts_t *fs);
/** Adds a new edge, returns an ID of a new edge. */
static int edgesAdd(plan_pddl_sas_inv_edges_t *es,
                    const plan_pddl_sas_inv_edge_t *e);
/** Adds a new edge into es that have re-mapped node IDs to new IDs. */
static int edgesAddMapped(plan_pddl_sas_inv_edges_t *es,
                          const plan_pddl_sas_inv_edge_t *e,
                          const plan_pddl_sas_inv_nodes_t *old);
/** Sort and unique all edges in the list. */
static void edgesUnique(plan_pddl_sas_inv_edges_t *es);
/** Removes covered edges and returns 1 if any edge was removed. */
static int edgesRemoveCoveredEdges(plan_pddl_sas_inv_edges_t *es,
                                   const int *ns);
/** Removes empty edges from the list of edges. */
static void edgesRemoveEmptyEdges(plan_pddl_sas_inv_edges_t *es);
/** Add conflicts between node and the nodes that are in super but not in
 *  sub. */
static void edgeAddConflictsFromSuperEdge(const plan_pddl_sas_inv_edge_t *sub,
                                          const plan_pddl_sas_inv_edge_t *super,
                                          plan_pddl_sas_inv_node_t *node,
                                          plan_pddl_sas_inv_nodes_t *ns);

/** Adds node to the edge if not already there. */
static void edgeAddNode(plan_pddl_sas_inv_edge_t *e, int node_id);
static void edgeAddAlternativeNode(plan_pddl_sas_inv_edge_t *e,
                                   int test_id, int add_id);
static void edgesAddAlternativeNode(plan_pddl_sas_inv_edges_t *es,
                                    int test_id, int add_id);
/** Adds nodes if test_id already in e. */
static void edgeAddAlternativeNodes(plan_pddl_sas_inv_edge_t *e,
                                    int test_id,
                                    const int *add_bool, int add_size);
static void edgesAddAlternativeNodes(plan_pddl_sas_inv_edges_t *es,
                                     int test_id,
                                     const int *add_bool, int add_size);


/** Initializes a single node. */
static void nodeInit(plan_pddl_sas_inv_node_t *node,
                     int id, int node_size, int fact_size);
/** Frees allocated resources. */
static void nodeFree(plan_pddl_sas_inv_node_t *node);
/** Copy .inv, .conflict and edges from src to dst. */
static void nodeCopyInvConflictEdges(plan_pddl_sas_inv_node_t *dst,
                                    const plan_pddl_sas_inv_node_t *src);
/** Copies node from old to dst and re-maps all IDs. */
static void nodeCopyMapped(plan_pddl_sas_inv_node_t *dst,
                           const plan_pddl_sas_inv_node_t *old,
                           const plan_pddl_sas_inv_nodes_t *old_ns);
/** Returns true if nodes' .inv[] equal */
static int nodeInvEq(const plan_pddl_sas_inv_node_t *n1,
                     const plan_pddl_sas_inv_node_t *n2);
/** Returns true if n1.inv is subset of n2.inv */
static int nodeInvSubset(const plan_pddl_sas_inv_node_t *n1,
                         const plan_pddl_sas_inv_node_t *n2);
/** Adds edges from src to dst. */
static void nodeAddEdgesFromOtherNode(plan_pddl_sas_inv_node_t *dst,
                                      const plan_pddl_sas_inv_node_t *src,
                                      plan_pddl_sas_inv_nodes_t *ns);
/** Add node_id into must and if it is also in .conflict[] add
 *  CONFLICT_NODE_ID to .must[] too. */
static int nodeAddMust(plan_pddl_sas_inv_node_t *node, int node_id);
/** Adds conflict between two nodes. */
static int nodeAddConflict(plan_pddl_sas_inv_node_t *node,
                           plan_pddl_sas_inv_node_t *node2);

/** Initializes empty set of nodes. */
static void nodesInit(plan_pddl_sas_inv_nodes_t *ns,
                      int node_size, int fact_size);
/** Gather all edges to each node from node's .must[]. */
static void nodesGatherEdges(plan_pddl_sas_inv_nodes_t *ns);
/** Sets .inv[] array according to the .must[]. */
static int nodesSetInv(plan_pddl_sas_inv_nodes_t *ns);
/** Deduplicates nodes. */
static void nodesSetRepr(plan_pddl_sas_inv_nodes_t *ns);
/** Assignes each node a new ID according to its .repr and returns number
 *  of new nodes. */
static int nodesSetNewId(plan_pddl_sas_inv_nodes_t *ns);
/** Finds best split node -- heuristic. */
static int nodesFindSplit(const plan_pddl_sas_inv_nodes_t *ns);
static void nodesPruneEdgesFromSupersets(plan_pddl_sas_inv_nodes_t *ns);

void planPDDLSasInvNodesInitFromFacts(plan_pddl_sas_inv_nodes_t *ns,
                                      const plan_pddl_sas_inv_facts_t *fs)
{
    plan_pddl_sas_inv_node_t *node;
    const plan_pddl_sas_inv_fact_t *fact;
    int i, j, node_size;

    // +1 for CONFLICT_NODE_ID
    node_size = fs->fact_size + 1;
    nodesInit(ns, node_size, fs->fact_size);
    for (i = 0; i < fs->fact_size; ++i){
        node = ns->node + i + 1;
        fact = fs->fact + i;
        memcpy(node->conflict + 1, fact->conflict, sizeof(int) * fs->fact_size);
        node->inv[i] = 1;

        for (j = 0; j < fact->edge_size; ++j)
            edgesAddFromFacts(&node->edges, fact->edge + j);
        edgesUnique(&node->edges);
    }
}

void planPDDLSasInvNodesFree(plan_pddl_sas_inv_nodes_t *ns)
{
    int i;

    for (i = 0; i < ns->node_size; ++i)
        nodeFree(ns->node + i);
    BOR_FREE(ns->node);
}

void planPDDLSasInvNodesReinit(plan_pddl_sas_inv_nodes_t *ns)
{
    plan_pddl_sas_inv_nodes_t old_ns;
    int i, new_node_size;

    nodesGatherEdges(ns);
    nodesSetInv(ns);
    nodesSetRepr(ns);
    new_node_size = nodesSetNewId(ns);

    old_ns = *ns;
    nodesInit(ns, new_node_size, old_ns.fact_size);
    for (i = 0; i < old_ns.node_size; ++i){
        if (old_ns.node[i].new_id >= 0)
            nodeCopyMapped(ns->node + old_ns.node[i].new_id,
                           old_ns.node + i, &old_ns);
    }

    nodesPruneEdgesFromSupersets(ns);

    planPDDLSasInvNodesFree(&old_ns);
}

int planPDDLSasInvNodesSplit(plan_pddl_sas_inv_nodes_t *ns)
{
    plan_pddl_sas_inv_nodes_t old_ns;
    int i, j, split_node, node_size;
    int *alt;

    // Find the node we use for splitting.
    split_node = nodesFindSplit(ns);
    if (split_node == -1)
        return -1;

    // Compute number of nodes in the new set and set alternative node ID
    alt = BOR_ALLOC_ARR(int, ns->node_size);
    node_size = ns->node_size;
    for (i = 0; i < ns->node_size; ++i){
        alt[i] = -1;
        if (edgesHaveNode(&ns->node[i].edges, split_node))
            alt[i] = node_size++;
    }

    // Create a new nodes and preserve the old ones for a while
    old_ns = *ns;
    nodesInit(ns, node_size, old_ns.fact_size);
    for (i = 1; i < old_ns.node_size; ++i)
        nodeCopyInvConflictEdges(ns->node + i, old_ns.node + i);

    // Add alternative node IDs into edges and into conflict
    for (i = 0; i < old_ns.node_size; ++i){
        for (j = 0; j < old_ns.node_size; ++j){
            if (alt[j] >= 0)
                edgesAddAlternativeNode(&ns->node[i].edges, j, alt[j]);
        }
    }

    // Initialize new nodes and update the old ones.
    for (i = 0; i < old_ns.node_size; ++i){
        if (alt[i] >= 0){
            nodeCopyInvConflictEdges(ns->node + alt[i], old_ns.node + i);
            nodeAddConflict(ns->node + alt[i], ns->node + split_node);
            nodeAddMust(ns->node + i, split_node);
        }
    }

    // Add alternative nodes into conflicts
    for (i = 0; i < ns->node_size; ++i){
        for (j = 0; j < old_ns.node_size; ++j){
            if (alt[j] >= 0 && ns->node[i].conflict[j])
                nodeAddConflict(ns->node + i, ns->node + alt[j]);
        }
    }

    planPDDLSasInvNodesFree(&old_ns);
    BOR_FREE(alt);
    return 0;
}

int planPDDLSasInvNodeInvSize(const plan_pddl_sas_inv_node_t *node)
{
    int i, size;

    size = 0;
    for (i = 0; i < node->fact_size; ++i){
        if (node->inv[i])
            ++size;
    }

    return size;
}

void planPDDLSasInvNodeUniqueEdges(plan_pddl_sas_inv_node_t *node)
{
    edgesUnique(&node->edges);
}

int planPDDLSasInvNodeRemoveCoveredEdges(plan_pddl_sas_inv_node_t *node)
{
    return edgesRemoveCoveredEdges(&node->edges, node->must);
}

int planPDDLSasInvNodeRemoveConflictNodesFromEdges(plan_pddl_sas_inv_node_t *node)
{
    int i, change = 0, empty = 0;

    for (i = 0; i < node->edges.edge_size; ++i){
        change |= edgeRemoveNodes(node->edges.edge + i, node->conflict);
        if (node->edges.edge[i].size == 0){
            node->must[CONFLICT_NODE_ID] = 1;
            empty = 1;
        }
    }

    if (empty)
        edgesRemoveEmptyEdges(&node->edges);
    return change;
}

int planPDDLSasInvNodeAddMustFromSingleEdges(plan_pddl_sas_inv_node_t *node)
{
    int i, change = 0;

    for (i = 0; i < node->edges.edge_size; ++i){
        if (node->edges.edge[i].size == 1)
            change |= nodeAddMust(node, node->edges.edge[i].node[0]);
    }

    return change;
}

int planPDDLSasInvNodePruneSuperEdges(plan_pddl_sas_inv_node_t *node,
                                      plan_pddl_sas_inv_nodes_t *ns)
{
    plan_pddl_sas_inv_edge_t *sub, *super;
    int i, j, change = 0;

    for (i = 0; i < node->edges.edge_size; ++i){
        sub = node->edges.edge + i;
        if (sub->size == 0)
            continue;

        for (j = 0; j < node->edges.edge_size; ++j){
            super = node->edges.edge + j;
            if (sub == super)
                continue;

            if (edgeIsSubset(sub, super)){
                edgeAddConflictsFromSuperEdge(sub, super, node, ns);
                edgeFree(super);
                change = 1;
            }
        }
    }

    if (change)
        edgesRemoveEmptyEdges(&node->edges);

    return change;
}

int planPDDLSasInvNodeAddMustsOfMusts(plan_pddl_sas_inv_node_t *node,
                                      const plan_pddl_sas_inv_nodes_t *ns)
{
    const plan_pddl_sas_inv_node_t *node2;
    int i, j, change = 0;

    for (i = 0; i < ns->node_size; ++i){
        if (!node->must[i] || i == node->id)
            continue;

        node2 = ns->node + i;
        for (j = 0; j < ns->node_size; ++j){
            if (node2->must[j])
                change |= nodeAddMust(node, j);
        }
    }

    return change;
}

int planPDDLSasInvNodeAddConflictsOfMusts(plan_pddl_sas_inv_node_t *node,
                                          plan_pddl_sas_inv_nodes_t *ns)
{
    const plan_pddl_sas_inv_node_t *node2;
    int i, j, change = 0;

    for (i = 0; i < ns->node_size; ++i){
        if (!node->must[i] && i != node->id)
            continue;

        node2 = ns->node + i;
        for (j = 0; j < ns->node_size; ++j){
            if (node2->conflict[j])
                change |= nodeAddConflict(node, ns->node + j);
        }
    }

    return change;
}

void planPDDLSasInvNodePrint(const plan_pddl_sas_inv_node_t *node, FILE *fout)
{
    int i, j;

    fprintf(fout, "Node %d", node->id);
    if (node->id == CONFLICT_NODE_ID)
        fprintf(fout, "*");
    fprintf(fout, ": ");

    fprintf(fout, "repr: %d", node->repr);
    fprintf(fout, ", new-id: %d", node->new_id);

    fprintf(fout, ", inv:");
    for (i = 0; i < node->fact_size; ++i){
        if (node->inv[i])
            fprintf(fout, " %d", i);
    }

    fprintf(fout, ", conflict:");
    for (i = 0; i < node->node_size; ++i){
        if (node->conflict[i])
            fprintf(fout, " %d", i);
    }

    fprintf(fout, ", must:");
    for (i = 0; i < node->node_size; ++i){
        if (node->must[i])
            fprintf(fout, " %d", i);
    }

    fprintf(fout, ", edge[%d]:", node->edges.edge_size);
    for (i = 0; i < node->edges.edge_size; ++i){
        fprintf(fout, " [");
        for (j = 0; j < node->edges.edge[i].size; ++j){
            if (j != 0)
                fprintf(fout, " ");
            fprintf(fout, "%d", node->edges.edge[i].node[j]);
        }
        fprintf(fout, "]");
    }

    fprintf(fout, "\n");
}

void planPDDLSasInvNodesPrint(const plan_pddl_sas_inv_nodes_t *ns, FILE *fout)
{
    int i;

    for (i = 0; i < ns->node_size; ++i)
        planPDDLSasInvNodePrint(ns->node + i, fout);
    fprintf(fout, "\n");
}



/*** EDGE ***/
static int cmpInt(const void *a, const void *b)
{
    int v1 = *(int *)a;
    int v2 = *(int *)b;
    return v1 - v2;
}

static int cmpEdge(const void *a, const void *b)
{
    const plan_pddl_sas_inv_edge_t *e1 = a;
    const plan_pddl_sas_inv_edge_t *e2 = b;
    int i, j;

    for (i = 0, j = 0; i < e1->size && j < e2->size; ++i, ++j){
        if (e1->node[i] < e2->node[j])
            return -1;
        if (e1->node[i] > e2->node[j])
            return 1;
    }
    if (i == e1->size && j == e2->size)
        return 0;
    if (i == e1->size)
        return -1;
    return 1;
}

static void edgeInit(plan_pddl_sas_inv_edge_t *e,
                     const int *ids, int size)
{
    bzero(e, sizeof(*e));
    e->size = size;
    e->alloc = size;
    e->node = BOR_ALLOC_ARR(int, size);
    memcpy(e->node, ids, sizeof(int) * size);
    edgeUnique(e);
}

static void edgeFree(plan_pddl_sas_inv_edge_t *e)
{
    if (e->node)
        BOR_FREE(e->node);
    bzero(e, sizeof(*e));
}

static void edgeUnique(plan_pddl_sas_inv_edge_t *edge)
{
    int i, ins;

    if (edge->size <= 1)
        return;

    qsort(edge->node, edge->size, sizeof(int), cmpInt);
    for (i = 1, ins = 0; i < edge->size; ++i){
        if (edge->node[i] != edge->node[ins]){
            edge->node[++ins] = edge->node[i];
        }
    }
    edge->size = ins + 1;
}

static int edgeHasNode(const plan_pddl_sas_inv_edge_t *e, int node_id)
{
    int i;

    for (i = 0; i < e->size; ++i){
        if (e->node[i] == node_id)
            return 1;
        if (e->node[i] > node_id)
            return 0;
    }

    return 0;
}

static int edgeIsCovered(const plan_pddl_sas_inv_edge_t *edge, const int *ns)
{
    int i;

    for (i = 0; i < edge->size; ++i){
        if (ns[edge->node[i]])
            return 1;
    }

    return 0;
}

static int edgeIsSubset(const plan_pddl_sas_inv_edge_t *e1,
                        const plan_pddl_sas_inv_edge_t *e2)
{
    int i, j;

    if (e1->size > e2->size)
        return 0;

    for (i = 0, j = 0; i < e1->size && j < e2->size;){
        if (e1->node[i] == e2->node[j]){
            ++i;
            ++j;
        }else if (e1->node[i] < e2->node[j]){
            return 0;
        }else{
            ++j;
        }
    }

    return i == e1->size;
}

static int edgeRemoveNodes(plan_pddl_sas_inv_edge_t *e, const int *ns)
{
    int i, ins, change = 0;

    for (i = 0, ins = 0; i < e->size; ++i){
        if (ns[e->node[i]]){
            change = 1;
        }else{
            e->node[ins++] = e->node[i];
        }
    }
    e->size = ins;
    return change;
}

static void edgeAddConflictsFromSuperEdge(const plan_pddl_sas_inv_edge_t *sub,
                                          const plan_pddl_sas_inv_edge_t *super,
                                          plan_pddl_sas_inv_node_t *node,
                                          plan_pddl_sas_inv_nodes_t *ns)
{
    int i, j;

    for (i = 0, j = 0; i < sub->size && j < super->size;){
        if (sub->node[i] == super->node[j]){
            ++i;
            ++j;

        }else if (sub->node[i] < super->node[j]){
            ++i;
        }else{
            nodeAddConflict(node, ns->node + super->node[j]);
            ++j;
        }
    }

    for (; j < super->size; ++j)
        nodeAddConflict(node, ns->node + super->node[j]);
}

static void edgeAddNode(plan_pddl_sas_inv_edge_t *e, int node_id)
{
    if (edgeHasNode(e, node_id))
        return;

    if (e->size >= e->alloc){
        e->alloc *= 2;
        e->node = BOR_REALLOC_ARR(e->node, int, e->alloc);
    }
    e->node[e->size++] = node_id;
}

static void edgeAddAlternativeNode(plan_pddl_sas_inv_edge_t *e,
                                   int test_id, int add_id)
{
    if (edgeHasNode(e, test_id))
        edgeAddNode(e, add_id);
}

static void edgesAddAlternativeNode(plan_pddl_sas_inv_edges_t *es,
                                    int test_id, int add_id)
{
    int i;

    for (i = 0; i < es->edge_size; ++i)
        edgeAddAlternativeNode(es->edge + i, test_id, add_id);
}

static void edgeAddAlternativeNodes(plan_pddl_sas_inv_edge_t *e,
                                    int test_id,
                                    const int *add_bool, int add_size)
{
    int i;

    if (!edgeHasNode(e, test_id))
        return;

    for (i = 0; i < add_size; ++i){
        if (add_bool[i])
            edgeAddNode(e, i);
    }
}

static void edgesAddAlternativeNodes(plan_pddl_sas_inv_edges_t *es,
                                     int test_id,
                                     const int *add_bool, int add_size)
{
    int i;

    for (i = 0; i < es->edge_size; ++i)
        edgeAddAlternativeNodes(es->edge + i, test_id, add_bool, add_size);
}


static void edgesInit(plan_pddl_sas_inv_edges_t *es)
{
    bzero(es, sizeof(*es));
}

static void edgesFree(plan_pddl_sas_inv_edges_t *es)
{
    int i;

    for (i = 0; i < es->edge_size; ++i)
        edgeFree(es->edge + i);
    if (es->edge)
        BOR_FREE(es->edge);
    bzero(es, sizeof(*es));
}

static int edgesHaveNode(const plan_pddl_sas_inv_edges_t *es, int node_id)
{
    int i;

    for (i = 0; i < es->edge_size; ++i){
        if (edgeHasNode(es->edge + i, node_id))
            return 1;
    }

    return 0;
}

static void _edgesAdd(plan_pddl_sas_inv_edges_t *es,
                      const int *ids, int size)
{
    plan_pddl_sas_inv_edge_t *edge;

    if (es->edge_size >= es->edge_alloc){
        if (es->edge_alloc == 0){
            es->edge_alloc = 2;
        }else{
            es->edge_alloc *= 2;
        }

        es->edge = BOR_REALLOC_ARR(es->edge, plan_pddl_sas_inv_edge_t,
                                   es->edge_alloc);
    }

    edge = es->edge + es->edge_size++;
    edgeInit(edge, ids, size);
}

static void edgesAddFromFacts(plan_pddl_sas_inv_edges_t *es,
                              const plan_pddl_ground_facts_t *fs)
{
    plan_pddl_sas_inv_edge_t *e;
    int i;

    _edgesAdd(es, fs->fact, fs->size);
    e = es->edge + es->edge_size - 1;
    for (i = 0; i < e->size; ++i)
        e->node[i] += 1;
}

static int edgesAdd(plan_pddl_sas_inv_edges_t *es,
                    const plan_pddl_sas_inv_edge_t *e)
{
    _edgesAdd(es, e->node, e->size);
    return es->edge_size - 1;
}

static int edgesAddMapped(plan_pddl_sas_inv_edges_t *es,
                          const plan_pddl_sas_inv_edge_t *e,
                          const plan_pddl_sas_inv_nodes_t *old)
{
    int id, i;

    id = edgesAdd(es, e);
    for (i = 0; i < es->edge[id].size; ++i)
        es->edge[id].node[i] = old->node[es->edge[id].node[i]].new_id;
    return id;
}

static void edgesUnique(plan_pddl_sas_inv_edges_t *es)
{
    int i, ins;

    if (es->edge_size <= 1)
        return;

    qsort(es->edge, es->edge_size, sizeof(plan_pddl_sas_inv_edge_t), cmpEdge);
    for (i = 1, ins = 0; i < es->edge_size; ++i){
        if (cmpEdge(es->edge + i, es->edge + ins) == 0){
            edgeFree(es->edge + i);
        }else{
            es->edge[++ins] = es->edge[i];
        }
    }

    es->edge_size = ins + 1;
}

static int edgesRemoveCoveredEdges(plan_pddl_sas_inv_edges_t *es,
                                    const int *ns)
{
    int i, ins, change = 0;

    for (i = 0, ins = 0; i < es->edge_size; ++i){
        if (edgeIsCovered(es->edge + i, ns)){
            edgeFree(es->edge + i);
            change = 1;
        }else{
            es->edge[ins++] = es->edge[i];
        }
    }

    es->edge_size = ins;
    return change;
}

static void edgesRemoveEmptyEdges(plan_pddl_sas_inv_edges_t *es)
{
    int i, ins;

    for (i = 0, ins = 0; i < es->edge_size; ++i){
        if (es->edge[i].size == 0){
            edgeFree(es->edge + i);
        }else{
            es->edge[ins++] = es->edge[i];
        }
    }
    es->edge_size = ins;
}
/*** EDGE END ***/




/*** NODE ***/
static void nodeInit(plan_pddl_sas_inv_node_t *node,
                     int id, int node_size, int fact_size)
{
    int i;

    bzero(node, sizeof(*node));
    node->id = id;
    node->repr = -1;
    node->new_id = -1;
    node->node_size = node_size;
    node->fact_size = fact_size;

    node->inv = BOR_CALLOC_ARR(int, fact_size);
    node->conflict = BOR_CALLOC_ARR(int, node_size);
    node->must = BOR_CALLOC_ARR(int, node_size);
    node->must[node->id] = 1;

    if (node->id == CONFLICT_NODE_ID){
        for (i = 0; i < node_size; ++i){
            if (i != node->id)
                node->conflict[i] = 1;
        }
    }else{
        node->conflict[CONFLICT_NODE_ID] = 1;
    }

    edgesInit(&node->edges);
}

static void nodeFree(plan_pddl_sas_inv_node_t *node)
{
    if (node->inv)
        BOR_FREE(node->inv);
    if (node->conflict)
        BOR_FREE(node->conflict);
    if (node->must)
        BOR_FREE(node->must);
    edgesFree(&node->edges);
}

static void nodeCopyInvConflictEdges(plan_pddl_sas_inv_node_t *dst,
                                     const plan_pddl_sas_inv_node_t *src)
{
    int i;

    memcpy(dst->inv, src->inv, sizeof(int) * src->fact_size);
    memcpy(dst->conflict, src->conflict, sizeof(int) * src->node_size);
    for (i = 0; i < src->edges.edge_size; ++i)
        edgesAdd(&dst->edges, src->edges.edge + i);
}

static void nodeCopyMapped(plan_pddl_sas_inv_node_t *dst,
                           const plan_pddl_sas_inv_node_t *old,
                           const plan_pddl_sas_inv_nodes_t *old_ns)
{
    int i;

    memcpy(dst->inv, old->inv, sizeof(int) * old->fact_size);
    for (i = 0; i < old->node_size; ++i){
        if (old->conflict[i] && old_ns->node[i].new_id >= 0)
            dst->conflict[old_ns->node[i].new_id] = 1;
    }

    for (i = 0; i < old->edges.edge_size; ++i)
        edgesAddMapped(&dst->edges, old->edges.edge + i, old_ns);
}

static int nodeInvEq(const plan_pddl_sas_inv_node_t *n1,
                     const plan_pddl_sas_inv_node_t *n2)
{
    int cmp;
    cmp = memcmp(n1->inv, n2->inv, sizeof(int) * n1->fact_size);
    return cmp == 0;
}

static int nodeInvSubset(const plan_pddl_sas_inv_node_t *n1,
                         const plan_pddl_sas_inv_node_t *n2)
{
    int i;

    for (i = 0; i < n1->fact_size; ++i){
        if (n1->inv[i] && !n2->inv[i])
            return 0;
    }

    return 1;
}

static void nodeAddEdgesFromOtherNode(plan_pddl_sas_inv_node_t *dst,
                                      const plan_pddl_sas_inv_node_t *src,
                                      plan_pddl_sas_inv_nodes_t *ns)
{
    int i, id, empty = 0;

    if (src->edges.edge_size == 0)
        return;

    for (i = 0; i < src->edges.edge_size; ++i){
        if (!edgeIsCovered(src->edges.edge + i, dst->must)){
            id = edgesAdd(&dst->edges, src->edges.edge + i);
            edgeRemoveNodes(dst->edges.edge + id, dst->conflict);
            if (dst->edges.edge[id].size == 0){
                dst->must[CONFLICT_NODE_ID] = 1;
                empty = 1;
            }
        }
    }

    if (empty)
        edgesRemoveEmptyEdges(&dst->edges);
    edgesUnique(&dst->edges);
}

static int nodeAddMust(plan_pddl_sas_inv_node_t *node, int add_id)
{
    if (!node->must[add_id]){
        node->must[add_id] = 1;
        if (node->conflict[add_id])
            node->must[CONFLICT_NODE_ID] = 1;
        return 1;
    }

    return 0;
}

static int nodeAddConflict(plan_pddl_sas_inv_node_t *node,
                           plan_pddl_sas_inv_node_t *node2)
{
    if (!node->conflict[node2->id]){
        node->conflict[node2->id] = 1;
        node2->conflict[node->id] = 1;

        if (node->must[node2->id])
            node->must[CONFLICT_NODE_ID] = 1;
        if (node2->must[node->id])
            node2->must[CONFLICT_NODE_ID] = 1;
        return 1;
    }

    return 0;
}


static void nodesInit(plan_pddl_sas_inv_nodes_t *ns,
                      int node_size, int fact_size)
{
    int i;

    ns->fact_size = fact_size;
    ns->node_size = node_size;
    ns->node = BOR_ALLOC_ARR(plan_pddl_sas_inv_node_t, node_size);
    for (i = 0; i < ns->node_size; ++i)
        nodeInit(ns->node + i, i, node_size, fact_size);
}

static void nodeGatherEdges(plan_pddl_sas_inv_node_t *node,
                            plan_pddl_sas_inv_nodes_t *ns)
{
    int i;

    for (i = 0; i < ns->node_size; ++i){
        if (node->must[i] && node->id != i)
            nodeAddEdgesFromOtherNode(node, ns->node + i, ns);
    }
}

static void nodesGatherEdges(plan_pddl_sas_inv_nodes_t *ns)
{
    int i;

    for (i = 0; i < ns->node_size; ++i)
        nodeGatherEdges(ns->node + i, ns);
}

static int nodeSetInv(plan_pddl_sas_inv_node_t *node,
                      const plan_pddl_sas_inv_nodes_t *ns)
{
    int i, j, change = 0;

    for (i = 0; i < ns->node_size; ++i){
        if (i == node->id || !node->must[i])
            continue;

        for (j = 0; j < ns->fact_size; ++j){
            if (!node->inv[j] && ns->node[i].inv[j]){
                node->inv[j] = 1;
                change = 1;
            }
        }
    }

    return change;
}

static int nodesSetInv(plan_pddl_sas_inv_nodes_t *ns)
{
    int i, change = 0;

    for (i = 0; i < ns->node_size; ++i)
        change |= nodeSetInv(ns->node + i, ns);
    return change;
}

static void nodesSetRepr(plan_pddl_sas_inv_nodes_t *ns)
{
    plan_pddl_sas_inv_node_t *node;
    plan_pddl_sas_inv_edge_t *edge;
    int i, j, k;

    for (i = 0; i < ns->node_size; ++i)
        ns->node[i].repr = -1;

    // Set .repr
    for (i = 0; i < ns->node_size; ++i){
        node = ns->node + i;

        // Skip nodes with already assigned representative
        if (node->repr >= 0)
            continue;

        // Conflicting nodes set as representative of special
        // conflict-node and free its edges.
        if (node->conflict[node->id]){
            node->repr = CONFLICT_NODE_ID;
            edgesFree(&node->edges);
            continue;
        }

        node->repr = node->id;
        for (j = i + 1; j < ns->node_size; ++j){
            if (nodeInvEq(node, ns->node + j)){
                ns->node[j].repr = i;
                edgesFree(&ns->node[j].edges);
            }
        }
    }

    // Change edges to point directly to representatives
    for (i = 0; i < ns->node_size; ++i){
        node = ns->node + i;
        //if (node->repr != node->id)
        //    continue;

        for (j = 0; j < node->edges.edge_size; ++j){
            edge = node->edges.edge + j;
            for (k = 0; k < edge->size; ++k)
                edge->node[k] = ns->node[edge->node[k]].repr;
        }
    }
}

static int nodesSetNewId(plan_pddl_sas_inv_nodes_t *ns)
{
    int i, id;

    for (i = 0; i < ns->node_size; ++i)
        ns->node[i].new_id = -1;
    for (i = 0, id = 0; i < ns->node_size; ++i){
        if (ns->node[i].repr == ns->node[i].id)
            ns->node[i].new_id = id++;
    }

    return id;
}

static int nodesFindSplit(const plan_pddl_sas_inv_nodes_t *ns)
{
    int i, j, k, *count, split_node = 0;

    count = BOR_CALLOC_ARR(int, ns->node_size);
    for (i = 0; i < ns->node_size; ++i){
        for (j = 0; j < ns->node[i].edges.edge_size; ++j){
            for (k = 0; k < ns->node[i].edges.edge[j].size; ++k){
                ++count[ns->node[i].edges.edge[j].node[k]];
            }
        }
    }

    for (i = 1; i < ns->node_size; ++i){
        if (count[split_node] < count[i])
            split_node = i;
    }

    if (count[split_node] == 0)
        split_node = -1;
    BOR_FREE(count);

    return split_node;
}

static void nodePruneEdgesFromSupersets(plan_pddl_sas_inv_node_t *node,
                                        plan_pddl_sas_inv_nodes_t *ns)
{
    const plan_pddl_sas_inv_edge_t *edge;
    int i, j, *super;

    super = BOR_CALLOC_ARR(int, ns->node_size);

    // First gather node IDs that are present in edges
    for (i = 0; i < node->edges.edge_size; ++i){
        edge = node->edges.edge + i;
        for (j = 0; j < edge->size; ++j){
            if (node->id != edge->node[j])
                super[edge->node[j]] = 1;
        }
    }

    // Now find supersets
    for (i = 0; i < node->node_size; ++i){
        if (super[i] && !nodeInvSubset(node, ns->node + i))
            super[i] = 0;
    }

    // Remove superset invariants
    for (i = 0; i < node->edges.edge_size; ++i){
        edgeRemoveNodes(node->edges.edge + i, super);
        if (node->edges.edge[i].size == 0)
            node->must[CONFLICT_NODE_ID] = 1;
    }

    // Add alternative nodes to the edges that already contains edge
    // pointing to this node.
    for (i = 0; i < node->node_size; ++i){
        edgesAddAlternativeNodes(&ns->node[i].edges, node->id,
                                 super, ns->node_size);
    }

    BOR_FREE(super);
}

static void nodesPruneEdgesFromSupersets(plan_pddl_sas_inv_nodes_t *ns)
{
    int i;

    for (i = 0; i < ns->node_size; ++i)
        nodePruneEdgesFromSupersets(ns->node + i, ns);
}
/*** NODE END ***/
