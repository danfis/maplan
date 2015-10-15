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
#include <boruvka/hfunc.h>
#include "plan/pddl_sas_inv.h"

/** Stores fact IDs from src to dst */
static void storeFactIds(plan_pddl_ground_facts_t *dst,
                         plan_pddl_fact_pool_t *fact_pool,
                         const plan_pddl_facts_t *src);
/** Process all actions and store all needed informations. */
static void processActions(plan_pddl_sas_inv_finder_t *invf,
                           const plan_pddl_ground_action_pool_t *pool);


/** Initializes fact structure. */
static void factInit(plan_pddl_sas_inv_fact_t *f, int id, int fact_size);
/** Frees allocated memory. */
static void factFree(plan_pddl_sas_inv_fact_t *f);
/** Adds edge to the fact */
static void factAddEdge(plan_pddl_sas_inv_fact_t *f,
                        const plan_pddl_ground_facts_t *fs);
/** Adds conflicts between all facts stored in fs. */
static void factsAddConflicts(plan_pddl_sas_inv_fact_t *facts,
                              const plan_pddl_ground_facts_t *fs);


/** Sort and unique IDs stored in the edge. */
static void edgeUnique(plan_pddl_sas_inv_edge_t *edge);
/** Sort and unique edges in the array */
static void edgesUnique(plan_pddl_sas_inv_edge_t **edge, int *edge_size);
/** Returns true if the edge is covered by any node that has set 1 in the
 *  ns array. */
static int edgeIsCovered(const plan_pddl_sas_inv_edge_t *edge, const int *ns);
/** Remove nodes that has set 1 in the ns array. Returns 1 if any node was
 *  removed, 0 otherwise. */
static int edgeRemoveNodes(plan_pddl_sas_inv_edge_t *edge, const int *ns);
/** Add nodes to the edge if it contains node_id */
static void edgeAddAlternativeNodes(plan_pddl_sas_inv_edge_t *edge,
                                    int node_id,
                                    const int *nodes, int nodes_size);
/** Change all appearances of {from} to {to}. */
static int edgeChangeNode(plan_pddl_sas_inv_edge_t *edge, int from, int to);
/** edgeChangeNode() for all edges */
static int edgesChangeNode(plan_pddl_sas_inv_edge_t **edges, int *edges_size,
                           int from, int to);
/** Copies edge from src to dst */
static void edgeCopy(plan_pddl_sas_inv_edge_t *dst,
                     const plan_pddl_sas_inv_edge_t *src);
/** Add edges from src to dst */
static int edgesAddEdges(plan_pddl_sas_inv_edge_t **dst, int *dst_size,
                         const plan_pddl_sas_inv_edge_t *src, int src_size);


/** Creates an array of nodes from list of facts. */
static void nodesFromFacts(const plan_pddl_sas_inv_fact_t *facts,
                           int facts_size,
                           plan_pddl_sas_inv_node_t **nodes,
                           int *nodes_size);
/** Creates an array of nodes from the already refined nodes. */
static void nodesFromNodes(plan_pddl_sas_inv_node_t *src,
                           int src_size,
                           plan_pddl_sas_inv_node_t **dst,
                           int *dst_size);
/** Split one node and update all nodes. */
static void nodesSplitOnEdge(const plan_pddl_sas_inv_node_t *src, int src_size,
                             plan_pddl_sas_inv_node_t **dst, int *dst_size);
/** Creates an array of nodes */
static plan_pddl_sas_inv_node_t *nodesNew(int node_size, int fact_size);
/** Deletes allocated memory */
static void nodesDel(plan_pddl_sas_inv_node_t *nodes, int node_size);
/** Initializes a node */
static void nodeInit(plan_pddl_sas_inv_node_t *node,
                     int id, int node_size, int fact_size);
/** Copies node from src to dst. */
static void nodeCopy(plan_pddl_sas_inv_node_t *dst,
                     const plan_pddl_sas_inv_node_t *src);
/** Frees allocated memory */
static void nodeFree(plan_pddl_sas_inv_node_t *node);
/** Number of facts in node's invariant. */
static int nodeInvSize(const plan_pddl_sas_inv_node_t *node);
/** Removes empty edges from the edge list */
static void nodeRemoveEmptyEdges(plan_pddl_sas_inv_node_t *node);
/** Add add_id to the .must[] array */
static int nodeAddMust(plan_pddl_sas_inv_node_t *node, int add_id);
/** Records conflict between two nodes */
static int nodeAddConflict(plan_pddl_sas_inv_node_t *node,
                           plan_pddl_sas_inv_node_t *node2);
/** Returns true if the two nodes represent the same invariant. */
static int nodeInvEq(const plan_pddl_sas_inv_node_t *n1,
                     const plan_pddl_sas_inv_node_t *n2);
/** Returns true if n1.inv is subset of n2.inv */
static int nodeInvSubset(const plan_pddl_sas_inv_node_t *n1,
                         const plan_pddl_sas_inv_node_t *n2);
/** Set .inv[], i.e., set of facts that form the invariant. Returns true if
 *  set of invariants was changed. */
static int nodeSetInv(plan_pddl_sas_inv_node_t *node,
                      const plan_pddl_sas_inv_node_t *nodes);
/** Set .inv[] for all nodes, returns true if any node has changed */
static int nodesSetInv(plan_pddl_sas_inv_node_t *nodes, int nodes_size);
/** Sets representatives of nodes. */
static void nodesSetRepr(plan_pddl_sas_inv_node_t *nodes, int nodes_size);
/** Gather edges belonging to the whole node sets defined by .must[] array */
static void nodesGatherEdges(plan_pddl_sas_inv_node_t *node, int nodes_size);
/** Prune node's edges from nodes that are superset of this node. */
static void nodePruneSupersetEdges(plan_pddl_sas_inv_node_t *node,
                                   plan_pddl_sas_inv_node_t *nodes);
/** Returns true if there is a node that has at least one edge */
static int nodesHasEdge(const plan_pddl_sas_inv_node_t *node, int node_size);
/** Returns next split */
static void nodesFindSplit(const plan_pddl_sas_inv_node_t *node, int node_size,
                           int *node_id, int *node_split_id);

static void nodePrint(const plan_pddl_sas_inv_node_t *node, FILE *fout);
static void nodesPrint(const plan_pddl_sas_inv_node_t *nodes, int nodes_size,
                       FILE *fout);


