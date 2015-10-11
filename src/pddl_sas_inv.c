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


/** Creates an array of nodes from list of facts. */
static void nodesFromFacts(const plan_pddl_sas_inv_fact_t *facts,
                           int facts_size,
                           plan_pddl_sas_inv_node_t **nodes,
                           int *nodes_size);
/** Creates an array of nodes */
static plan_pddl_sas_inv_node_t *nodesNew(int node_size, int fact_size);
/** Deletes allocated memory */
static void nodesDel(plan_pddl_sas_inv_node_t *nodes, int node_size);
/** Initializes a node */
static void nodeInit(plan_pddl_sas_inv_node_t *node,
                     int id, int node_size, int fact_size);
/** Frees allocated memory */
static void nodeFree(plan_pddl_sas_inv_node_t *node);
/** Removes empty edges from the edge list */
static void nodeRemoveEmptyEdges(plan_pddl_sas_inv_node_t *node);
/** Add add_id to the .must[] array */
static int nodeAddMust(plan_pddl_sas_inv_node_t *node, int add_id);
/** Records conflict between two nodes */
static int nodeAddConflict(plan_pddl_sas_inv_node_t *node,
                           plan_pddl_sas_inv_node_t *node2);

static void nodePrint(const plan_pddl_sas_inv_node_t *node, FILE *fout);
static void nodesPrint(const plan_pddl_sas_inv_node_t *nodes, int nodes_size,
                       FILE *fout);


static void refineNodes(plan_pddl_sas_inv_node_t *node, int node_size);


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

    storeFactIds(&invf->fact_init, (plan_pddl_fact_pool_t *)&g->fact_pool,
                 &g->pddl->init_fact);
    processActions(invf, &g->action_pool);
    factsAddConflicts(invf->fact, &invf->fact_init);
}

void planPDDLSasInvFinderFree(plan_pddl_sas_inv_finder_t *invf)
{
    int i;

    for (i = 0; i < invf->fact_size; ++i)
        factFree(invf->fact + i);
    if (invf->fact)
        BOR_FREE(invf->fact);
    if (invf->fact_init.fact)
        BOR_FREE(invf->fact_init.fact);
}

void planPDDLSasInvFinder(plan_pddl_sas_inv_finder_t *invf)
{
    plan_pddl_sas_inv_node_t *node;
    int node_size;

    nodesFromFacts(invf->fact, invf->fact_size, &node, &node_size);
    fprintf(stderr, "INIT\n");
    nodesPrint(node, node_size, stderr);

    fprintf(stderr, "REFINE\n");
    refineNodes(node, node_size);
    nodesPrint(node, node_size, stderr);


    nodesDel(node, node_size);
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

static void nodeFree(plan_pddl_sas_inv_node_t *node)
{
    if (node->inv)
        BOR_FREE(node->inv);
    if (node->conflict)
        BOR_FREE(node->conflict);
    if (node->must)
        BOR_FREE(node->must);
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
            node2->must[node->conflict_node_id] = 1;
            fprintf(stderr, "ADDCONFLICT 2!! %d %d\n", node->id, node2->id);
        }
        return 1;
    }

    return 0;
}

static void nodePrint(const plan_pddl_sas_inv_node_t *node, FILE *fout)
{
    int i, j;

    fprintf(fout, "Node %d", node->id);
    if (node->id == node->conflict_node_id)
        fprintf(fout, "*");
    fprintf(fout, ": ");

    fprintf(fout, "inv:");
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