/** Creates a new invariant from the facts stored in binary array */
static plan_pddl_sas_inv_t *invNew(const int *fact, int fact_size);
/** Frees allocated memory */
static void invDel(plan_pddl_sas_inv_t *inv);
/** Returns true if inv is subset of inv2 */
static int invIsSubset(const plan_pddl_sas_inv_t *inv,
                       const plan_pddl_sas_inv_t *inv2);
/** Returns true if the invariants have at least one common fact */
static int invHasCommonFact(const plan_pddl_sas_inv_t *inv,
                            const plan_pddl_sas_inv_t *inv2);


static int refineRemoveCoveredEdges(plan_pddl_sas_inv_node_t *node);
static int refineRemoveConflictNodesFromEdges(plan_pddl_sas_inv_node_t *node);
static void refineNodes(plan_pddl_sas_inv_node_t *node, int node_size);


static int createInvariant(plan_pddl_sas_inv_finder_t *invf,
                           const plan_pddl_sas_inv_node_t *node);
/** Try to merge invariants to get the biggest possible invariants */
static void mergeInvariants(plan_pddl_sas_inv_finder_t *invf);
/** Remove invariants that are subsets of other invariant. */
static void pruneInvSubsets(plan_pddl_sas_inv_finder_t *invf);


static bor_htable_key_t invTableHashCompute(const plan_pddl_sas_inv_t *inv);
static bor_htable_key_t invTableHash(const bor_list_t *key, void *ud);
static int invTableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud);


void planPDDLSasInvFinderInit(plan_pddl_sas_inv_finder_t *invf,
                              const plan_pddl_ground_t *g)
{
    int i;

    bzero(invf, sizeof(*invf));

    invf->fact_size = g->fact_pool.size;
    invf->fact = BOR_ALLOC_ARR(plan_pddl_sas_inv_fact_t, invf->fact_size);
    for (i = 0; i < invf->fact_size; ++i)
        factInit(invf->fact + i, i, invf->fact_size);
    borListInit(&invf->inv);
    invf->inv_size = 0;
    invf->inv_table = borHTableNew(invTableHash, invTableEq, NULL);

    storeFactIds(&invf->fact_init, (plan_pddl_fact_pool_t *)&g->fact_pool,
                 &g->pddl->init_fact);
    processActions(invf, &g->action_pool);
    factsAddConflicts(invf->fact, &invf->fact_init);
}

void planPDDLSasInvFinderFree(plan_pddl_sas_inv_finder_t *invf)
{
    plan_pddl_sas_inv_t *inv;
    bor_list_t *item;
    int i;

    for (i = 0; i < invf->fact_size; ++i)
        factFree(invf->fact + i);
    if (invf->fact)
        BOR_FREE(invf->fact);
    if (invf->fact_init.fact)
        BOR_FREE(invf->fact_init.fact);

    if (invf->inv_table)
        borHTableDel(invf->inv_table);

    while (!borListEmpty(&invf->inv)){
        item = borListNext(&invf->inv);
        borListDel(item);
        inv = BOR_LIST_ENTRY(item, plan_pddl_sas_inv_t, list);
        invDel(inv);
    }
}

void planPDDLSasInvFinder(plan_pddl_sas_inv_finder_t *invf)
{
    plan_pddl_sas_inv_node_t *node, *next_node;
    bor_list_t *item;
    int i, node_size, next_node_size;

    nodesFromFacts(invf->fact, invf->fact_size, &node, &node_size);
    fprintf(stderr, "INIT\n");
    nodesPrint(node, node_size, stderr);

    fprintf(stderr, "REFINE\n");
    refineNodes(node, node_size);
    nodesPrint(node, node_size, stderr);

    //while (1){
    int max_splits = node_size;
    for (i = 0; i < max_splits; ++i){
        while (nodesSetInv(node, node_size)){
            fprintf(stderr, "SET-INV\n");
            nodesPrint(node, node_size, stderr);

            nodesFromNodes(node, node_size, &next_node, &next_node_size);
            nodesDel(node, node_size);
            node = next_node;
            node_size = next_node_size;

            refineNodes(node, node_size);
        }

        if (!nodesHasEdge(node, node_size))
            break;
        /*
        nodesSplitOnEdge(node, node_size, &next_node, &next_node_size);
        node = next_node;
        node_size = next_node_size;

        fprintf(stderr, "REFINE\n");
        refineNodes(node, node_size);
        nodesPrint(node, node_size, stderr);
        */
    }

    // TODO: Exponential extension.
    //       Instead of recursion use splittin a node into two node with
    //       one node from edge put in M and C.
    //       Use certain heuristic for selection of splitting node.

    for (i = 0; i < node_size; ++i)
        createInvariant(invf, node + i);
    mergeInvariants(invf);
    pruneInvSubsets(invf);

    nodesDel(node, node_size);

    plan_pddl_sas_inv_t *inv;
    BOR_LIST_FOR_EACH(&invf->inv, item)
    {
        inv = BOR_LIST_ENTRY(item, plan_pddl_sas_inv_t, list);
        int i;
        fprintf(stderr, "INV:");
        for (i = 0; i < inv->size; ++i)
            fprintf(stderr, " %d", inv->fact[i]);
        fprintf(stderr, "\n");
    }
}



static void storeFactIds(plan_pddl_ground_facts_t *dst,
                         plan_pddl_fact_pool_t *fact_pool,
                         const plan_pddl_facts_t *src)
{
    int i, fact_id;

    dst->size = 0;
    dst->fact = BOR_ALLOC_ARR(int, src->size);
    for (i = 0; i < src->size; ++i){
        fact_id = planPDDLFactPoolFind(fact_pool, src->fact + i);
        if (fact_id >= 0){
            dst->fact[dst->size++] = fact_id;
        }
    }

    if (dst->size != src->size){
        dst->fact = BOR_REALLOC_ARR(dst->fact, int, dst->size);
    }
}

static void processAction(plan_pddl_sas_inv_finder_t *invf,
                          const plan_pddl_ground_facts_t *pre,
                          const plan_pddl_ground_facts_t *pre_neg,
                          const plan_pddl_ground_facts_t *eff_add,
                          const plan_pddl_ground_facts_t *eff_del)
{
    plan_pddl_sas_inv_fact_t *fact;
    int i, j;


    if (eff_del->size == 0){
        for (i = 0; i < eff_add->size; ++i){
            fact = invf->fact + eff_add->fact[i];
            for (j = 0; j < invf->fact_size; ++j){
                if (j != fact->id){
                    fact->conflict[j] = 1;
                    invf->fact[j].conflict[fact->id] = 1;
                }
            }
        }

    }else{
        for (i = 0; i < eff_add->size; ++i){
            fact = invf->fact + eff_add->fact[i];
            factAddEdge(fact, eff_del);
        }
        factsAddConflicts(invf->fact, eff_add);
    }
}

static void processActions(plan_pddl_sas_inv_finder_t *invf,
                           const plan_pddl_ground_action_pool_t *pool)
{
    const plan_pddl_ground_action_t *a;
    int i, j;

    for (i = 0; i < pool->size; ++i){
        a = planPDDLGroundActionPoolGet(pool, i);
        processAction(invf, &a->pre, &a->pre_neg,
                      &a->eff_add, &a->eff_del);
        for (j = 0; j < a->cond_eff.size; ++j){
            processAction(invf, &a->cond_eff.cond_eff[j].pre,
                          &a->cond_eff.cond_eff[j].pre_neg,
                          &a->cond_eff.cond_eff[j].eff_add,
                          &a->cond_eff.cond_eff[j].eff_del);
        }
    }
}


/*** FACT ***/
static void factInit(plan_pddl_sas_inv_fact_t *f, int id, int fact_size)
{
    bzero(f, sizeof(*f));
    f->id = id;
    f->fact_size = fact_size;
    f->conflict = BOR_CALLOC_ARR(int, fact_size);
}

static void factFree(plan_pddl_sas_inv_fact_t *f)
{
    int i;

    if (f->conflict)
        BOR_FREE(f->conflict);

    for (i = 0; i < f->edge_size; ++i){
        if (f->edge[i].fact)
            BOR_FREE(f->edge[i].fact);
    }
    if (f->edge)
        BOR_FREE(f->edge);
}

static void factAddEdge(plan_pddl_sas_inv_fact_t *f,
                        const plan_pddl_ground_facts_t *fs)
{
    plan_pddl_ground_facts_t *dst;

    if (fs->size == 0)
        return;

    ++f->edge_size;
    f->edge = BOR_REALLOC_ARR(f->edge, plan_pddl_ground_facts_t,
                              f->edge_size);
    dst = f->edge + f->edge_size - 1;
    dst->size = fs->size;
    dst->fact = BOR_ALLOC_ARR(int, fs->size);
    memcpy(dst->fact, fs->fact, sizeof(int) * fs->size);
}

static void factsAddConflicts(plan_pddl_sas_inv_fact_t *facts,
                              const plan_pddl_ground_facts_t *fs)
{
    int i, j, f1, f2;

    for (i = 0; i < fs->size; ++i){
        f1 = fs->fact[i];
        for (j = i + 1; j < fs->size; ++j){
            f2 = fs->fact[j];
            facts[f1].conflict[f2] = 1;
            facts[f2].conflict[f1] = 1;
        }
    }
}
/*** FACT END ***/




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

static void edgesUnique(plan_pddl_sas_inv_edge_t **edge_out, int *edge_size)
{
    plan_pddl_sas_inv_edge_t *edge = *edge_out;
    int i, ins, size = *edge_size;

    if (size <= 1)
        return;

    qsort(edge, size, sizeof(plan_pddl_sas_inv_edge_t), cmpEdge);
    for (i = 1, ins = 0; i < size; ++i){
        if (cmpEdge(edge + i, edge + ins) == 0){
            BOR_FREE(edge[i].node);
        }else{
            edge[++ins] = edge[i];
        }
    }

    *edge_size = ins + 1;
    if (size != *edge_size)
        edge = BOR_REALLOC_ARR(edge, plan_pddl_sas_inv_edge_t, *edge_size);
    *edge_out = edge;
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

static int edgeRemoveNodes(plan_pddl_sas_inv_edge_t *edge, const int *ns)
{
    int i, ins, change = 0;

    if (edge->size == 0)
        return 0;

    for (i = 0, ins = 0; i < edge->size; ++i){
        if (!ns[edge->node[i]]){
            edge->node[ins++] = edge->node[i];
        }else{
            change = 1;
        }
    }

    if (ins == 0){
        BOR_FREE(edge->node);
        edge->node = NULL;

    }else if (edge->size != ins){
        edge->node = BOR_REALLOC_ARR(edge->node, int, ins);
    }

    edge->size = ins;
    return change;
}

static void edgeAddAlternativeNodes(plan_pddl_sas_inv_edge_t *edge,
                                    int node_id,
                                    const int *nodes, int nodes_size)
{
    int i, found = 0;

    if (nodes_size == 0)
        return;

    for (i = 0; i < edge->size; ++i){
        if (edge->node[i] == node_id){
            found = 1;
            break;
        }
    }

    if (!found)
        return;


    edge->node = BOR_REALLOC_ARR(edge->node, int, edge->size + nodes_size);
    for (i = 0; i < nodes_size; ++i)
        edge->node[edge->size++] = nodes[i];

    edgeUnique(edge);
}

static int edgeChangeNode(plan_pddl_sas_inv_edge_t *edge, int from, int to)
{
    int i, change = 0;

    for (i = 0; i < edge->size; ++i){
        if (edge->node[i] == from){
            edge->node[i] = to;
            change = 1;
        }
    }

    if (change)
        edgeUnique(edge);

    return change;
}

static int edgesChangeNode(plan_pddl_sas_inv_edge_t **edges, int *edges_size,
                            int from, int to)
{
    int i, change = 0;

    if (*edges_size == 0)
        return 0;

    for (i = 0; i < *edges_size; ++i)
        change |= edgeChangeNode((*edges) + i, from, to);

    if (change)
        edgesUnique(edges, edges_size);
    return change;
}

static void edgeCopy(plan_pddl_sas_inv_edge_t *dst,
                     const plan_pddl_sas_inv_edge_t *src)
{
    dst->size = src->size;
    dst->node = BOR_ALLOC_ARR(int, dst->size);
    memcpy(dst->node, src->node, sizeof(int) * dst->size);
}

static int edgesAddEdges(plan_pddl_sas_inv_edge_t **edges,
                         int *edges_size,
                         const plan_pddl_sas_inv_edge_t *add_edges,
                         int add_edges_size)
{
    plan_pddl_sas_inv_edge_t *dst = *edges;
    int i, dst_size = *edges_size;
    int added = 0;

    if (add_edges_size == 0)
        return 0;

    dst = BOR_REALLOC_ARR(dst, plan_pddl_sas_inv_edge_t,
                          dst_size + add_edges_size);
    for (i = 0; i < add_edges_size; ++i)
        edgeCopy(dst + dst_size++, add_edges + i);

    added = dst_size - *edges_size;
    *edges_size = dst_size;
    *edges = dst;
    edgesUnique(edges, edges_size);
    return added;
}

static void edgesCopyMappedEdges(plan_pddl_sas_inv_edge_t **dst,
                                 int *dst_size,
                                 const plan_pddl_sas_inv_edge_t *src,
                                 int edge_size,
                                 const plan_pddl_sas_inv_node_t *node)
{
    plan_pddl_sas_inv_edge_t *edge;
    int i, j, node_id;

    edge = BOR_CALLOC_ARR(plan_pddl_sas_inv_edge_t, edge_size);
    for (i = 0; i < edge_size; ++i){
        edge[i].node = BOR_ALLOC_ARR(int, src[i].size);
        for (j = 0; j < src[i].size; ++j){
            node_id = src[i].node[j];
            if (node[node_id].new_id >= 0)
                edge[i].node[edge[i].size++] = node[node_id].new_id;
        }
    }

    *dst = edge;
    *dst_size = edge_size;
    edgesUnique(dst, dst_size);
}
/*** EDGE END ***/



/*** NODE ***/
static void nodesFromFacts(const plan_pddl_sas_inv_fact_t *facts,
                           int facts_size,
                           plan_pddl_sas_inv_node_t **nodes_out,
                           int *nodes_size_out)
{
    plan_pddl_sas_inv_node_t *nodes, *node;
    const plan_pddl_sas_inv_fact_t *fact;
    int i, j, node_size = facts_size + 1;

    nodes = BOR_ALLOC_ARR(plan_pddl_sas_inv_node_t, node_size);
    for (i = 0; i < node_size; ++i)
        nodeInit(nodes + i, i, node_size, facts_size);

    for (i = 0; i < facts_size; ++i){
        node = nodes + i;
        fact = facts + i;
        memcpy(node->conflict, fact->conflict, sizeof(int) * facts_size);
        node->inv[i] = 1;

        node->edge_size = fact->edge_size;
        node->edge = BOR_ALLOC_ARR(plan_pddl_sas_inv_edge_t, fact->edge_size);
        for (j = 0; j < node->edge_size; ++j){
            node->edge[j].size = fact->edge[j].size;
            node->edge[j].node = BOR_ALLOC_ARR(int, fact->edge[j].size);
            memcpy(node->edge[j].node, fact->edge[j].fact,
                   sizeof(int) * fact->edge[j].size);
            edgeUnique(node->edge + j);
        }
    }

    *nodes_out = nodes;
    *nodes_size_out = node_size;
}

static void nodeEdgeOccurrenceStats(plan_pddl_sas_inv_node_t *node,
                                    plan_pddl_sas_inv_node_t *nodes)
{
    int i, j, *occ;

    if (node->edge_size == 0)
        return;

    occ = BOR_CALLOC_ARR(int, node->node_size);
    for (i = 0; i < node->edge_size; ++i){
        for (j = 0; j < node->edge[i].size; ++j)
            ++occ[node->edge[i].node[j]];
    }

    for (i = 0; i < node->node_size; ++i){
        if (occ[i] > 0){
            nodes[i].edge_occurrence += occ[i];
            if (nodes[i].max_edge_occurrence < occ[i]){
                nodes[i].max_edge_occurrence = occ[i];
                nodes[i].max_edge_occurrence_node_id = node->id;
            }
        }
    }
    BOR_FREE(occ);
}

static void nodesEdgeOccurrenceStats(plan_pddl_sas_inv_node_t *node,
                                     int src_size)
{
    int i;

    for (i = 0; i < src_size; ++i){
        node[i].edge_occurrence = 0;
        node[i].max_edge_occurrence = 0;
        node[i].max_edge_occurrence_node_id = -1;
    }

    for (i = 0; i < src_size; ++i){
        nodeEdgeOccurrenceStats(node + i, node);
    }
    for (i = 0; i < src_size; ++i){
        fprintf(stderr, "Occurence Stats %d: num: %d, max: %d, max_id: %d,"
                        " edge_size: %d, repr: %d, bazmek: %d\n",
                i, node[i].edge_occurrence,
                node[i].max_edge_occurrence,
                node[i].max_edge_occurrence_node_id,
                node[i].edge_size,
                node[i].repr,
                node[i].must[node[i].node_size - 1]);
    }
}

static void nodesFromNodes(plan_pddl_sas_inv_node_t *src,
                           int src_size,
                           plan_pddl_sas_inv_node_t **dst,
                           int *dst_size)
{
    plan_pddl_sas_inv_node_t *node;
    int i, j, node_size, fact_size = src[0].fact_size;

    nodesSetRepr(src, src_size);
    fprintf(stderr, "SET-REPR\n");
    nodesPrint(src, src_size, stderr);

    nodesGatherEdges(src, src_size);
    for (i = 0; i < src_size; ++i){
        refineRemoveCoveredEdges(src + i);
        refineRemoveConflictNodesFromEdges(src + i);
    }
    fprintf(stderr, "GATHER EDGES\n");
    nodesPrint(src, src_size, stderr);

    nodesEdgeOccurrenceStats(src, src_size);

    node_size = 0;
    for (i = 0; i < src_size; ++i){
        if (src[i].repr < 0){
            src[i].new_id = node_size;
            ++node_size;
        }
    }
    fprintf(stderr, "node_size: %d\n", node_size);
    fprintf(stderr, "NEW ID\n");
    nodesPrint(src, src_size, stderr);

    node = BOR_ALLOC_ARR(plan_pddl_sas_inv_node_t, node_size);
    for (i = 0; i < node_size; ++i)
        nodeInit(node + i, i, node_size, fact_size);

    for (i = 0; i < src_size; ++i){
        if (src[i].new_id < 0 || src[i].repr >= 0)
            continue;

        // Copy facts forming an invariant candidate
        memcpy(node[src[i].new_id].inv, src[i].inv, sizeof(int) * fact_size);

        // Construct conflict table
        for (j = 0; j < src_size; ++j){
            if (src[i].conflict[j] && src[j].new_id >= 0)
                node[src[i].new_id].conflict[src[j].new_id] = 1;
        }

        // Copy edges and map nodes stored there to the new node IDs
        edgesCopyMappedEdges(&node[src[i].new_id].edge,
                             &node[src[i].new_id].edge_size,
                             src[i].edge, src[i].edge_size, src);
    }
    fprintf(stderr, "NEW NODES\n");
    nodesPrint(node, node_size, stderr);

    for (i = 0; i < node_size; ++i){
        nodePruneSupersetEdges(node + i, node);
    }

    fprintf(stderr, "NEW NODES PRUNED\n");
    nodesPrint(node, node_size, stderr);

    *dst = node;
    *dst_size = node_size;
}

static void nodesSplitOnEdge(const plan_pddl_sas_inv_node_t *src, int src_size,
                             plan_pddl_sas_inv_node_t **dst, int *dst_size)
{
    plan_pddl_sas_inv_node_t *node;
    int split_node, splitting_node, new_node;
    int i, j, node_size, fact_size = src[0].fact_size;

    node_size = src_size + 1;
    node = BOR_ALLOC_ARR(plan_pddl_sas_inv_node_t, node_size);
    for (i = 0; i < node_size; ++i)
        nodeInit(node + i, i, node_size, fact_size);
    for (i = 0; i < src_size - 1; ++i)
        nodeCopy(node + i, src + i);

    for (i = 0; i < src_size; ++i){
        if (src[i].edge_size == 0 || src[i].must[src_size - 1])
            continue;

        split_node = i;
        splitting_node = src[i].edge[0].node[0];
        fprintf(stderr, "SPLIT %d %d\n", split_node, splitting_node);

        new_node = node_size - 2;
        nodeCopy(node + new_node, node + split_node);
        node[new_node].must[splitting_node] = 1;
        node[split_node].conflict[splitting_node] = 1;
        node[splitting_node].conflict[split_node] = 1;
        for (i = 0; i < node->node_size; ++i){
            for (j = 0; j < node[i].edge_size; ++j){
                edgeAddAlternativeNodes(node[i].edge + j, split_node,
                                        &new_node, 1);
            }
        }
        break;
    }

    fprintf(stderr, "SPLIT NODES\n");
    nodesPrint(node, node_size, stderr);

    *dst = node;
    *dst_size = node_size;
}

static plan_pddl_sas_inv_node_t *nodesNew(int node_size, int fact_size)
{
    plan_pddl_sas_inv_node_t *nodes;
    int i;

    nodes = BOR_ALLOC_ARR(plan_pddl_sas_inv_node_t, node_size);
    for (i = 0; i < node_size; ++i)
        nodeInit(nodes + i, i, node_size, fact_size);

    return nodes;
}

static void nodesDel(plan_pddl_sas_inv_node_t *nodes, int node_size)
{
    int i;

    for (i = 0; i < node_size; ++i)
        nodeFree(nodes + i);
    BOR_FREE(nodes);
}

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

    node->conflict_node_id = node_size - 1;
    if (node->id == node->conflict_node_id){
        for (i = 0; i < node_size; ++i){
            if (i != node->id)
                node->conflict[i] = 1;
        }
    }
}

static void nodeCopy(plan_pddl_sas_inv_node_t *dst,
                     const plan_pddl_sas_inv_node_t *src)
{
    int i;

    memcpy(dst->inv, src->inv, sizeof(int) * src->fact_size);
    memcpy(dst->conflict, src->conflict, sizeof(int) * src->node_size);
    memcpy(dst->must, src->must, sizeof(int) * src->node_size);
    dst->must[src->id] = 0;
    dst->must[dst->id] = 1;

    dst->edge_size = src->edge_size;
    dst->edge = BOR_ALLOC_ARR(plan_pddl_sas_inv_edge_t, dst->edge_size);
    for (i = 0; i < dst->edge_size; ++i)
        edgeCopy(dst->edge + i, src->edge + i);
}

static void nodeFree(plan_pddl_sas_inv_node_t *node)
{
    if (node->inv)
        BOR_FREE(node->inv);
    if (node->conflict)
        BOR_FREE(node->conflict);
    if (node->must)
        BOR_FREE(node->must);
}

static int nodeInvSize(const plan_pddl_sas_inv_node_t *node)
{
    int i, size;

    size = 0;
    for (i = 0; i < node->fact_size; ++i){
        if (node->inv[i])
            ++size;
    }

    return size;
}

static void nodeRemoveEmptyEdges(plan_pddl_sas_inv_node_t *node)
{
    int i, ins;

    for (i = 0, ins = 0; i < node->edge_size; ++i){
        if (node->edge[i].size == 0){
            if (node->edge[i].node)
                BOR_FREE(node->edge[i].node);
        }else{
            node->edge[ins++] = node->edge[i];
        }
    }

    if (ins == 0){
        BOR_FREE(node->edge);
        node->edge = NULL;

    }else if (node->edge_size != ins){
        node->edge = BOR_REALLOC_ARR(node->edge, plan_pddl_sas_inv_edge_t, ins);
    }
    node->edge_size = ins;
}

static int nodeAddMust(plan_pddl_sas_inv_node_t *node, int add_id)
{
    if (!node->must[add_id]){
        node->must[add_id] = 1;
        if (node->conflict[add_id]){
            node->must[node->conflict_node_id] = 1;
            fprintf(stderr, "ADDMUST!! %d\n", node->id);
        }
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

        if (node->must[node2->id]){
            node->must[node->conflict_node_id] = 1;
            fprintf(stderr, "ADDCONFLICT!! %d %d\n", node->id, node2->id);
        }
        if (node2->must[node->id]){
            node2->must[node2->conflict_node_id] = 1;
            fprintf(stderr, "ADDCONFLICT 2!! %d %d\n", node->id, node2->id);
        }
        return 1;
    }

    return 0;
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

    for (i = 0; i < n1->node_size; ++i){
        if (n1->inv[i] && !n2->inv[i])
            return 0;
    }

    return 1;
}

static int nodeSetInv(plan_pddl_sas_inv_node_t *node,
                      const plan_pddl_sas_inv_node_t *nodes)
{
    int i, j, change = 0;

    for (i = 0; i < node->node_size; ++i){
        if (i == node->id || !node->must[i])
            continue;

        for (j = 0; j < node->fact_size; ++j){
            if (!node->inv[j] && nodes[i].inv[j]){
                node->inv[j] = 1;
                change = 1;
            }
        }
    }

    return change;
}

static int nodesSetInv(plan_pddl_sas_inv_node_t *nodes, int nodes_size)
{
    int i, change = 0;

    for (i = 0; i < nodes_size; ++i)
        change |= nodeSetInv(nodes + i, nodes);
    return change;
}

static void nodesSetRepr(plan_pddl_sas_inv_node_t *nodes, int nodes_size)
{
    int i, j;

    // Set .repr
    for (i = 0; i < nodes_size; ++i){
        if (nodes[i].repr >= 0)
            continue;
        if (nodes[i].must[nodes[i].conflict_node_id]){
            nodes[i].repr = nodes[i].conflict_node_id;
            continue;
        }

        for (j = i + 1; j < nodes_size; ++j){
            if (nodeInvEq(nodes + i, nodes + j))
                nodes[j].repr = i;
        }
    }

    // Change edges to point directly to representatives
    for (i = 0; i < nodes_size; ++i){
        if (nodes[i].repr < 0)
            continue;

        for (j = 0; j < nodes_size; ++j)
            edgesChangeNode(&nodes[j].edge, &nodes[j].edge_size,
                            i, nodes[i].repr);
    }
}

static void nodesGatherEdges(plan_pddl_sas_inv_node_t *nodes, int nodes_size)
{
    plan_pddl_sas_inv_node_t *node;
    int i, j;

    for (i = 0; i < nodes_size; ++i){
        node = nodes + i;

        for (j = 0; j < nodes_size; ++j){
            if (node->must[j] && node->id != j){
                edgesAddEdges(&node->edge, &node->edge_size,
                              nodes[j].edge, nodes[j].edge_size);
            }
        }
    }
}

static void nodePruneSupersetEdges(plan_pddl_sas_inv_node_t *node,
                                   plan_pddl_sas_inv_node_t *nodes)
{
    int i, j, *super, super_size;

    super = BOR_CALLOC_ARR(int, node->node_size);

    // First gather node IDs that are present in edges
    for (i = 0; i < node->edge_size; ++i){
        for (j = 0; j < node->edge[i].size; ++j)
            super[node->edge[i].node[j]] = 1;
    }

    // Now find supersets
    for (i = 0; i < node->node_size; ++i){
        if (super[i]){
            if (i == node->id || !nodeInvSubset(node, nodes + i)){
                super[i] = 0;
            }
        }
    }

    // Remove superset invariants from edges
    for (i = 0; i < node->edge_size; ++i)
        edgeRemoveNodes(node->edge + i, super);

    // Squash list of superset invariants
    super_size = 0;
    for (i = 0; i < node->node_size; ++i){
        if (super[i])
            super[super_size++] = i;
    }

    if (super_size > 0){
        // Add alternative nodes to the edges that already contains edge
        // pointing to this node.
        for (i = 0; i < node->node_size; ++i){
            for (j = 0; j < nodes[i].edge_size; ++j){
                edgeAddAlternativeNodes(nodes[i].edge + j, node->id,
                                        super, super_size);
            }
        }
    }

    // Finally check if there were left an empty edges. If so mark this
    // node as conflicting with any other node.
    for (i = 0; i < node->edge_size; ++i){
        if (node->edge[i].size == 0){
            node->must[node->conflict_node_id] = 1;
            break;
        }
    }

    BOR_FREE(super);
}

static int nodesHasEdge(const plan_pddl_sas_inv_node_t *node, int node_size)
{
    int i;

    for (i = 0; i < node_size; ++i){
        if (node[i].edge_size > 0)
            return 1;
    }

    return 0;
}

static void nodesFindSplit(const plan_pddl_sas_inv_node_t *node, int node_size,
                           int *node_id, int *node_split_id)
{
    int i;

    for (i = 0; i < node_size; ++i){
        if (node[i].edge_size > 0){
            *node_id = i;
            *node_split_id = node[i].edge[0].node[0];
        }
    }
}

static void nodePrint(const plan_pddl_sas_inv_node_t *node, FILE *fout)
{
    int i, j;

    fprintf(fout, "Node %d", node->id);
    if (node->id == node->conflict_node_id)
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

    fprintf(fout, ", edge:");
    for (i = 0; i < node->edge_size; ++i){
        fprintf(fout, " [");
        for (j = 0; j < node->edge[i].size; ++j){
            if (j != 0)
                fprintf(fout, " ");
            fprintf(fout, "%d", node->edge[i].node[j]);
        }
        fprintf(fout, "]");
    }

    fprintf(fout, "\n");
}

static void nodesPrint(const plan_pddl_sas_inv_node_t *nodes, int nodes_size,
                       FILE *fout)
{
    int i;

    for (i = 0; i < nodes_size; ++i)
        nodePrint(nodes + i, fout);
    fprintf(fout, "\n");
}
/*** NODE END ***/



/*** INV ***/
static plan_pddl_sas_inv_t *invNew(const int *fact, int fact_size)
{
    plan_pddl_sas_inv_t *inv;
    int i, size;

    for (size = 0, i = 0; i < fact_size; ++i){
        if (fact[i])
            ++size;
    }

    if (size == 0)
        return NULL;

    inv = BOR_ALLOC(plan_pddl_sas_inv_t);
    inv->fact = BOR_ALLOC_ARR(int, size);
    inv->size = 0;
    borListInit(&inv->list);
    for (i = 0; i < fact_size; ++i){
        if (fact[i])
            inv->fact[inv->size++] = i;
    }

    borListInit(&inv->table);
    inv->table_key = invTableHashCompute(inv);

    return inv;
}

static void invDel(plan_pddl_sas_inv_t *inv)
{
    if (inv->fact)
        BOR_FREE(inv->fact);
    BOR_FREE(inv);
}

static int invIsSubset(const plan_pddl_sas_inv_t *inv,
                       const plan_pddl_sas_inv_t *inv2)
{
    int i, j;

    for (i = 0, j = 0; i < inv->size && j < inv2->size;){
        if (inv->fact[i] == inv2->fact[j]){
            ++i;
            ++j;

        }else if (inv->fact[i] < inv2->fact[j]){
            return 0;
        }else{
            ++j;
        }
    }

    return i == inv->size;
}

static int invHasCommonFact(const plan_pddl_sas_inv_t *inv,
                            const plan_pddl_sas_inv_t *inv2)
{
    int i, j;

    for (i = 0, j = 0; i < inv->size && j < inv2->size;){
        if (inv->fact[i] == inv2->fact[j]){
            return 1;
        }else if (inv->fact[i] < inv2->fact[j]){
            ++i;
        }else{
            ++j;
        }
    }

    return 0;
}
/*** INV END ***/



/*** REFINE ***/
static int refineRemoveCoveredEdges(plan_pddl_sas_inv_node_t *node)
{
    int i, change = 0;

    for (i = 0; i < node->edge_size; ++i){
        if (edgeIsCovered(node->edge + i, node->must)){
            node->edge[i].size = 0;
            change = 1;
        }
    }

    if (change)
        nodeRemoveEmptyEdges(node);
    return change;
}

static int refineRemoveConflictNodesFromEdges(plan_pddl_sas_inv_node_t *node)
{
    int i, change = 0;

    for (i = 0; i < node->edge_size; ++i){
        if (edgeRemoveNodes(node->edge + i, node->conflict)){
            change = 1;
            if (node->edge[i].size == 0){
                node->must[node->conflict_node_id] = 1;
                fprintf(stderr, "CONFLICT!! %d\n", node->id);
            }
        }
    }

    if (change)
        nodeRemoveEmptyEdges(node);
    return change;
}

static void refineAddConflictsFromSuperEdge(const plan_pddl_sas_inv_edge_t *sub,
                                            const plan_pddl_sas_inv_edge_t *super,
                                            plan_pddl_sas_inv_node_t *node,
                                            plan_pddl_sas_inv_node_t *nodes)
{
    int i, j;

    for (i = 0, j = 0; i < sub->size && j < super->size;){
        if (sub->node[i] == super->node[j]){
            ++i;
            ++j;

        }else if (sub->node[i] < super->node[j]){
            ++i;
        }else{
            nodeAddConflict(node, nodes + super->node[j]);
            ++j;
        }
    }

    for (; j < super->size; ++j)
        nodeAddConflict(node, nodes + super->node[j]);
}

static int refinePruneSuperEdges(plan_pddl_sas_inv_node_t *node,
                                 plan_pddl_sas_inv_node_t *nodes)
{
    int i, j, ins, change = 0;

    for (i = 0; i < node->edge_size; ++i){
        if (node->edge[i].size == 0)
            continue;

        for (j = 0; j < node->edge_size; ++j){
            if (i == j || node->edge[j].size == 0)
                continue;

            if (edgeIsSubset(node->edge + i, node->edge + j)){
                refineAddConflictsFromSuperEdge(node->edge + i,
                                                node->edge + j,
                                                node, nodes);
                BOR_FREE(node->edge[j].node);
                node->edge[j].node = NULL;
                node->edge[j].size = 0;
                change = 1;

            }
        }
    }

    for (i = 0, ins = 0; i < node->edge_size; ++i){
        if (node->edge[i].size > 0)
            node->edge[ins++] = node->edge[i];
    }

    node->edge_size = ins;

    return change;
}

static int refineAddMustFromSingleEdges(plan_pddl_sas_inv_node_t *node)
{
    int i, change = 0;

    for (i = 0; i < node->edge_size; ++i){
        if (node->edge[i].size == 1
                && nodeAddMust(node, node->edge[i].node[0]))
            change = 1;
    }

    return change;
}

static int refineAddMustsOfMusts(plan_pddl_sas_inv_node_t *node,
                                 const plan_pddl_sas_inv_node_t *nodes)
{
    const plan_pddl_sas_inv_node_t *node2;
    int i, j, change = 0;

    for (i = 0; i < node->node_size; ++i){
        if (!node->must[i] || i == node->id)
            continue;

        node2 = nodes + i;
        for (j = 0; j < node2->node_size; ++j){
            if (node2->must[j] && nodeAddMust(node, j))
                change = 1;
        }
    }

    return change;
}

static int refineAddConflictsOfMusts(plan_pddl_sas_inv_node_t *node,
                                     plan_pddl_sas_inv_node_t *nodes)
{
    const plan_pddl_sas_inv_node_t *node2;
    int i, j, change = 0;

    for (i = 0; i < node->node_size; ++i){
        if (!node->must[i] && i != node->id)
            continue;

        node2 = nodes + i;
        for (j = 0; j < node2->node_size; ++j){
            if (node2->conflict[j] && nodeAddConflict(node, nodes + j))
                change = 1;
        }
    }

    return change;
}

static int refineNode(plan_pddl_sas_inv_node_t *node,
                      plan_pddl_sas_inv_node_t *nodes)
{
    int change, anychange = 0;

    do {
        change = 0;

        edgesUnique(&node->edge, &node->edge_size);

        change |= refineRemoveCoveredEdges(node);
        change |= refineRemoveConflictNodesFromEdges(node);
        change |= refinePruneSuperEdges(node, nodes);
        change |= refineAddMustFromSingleEdges(node);
        change |= refineAddMustsOfMusts(node, nodes);
        change |= refineAddConflictsOfMusts(node, nodes);

        anychange |= change;
    } while (change);

    return anychange;
}

static void refineNodes(plan_pddl_sas_inv_node_t *node, int node_size)
{
    int i, change;

    do {
        change = 0;
        for (i = 0; i < node_size; ++i)
            change |= refineNode(node + i, node);
    } while (change);
}
/*** REFINE END ***/


/*** CREATE/MERGE-INVARIANT ***/
static int factEdgeIsBound(const plan_pddl_ground_facts_t *edge, const int *I)
{
    int i;

    for (i = 0; i < edge->size; ++i){
        if (I[edge->fact[i]])
            return 1;
    }

    return 0;
}

static int factAllEdgesBound(const plan_pddl_sas_inv_fact_t *fact, const int *I)
{
    int i;

    for (i = 0; i < fact->edge_size; ++i){
        if (!factEdgeIsBound(fact->edge + i, I))
            return 0;
    }

    return 1;
}

static int factInConflict(const plan_pddl_sas_inv_fact_t *fact, const int *I)
{
    int i;

    for (i = 0; i < fact->fact_size; ++i){
        if (I[i] && fact->conflict[i])
            return 1;
    }

    return 0;
}

static int checkInvariant(const plan_pddl_sas_inv_finder_t *invf, const int *I)
{
    int i;

    for (i = 0; i < invf->fact_size; ++i){
        if (!I[i])
            continue;

        if (factInConflict(invf->fact + i, I))
            return 0;
        if (!factAllEdgesBound(invf->fact + i, I))
            return 0;
    }

    return 1;
}

static int checkMergedInvariant(const plan_pddl_sas_inv_finder_t *invf,
                                const plan_pddl_sas_inv_t *inv1,
                                const plan_pddl_sas_inv_t *inv2)
{
    int i, *I;

    I = BOR_CALLOC_ARR(int, invf->fact_size);
    for (i = 0; i < inv1->size; ++i)
        I[inv1->fact[i]] = 1;
    for (i = 0; i < inv2->size; ++i)
        I[inv2->fact[i]] = 1;

    for (i = 0; i < invf->fact_size; ++i){
        if (!I[i])
            continue;

        if (factInConflict(invf->fact + i, I)
                || !factAllEdgesBound(invf->fact + i, I)){
            BOR_FREE(I);
            return 0;
        }
    }

    BOR_FREE(I);
    return 1;
}

static int createUniqueInvariantFromFacts(plan_pddl_sas_inv_finder_t *invf,
                                          const int *fact)
{
    plan_pddl_sas_inv_t *inv;
    bor_list_t *item;

    // Create a unique invariant and append it to the list
    inv = invNew(fact, invf->fact_size);
    item = borHTableInsertUnique(invf->inv_table, &inv->table);
    if (item == NULL){
        ++invf->inv_size;
        borListAppend(&invf->inv, &inv->list);
        return 1;

    }else{
        invDel(inv);
        return 0;
    }
}

static int createMergedInvariant(plan_pddl_sas_inv_finder_t *invf,
                                 const plan_pddl_sas_inv_t *inv1,
                                 const plan_pddl_sas_inv_t *inv2)
{
    int i, *fact, change = 0;

    fact = BOR_CALLOC_ARR(int, invf->fact_size);
    for (i = 0; i < inv1->size; ++i)
        fact[inv1->fact[i]] = 1;
    for (i = 0; i < inv2->size; ++i)
        fact[inv2->fact[i]] = 1;
    change = createUniqueInvariantFromFacts(invf, fact);
    if (change)
        fprintf(stderr, "MERGE\n");

    BOR_FREE(fact);
    return change;
}

static int createInvariant(plan_pddl_sas_inv_finder_t *invf,
                           const plan_pddl_sas_inv_node_t *node)
{
    // Check that the invariant is really invariant
    if (node->edge_size > 0 || node->conflict[node->id]){
        if (checkInvariant(invf, node->inv)){
            fprintf(stderr, "ERROR: Something is really wrong!\n");
        }
        return 0;
    }

    // Skip invariants counting only one fact
    if (nodeInvSize(node) <= 1)
        return 0;

    // Create a unique invariant and append it to the list
    return createUniqueInvariantFromFacts(invf, node->inv);
}

static int mergeInvariantsTry(plan_pddl_sas_inv_finder_t *invf)
{
    bor_list_t *item1, *item2;
    plan_pddl_sas_inv_t *inv1, *inv2;
    int change = 0;

    BOR_LIST_FOR_EACH(&invf->inv, item1){
        inv1 = BOR_LIST_ENTRY(item1, plan_pddl_sas_inv_t, list);
        BOR_LIST_FOR_EACH(&invf->inv, item2){
            inv2 = BOR_LIST_ENTRY(item2, plan_pddl_sas_inv_t, list);
            if (inv1 != inv2
                    && invHasCommonFact(inv1, inv2)
                    && !invIsSubset(inv1, inv2)
                    && checkMergedInvariant(invf, inv1, inv2)){
                change |= createMergedInvariant(invf, inv1, inv2);
            }
        }
    }

    return change;
}

static void mergeInvariants(plan_pddl_sas_inv_finder_t *invf)
{
    while (mergeInvariantsTry(invf));
}

static void pruneInvSubsets(plan_pddl_sas_inv_finder_t *invf)
{
    bor_list_t *item, *item2, *tmp;
    plan_pddl_sas_inv_t *inv1, *inv2;

    BOR_LIST_FOR_EACH(&invf->inv, item){
        inv1 = bor_container_of(item, plan_pddl_sas_inv_t, list);
        BOR_LIST_FOR_EACH_SAFE(&invf->inv, item2, tmp){
            inv2 = bor_container_of(item2, plan_pddl_sas_inv_t, list);
            if (inv1 != inv2 && invIsSubset(inv2, inv1)){
                borListDel(&inv2->list);
                borHTableErase(invf->inv_table, &inv2->table);
                invDel(inv2);
                --invf->inv_size;
            }
        }
    }
}
/*** CREATE/MERGE-INVARIANT END ***/


static bor_htable_key_t invTableHashCompute(const plan_pddl_sas_inv_t *inv)
{
    return borCityHash_64(inv->fact, inv->size * sizeof(int));
}

static bor_htable_key_t invTableHash(const bor_list_t *key, void *ud)
{
    const plan_pddl_sas_inv_t *inv;

    inv = bor_container_of(key, const plan_pddl_sas_inv_t, table);
    return inv->table_key;
}

static int invTableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud)
{
    const plan_pddl_sas_inv_t *inv1, *inv2;

    inv1 = bor_container_of(k1, const plan_pddl_sas_inv_t, table);
    inv2 = bor_container_of(k2, const plan_pddl_sas_inv_t, table);

    if (inv1->size != inv2->size)
        return 0;
    return memcmp(inv1->fact, inv2->fact, sizeof(int) * inv1->size) == 0;
}
