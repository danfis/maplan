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
#include "plan/pddl_sas.h"
#include "plan/causal_graph.h"

#ifndef _GNU_SOURCE
/** Declaration of qsort_r() function that should be available in libc */
void qsort_r(void *base, size_t nmemb, size_t size,
             int (*compar)(const void *, const void *, void *),
             void *arg);
#endif

struct _inv_t {
    int *fact;
    int size;
    bor_list_t list;
};
typedef struct _inv_t inv_t;

/** Sort and unique nodes within delset */
static void sasDelsetUnique(plan_pddl_sas_delset_t *ds);
/** Sort and unique all delsets */
static void sasDelsetsUnique(plan_pddl_sas_node_t *node);
/** Returns true if the delset is bound with respect to the enabled nodes */
static int sasDelsetIsBound(const plan_pddl_sas_delset_t *ds, const int *ns);
/** Deletes nodes set in ns. Returns 1 if the delset was completely
 *  emptied, 0 otherwise. */
static int sasDelsetDelNodes(plan_pddl_sas_delset_t *ds, const int *ns);


/** Creates an array of nodes */
static plan_pddl_sas_node_t *sasNodesNew(int node_size, int fact_size);
/** Deletes allocated memory */
static void sasNodesDel(plan_pddl_sas_node_t *nodes, int node_size);
/** Initializes a node */
static void sasNodeInit(plan_pddl_sas_node_t *node,
                        int id, int node_size, int fact_size);
/** Frees allocated memory */
static void sasNodeFree(plan_pddl_sas_node_t *node);
/** Add facts from the edge as delsets */
static void sasNodeAddEdge(plan_pddl_sas_node_t *node,
                           const plan_pddl_ground_facts_t *edge);
/** Add facts from action edges as delsets into corresponding nodes */
static void sasNodesAddActionEdges(plan_pddl_sas_t *sas,
                                   const plan_pddl_ground_facts_t *del,
                                   const plan_pddl_ground_facts_t *add);
/** Add conflicts between facts as conflicts between corresponding nodes */
static void sasNodesAddFactConflicts(plan_pddl_sas_t *sas,
                                     const plan_pddl_ground_facts_t *fs);
/** TODO */
static void sasNodesRefine(plan_pddl_sas_node_t *nodes, int nodes_size);
/** TODO */
static void sasNodesCreateInvariants(const plan_pddl_sas_node_t *nodes,
                                     int nodes_size,
                                     plan_pddl_sas_t *sas);
static void sasNodePrint(const plan_pddl_sas_node_t *node, FILE *fout);
static void sasNodesPrint(const plan_pddl_sas_node_t *nodes, int nodes_size,
                          FILE *fout);


/** Frees allocated resources of sas-fact */
static void sasFactFree(plan_pddl_sas_fact_t *f);
/** Add conflicts between all facts in fs */
static void sasFactsAddConflicts(plan_pddl_sas_t *sas,
                                 const plan_pddl_ground_facts_t *fs);
/** Process all actions -- this sets up all facts and edges */
static void processActions(plan_pddl_sas_t *sas,
                           const plan_pddl_ground_action_pool_t *pool);
/** Transforms list of pddl facts to the list of fact IDs */
static void writeFactIds(plan_pddl_ground_facts_t *dst,
                         plan_pddl_fact_pool_t *fact_pool,
                         const plan_pddl_facts_t *src);
/** Finds and creates invariants */
static void findInvariants(plan_pddl_sas_t *sas);
/** Transforms invariants into sas variables */
static void invariantToVar(plan_pddl_sas_t *sas);
/** Applies simplifications received from causal graph */
static void causalGraph(plan_pddl_sas_t *sas);

void planPDDLSasInit(plan_pddl_sas_t *sas, const plan_pddl_ground_t *g)
{
    int i;

    sas->ground = g;

    sas->node_size = g->fact_pool.size + 1;
    sas->node = sasNodesNew(sas->node_size, g->fact_pool.size);
    for (i = 0; i < g->fact_pool.size; ++i)
        sas->node[i].inv[i] = 1;

    sas->fact_size = g->fact_pool.size;
    sas->fact = BOR_CALLOC_ARR(plan_pddl_sas_fact_t, sas->fact_size);
    for (i = 0; i < sas->fact_size; ++i){
        sas->fact[i].id = i;
        sas->fact[i].fact_size = sas->fact_size;
        sas->fact[i].conflict = BOR_CALLOC_ARR(int, sas->fact_size);
        sas->fact[i].var = PLAN_VAR_ID_UNDEFINED;
        sas->fact[i].val = PLAN_VAL_UNDEFINED;
    }

    borListInit(&sas->inv);
    sas->inv_size = 0;

    sas->var_range = NULL;
    sas->var_order = NULL;
    sas->var_size = 0;
    writeFactIds(&sas->goal, (plan_pddl_fact_pool_t *)&g->fact_pool,
                 &g->pddl->goal);
    writeFactIds(&sas->init, (plan_pddl_fact_pool_t *)&g->fact_pool,
                 &g->pddl->init_fact);

    processActions(sas, &g->action_pool);
    sasFactsAddConflicts(sas, &sas->init);
    sasNodesAddFactConflicts(sas, &sas->init);
}

void planPDDLSasFree(plan_pddl_sas_t *sas)
{
    bor_list_t *item;
    inv_t *inv;
    int i;

    if (sas->node)
        sasNodesDel(sas->node, sas->node_size);

    for (i = 0; i < sas->fact_size; ++i)
        sasFactFree(sas->fact + i);
    if (sas->fact != NULL)
        BOR_FREE(sas->fact);

    while (!borListEmpty(&sas->inv)){
        item = borListNext(&sas->inv);
        inv = BOR_LIST_ENTRY(item, inv_t, list);
        borListDel(&inv->list);
        if (inv->fact)
            BOR_FREE(inv->fact);
        BOR_FREE(inv);
    }
    if (sas->var_range != NULL)
        BOR_FREE(sas->var_range);
    if (sas->var_order != NULL)
        BOR_FREE(sas->var_order);
    if (sas->goal.fact != NULL)
        BOR_FREE(sas->goal.fact);
    if (sas->init.fact != NULL)
        BOR_FREE(sas->init.fact);
}

void planPDDLSas(plan_pddl_sas_t *sas, unsigned flags)
{
    findInvariants(sas);
    invariantToVar(sas);

    if (flags & PLAN_PDDL_SAS_USE_CG){
        causalGraph(sas);
    }
}

void planPDDLSasPrintInvariant(const plan_pddl_sas_t *sas,
                               const plan_pddl_ground_t *g,
                               FILE *fout)
{
    const plan_pddl_fact_t *fact;
    bor_list_t *item;
    const inv_t *inv;
    int i, j;

    i = 0;
    BOR_LIST_FOR_EACH(&sas->inv, item){
        inv = BOR_LIST_ENTRY(item, inv_t, list);
        fprintf(fout, "Invariant %d:\n", i);
        for (j = 0; j < inv->size; ++j){
            fact = planPDDLFactPoolGet(&g->fact_pool, inv->fact[j]);
            fprintf(fout, "    ");
            planPDDLFactPrint(&g->pddl->predicate, &g->pddl->obj, fact, fout);
            fprintf(fout, " var: %d, val: %d/%d",
                    sas->fact[inv->fact[j]].var,
                    sas->fact[inv->fact[j]].val,
                    sas->var_range[sas->fact[inv->fact[j]].var]);
            fprintf(fout, "\n");
        }

        ++i;
    }
}

void planPDDLSasPrintInvariantFD(const plan_pddl_sas_t *sas,
                                 const plan_pddl_ground_t *g,
                                 FILE *fout)
{
    const plan_pddl_fact_t *fact;
    bor_list_t *item;
    const inv_t *inv;
    int i, j, k;

    i = 0;
    BOR_LIST_FOR_EACH(&sas->inv, item){
        inv = BOR_LIST_ENTRY(item, inv_t, list);
        fprintf(fout, "I: ");
        for (j = 0; j < inv->size; ++j){
            fact = planPDDLFactPoolGet(&g->fact_pool, inv->fact[j]);
            if (j > 0)
                fprintf(fout, ";");
            fprintf(fout, "Atom ");
            fprintf(fout, "%s(", g->pddl->predicate.pred[fact->pred].name);

            for (k = 0; k < fact->arg_size; ++k){
                if (k > 0)
                    fprintf(fout, ", ");
                fprintf(fout, "%s", g->pddl->obj.obj[fact->arg[k]].name);
            }
            fprintf(fout, ")");
        }
        fprintf(fout, "\n");

        ++i;
    }
}

void planPDDLSasPrintFacts(const plan_pddl_sas_t *sas,
                           const plan_pddl_ground_t *g,
                           FILE *fout)
{
    const plan_pddl_fact_t *fact;
    const plan_pddl_sas_fact_t *f;
    int i;

    fprintf(fout, "SAS Facts[%d]:\n", sas->fact_size);
    for (i = 0; i < sas->fact_size; ++i){
        f = sas->fact + i;
        fact = planPDDLFactPoolGet(&g->fact_pool, i);
        fprintf(fout, "    [%d] ", i);
        //fprintf(fout, "    ");
        planPDDLFactPrint(&g->pddl->predicate, &g->pddl->obj, fact, fout);
        if (f->var != PLAN_VAR_ID_UNDEFINED){
            fprintf(fout, " var: %d, val: %d/%d", (int)f->var, (int)f->val,
                    (int)sas->var_range[f->var]);
        }else{
            fprintf(fout, " var: %d", (int)PLAN_VAR_ID_UNDEFINED);
        }

        /*
        fprintf(fout, ", conflict:");
        for (int j = 0; j < f->conflict.size; ++j){
            fprintf(fout, " %d", f->conflict.fact[j]);
        }
        */

        fprintf(fout, "\n");
    }
}




/*** FACT ***/
static void sasFactFree(plan_pddl_sas_fact_t *f)
{
    int i;

    if (f->conflict)
        BOR_FREE(f->conflict);

    for (i = 0; i < f->edge_size; ++i){
        if (f->edge[i].fact != NULL)
            BOR_FREE(f->edge[i].fact);
    }
    if (f->edge != NULL)
        BOR_FREE(f->edge);
}

static void sasFactAddEdge(plan_pddl_sas_fact_t *f,
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

static void sasFactsAddConflicts(plan_pddl_sas_t *sas,
                                 const plan_pddl_ground_facts_t *fs)
{
    int i, j, f1, f2;

    for (i = 0; i < fs->size; ++i){
        f1 = fs->fact[i];
        for (j = i + 1; j < fs->size; ++j){
            f2 = fs->fact[j];
            sas->fact[f1].conflict[f2] = 1;
            sas->fact[f2].conflict[f1] = 1;
        }
    }
}
/*** FACT END ***/


static void addActionEdges(plan_pddl_sas_t *sas,
                           const plan_pddl_ground_facts_t *del,
                           const plan_pddl_ground_facts_t *add)
{
    plan_pddl_sas_fact_t *fact;
    int i, j;

    for (i = 0; i < add->size; ++i){
        fact = sas->fact + add->fact[i];
        sasFactAddEdge(fact, del);
    }

    if (del->size == 0){
        for (i = 0; i < add->size; ++i){
            fact = sas->fact + add->fact[i];
            for (j = 0; j < sas->fact_size; ++j){
                if (j != fact->id){
                    fact->conflict[j] = 1;
                    sas->fact[j].conflict[fact->id] = 1;
                }
            }
        }
    }
}

static void processAction(plan_pddl_sas_t *sas,
                          const plan_pddl_ground_facts_t *pre,
                          const plan_pddl_ground_facts_t *pre_neg,
                          const plan_pddl_ground_facts_t *eff_add,
                          const plan_pddl_ground_facts_t *eff_del)
{
    addActionEdges(sas, eff_del, eff_add);
    sasFactsAddConflicts(sas, eff_add);

    sasNodesAddActionEdges(sas, eff_del, eff_add);
    sasNodesAddFactConflicts(sas, eff_add);
}

static void processActions(plan_pddl_sas_t *sas,
                           const plan_pddl_ground_action_pool_t *pool)
{
    const plan_pddl_ground_action_t *a;
    int i, j;

    for (i = 0; i < pool->size; ++i){
        a = planPDDLGroundActionPoolGet(pool, i);
        processAction(sas, &a->pre, &a->pre_neg,
                      &a->eff_add, &a->eff_del);
        for (j = 0; j < a->cond_eff.size; ++j){
            processAction(sas, &a->cond_eff.cond_eff[j].pre,
                          &a->cond_eff.cond_eff[j].pre_neg,
                          &a->cond_eff.cond_eff[j].eff_add,
                          &a->cond_eff.cond_eff[j].eff_del);
        }
    }
}

static void writeFactIds(plan_pddl_ground_facts_t *dst,
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



/*** CREATE-INVARIANT END ***/
/** Returns true if inv1 is subset of inv2. */
static int isSubInvariant(const inv_t *inv1, const inv_t *inv2)
{
    int i1, i2;

    if (inv1->size > inv2->size)
        return 0;

    for (i1 = 0, i2 = 0; i1 < inv1->size && i2 < inv2->size;){
        if (inv1->fact[i1] == inv2->fact[i2]){
            ++i1;
            ++i2;

        }else if (inv1->fact[i1] < inv2->fact[i2]){
            return 0;

        }else{
            ++i2;
        }
    }

    if (i1 == inv1->size)
        return 1;
    return 0;
}

/** Returns true if invariant stored in I is subset of inv2. */
static int isSubInvariant2(const int *I, int size, const inv_t *inv2)
{
    int i1, i2;

    for (i1 = 0, i2 = 0; i1 < size && i2 < inv2->size; ++i1){
        if (I[i1] <= 0)
            continue;

        for (; i2 < inv2->size && i1 != inv2->fact[i2]; ++i2);
        if (i2 < inv2->size && i1 == inv2->fact[i2]){
            ++i2;
        }else{
            return 0;
        }
    }

    for (; i1 < size; ++i1){
        if (I[i1] > 0)
            return 0;
    }

    return 1;
}

/** Returns true if the invariant is subset of an invariant already stored
 *  in database. */
static int hasSuperInvariant(const plan_pddl_sas_t *sas, const int *I)
{
    const bor_list_t *item;
    const inv_t *inv2;

    BOR_LIST_FOR_EACH(&sas->inv, item){
        inv2 = BOR_LIST_ENTRY(item, inv_t, list);
        if (isSubInvariant2(I, sas->fact_size, inv2))
            return 1;
    }

    return 0;
}

/** Remove invariants that are subsets of inv. */
static void removeSubInvariants(plan_pddl_sas_t *sas, const inv_t *inv)
{
    bor_list_t *item, *tmp;
    inv_t *inv2;

    BOR_LIST_FOR_EACH_SAFE(&sas->inv, item, tmp){
        inv2 = BOR_LIST_ENTRY(item, inv_t, list);
        if (isSubInvariant(inv2, inv)){
            borListDel(&inv2->list);
            --sas->inv_size;
            if (inv2->fact)
                BOR_FREE(inv2->fact);
            BOR_FREE(inv2);
        }
    }
}

/** Returns true if a new invariant was created
 *  or the invariant (possibly as sub-invariant) is already in the
 *  database. */
static int createInvariant(plan_pddl_sas_t *sas, const int *I)
{
    inv_t *inv;
    int i, size;

    // Compute size of the component.
    for (i = 0, size = 0; i < sas->fact_size; ++i){
        if (I[i] > 0)
            ++size;
    }

    // Ignore single fact invariants
    if (size <= 1)
        return 0;

    // If the invariant stored in I is subset of an invariant already
    // stored, just pretend that it was already added.
    if (hasSuperInvariant(sas, I))
        return 1;

    // Create invariant
    inv = BOR_ALLOC(inv_t);
    inv->fact = BOR_ALLOC_ARR(int, size);
    inv->size = 0;
    borListInit(&inv->list);
    for (i = 0; i < sas->fact_size; ++i){
        if (I[i] > 0)
            inv->fact[inv->size++] = i;
    }

    // Remove invariants that are subset of inv
    removeSubInvariants(sas, inv);
    borListAppend(&sas->inv, &inv->list);
    ++sas->inv_size;

    return 1;
}
/*** CREATE-INVARIANT END ***/



/*** MERGE-INVARIANT ***/
static int factEdgeIsBound(const plan_pddl_ground_facts_t *edge, const int *I)
{
    int i;

    for (i = 0; i < edge->size; ++i){
        if (I[edge->fact[i]])
            return 1;
    }

    return 0;
}

static int factAllEdgesBound(const plan_pddl_sas_fact_t *fact, const int *I)
{
    int i;

    for (i = 0; i < fact->edge_size; ++i){
        if (!factEdgeIsBound(fact->edge + i, I))
            return 0;
    }

    return 1;
}

static int factInConflict(const plan_pddl_sas_fact_t *fact, const int *I)
{
    int i;

    for (i = 0; i < fact->fact_size; ++i){
        if (I[i] && fact->conflict[i])
            return 1;
    }

    return 0;
}

static int checkInvariant(const plan_pddl_sas_t *sas, const int *I)
{
    int i;

    for (i = 0; i < sas->fact_size; ++i){
        if (!I[i])
            continue;

        if (factInConflict(sas->fact + i, I))
            return 0;
        if (!factAllEdgesBound(sas->fact + i, I))
            return 0;
    }

    return 1;
}

/** Returns true if the invariants have at least one common fact */
static int hasCommonFact(const inv_t *inv, const inv_t *inv2)
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

/** Tries to merge invariant inv2 into inv. Returns true on success. */
static int mergeInvariant(const plan_pddl_sas_t *sas,
                          inv_t *inv, const inv_t *inv2)
{
    int *I, i, size, ret = 0;

    // Check that there is at least one common fact
    if (!hasCommonFact(inv, inv2))
        return 0;

    I = BOR_CALLOC_ARR(int, sas->fact_size);
    for (i = 0; i < inv->size; ++i)
        I[inv->fact[i]] = 1;
    for (i = 0; i < inv2->size; ++i)
        I[inv2->fact[i]] = 1;

    if (checkInvariant(sas, I)){
        for (i = 0, size = 0; i < sas->fact_size; ++i){
            if (I[i] > 0)
                ++size;
        }

        inv->size = 0;
        inv->fact = BOR_REALLOC_ARR(inv->fact, int, size);
        for (i = 0; i < sas->fact_size; ++i){
            if (I[i] > 0)
                inv->fact[inv->size++] = i;
        }
        ret = 1;
    }
    BOR_FREE(I);

    return ret;
}

static void mergeInvInvariants(plan_pddl_sas_t *sas, inv_t *inv)
{
    bor_list_t *item, *tmp;
    inv_t *inv2;

    item = borListNext(&inv->list);
    while (item != &sas->inv){
        inv2 = BOR_LIST_ENTRY(item, inv_t, list);
        if (mergeInvariant(sas, inv, inv2)){
            tmp = borListNext(item);
            borListDel(item);
            item = tmp;

            if (inv2->fact)
                BOR_FREE(inv2->fact);
            BOR_FREE(inv2);
        }else{
            item = borListNext(item);
        }
    }
}

/** Try to merge invariants to get the biggest possible invariants */
static void mergeInvariants(plan_pddl_sas_t *sas)
{
    bor_list_t *item;
    inv_t *inv;

    item = borListNext(&sas->inv);
    while (item != &sas->inv){
        inv = BOR_LIST_ENTRY(item, inv_t, list);
        mergeInvInvariants(sas, inv);
        item = borListNext(item);
    }
}
/*** MERGE-INVARIANT END ***/



/*** FIND-INVARIANTS ***/
static void findInvariants(plan_pddl_sas_t *sas)
{
    sasNodesPrint(sas->node, sas->node_size, stderr);
    sasNodesRefine(sas->node, sas->node_size);
    sasNodesPrint(sas->node, sas->node_size, stderr);

    sasNodesCreateInvariants(sas->node, sas->node_size, sas);
    mergeInvariants(sas);
}
/*** FIND-INVARIANTS END ***/



/*** INVARIANT-TO-VAR ***/
static int invariantCmp(const void *a, const void *b)
{
    const plan_pddl_ground_facts_t *f1 = a;
    const plan_pddl_ground_facts_t *f2 = b;
    return f2->size - f1->size;
}

static void invariantsToGroundFacts(plan_pddl_sas_t *sas,
                                    plan_pddl_ground_facts_t *fs)
{
    bor_list_t *item;
    inv_t *inv;
    plan_pddl_ground_facts_t *f;
    int size;

    size = 0;
    BOR_LIST_FOR_EACH(&sas->inv, item){
        inv = BOR_LIST_ENTRY(item, inv_t, list);
        f = fs + size;
        f->size = inv->size;
        f->fact = BOR_ALLOC_ARR(int, inv->size);
        memcpy(f->fact, inv->fact, sizeof(int) * inv->size);
        ++size;
    }

    qsort(fs, size, sizeof(plan_pddl_ground_facts_t), invariantCmp);
}

static void invariantGroundFactsFree(plan_pddl_sas_t *sas,
                                     plan_pddl_ground_facts_t *inv)
{
    int i;
    for (i = 0; i < sas->inv_size; ++i){
        if (inv[i].fact != NULL)
            BOR_FREE(inv[i].fact);
    }
}

static void invariantAddVar(plan_pddl_sas_t *sas,
                            const plan_pddl_ground_facts_t *inv)
{
    int size, var_range, var, val;

    size = var_range = inv->size;
    var = sas->var_size;
    ++sas->var_size;
    sas->var_range = BOR_REALLOC_ARR(sas->var_range, plan_val_t, sas->var_size);
    sas->var_range[var] = var_range;

    for (val = 0; val < size; ++val){
        sas->fact[inv->fact[val]].var = var;
        sas->fact[inv->fact[val]].val = val;
    }
}

static void invariantRemoveFacts(const plan_pddl_sas_t *sas,
                                 const plan_pddl_ground_facts_t *src,
                                 plan_pddl_ground_facts_t *dst)
{
    int si, di, ins, ssize, dsize;
    int sfact, dfact;

    ssize = src->size;
    dsize = dst->size;
    for (si = 0, di = 0, ins = 0; si < ssize && di < dsize;){
        sfact = src->fact[si];
        dfact = dst->fact[di];

        if (sfact == dfact){
            ++si;
            ++di;
        }else if (sfact < dfact){
            ++si;
        }else{ // dfact < sfact
            dst->fact[ins++] = dst->fact[di];
            ++di;
        }
    }

    for (; di < dsize; ++di)
        dst->fact[ins++] = dst->fact[di];

    dst->size = ins;
}

static void invariantRemoveTop(const plan_pddl_sas_t *sas,
                               plan_pddl_ground_facts_t *inv)
{
    int i;

    for (i = 1; i < sas->inv_size; ++i)
        invariantRemoveFacts(sas, inv, inv + i);
    inv[0].size = 0;
    qsort(inv, sas->inv_size, sizeof(plan_pddl_ground_facts_t),
          invariantCmp);
}

static int numVarEdge(const plan_pddl_sas_t *sas,
                      const plan_pddl_ground_facts_t *fs,
                      int var)
{
    int i, num = 0;

    for (i = 0; i < fs->size; ++i){
        if (sas->fact[fs->fact[i]].var == var)
            ++num;
    }
    return num;
}

static void setVarNeg(plan_pddl_sas_t *sas)
{
    const plan_pddl_sas_fact_t *fact;
    int i, j, add, *var_done;

    var_done = BOR_CALLOC_ARR(int, sas->var_size);
    for (i = 0; i < sas->fact_size; ++i){
        fact = sas->fact + i;
        if (var_done[fact->var])
            continue;

        add = (sas->var_range[fact->var] == 1);
        for (j = 0; !add && j < fact->edge_size; ++j){
            if (numVarEdge(sas, fact->edge + j, fact->var) == 0)
                add = 1;
        }

        if (add){
            var_done[fact->var] = 1;
            ++sas->var_range[fact->var];
        }
    }
    BOR_FREE(var_done);
}

static void invariantToVar(plan_pddl_sas_t *sas)
{
    plan_pddl_ground_facts_t *inv = NULL;
    plan_pddl_ground_facts_t one;
    int i;

    if (sas->inv_size > 0){
        inv = BOR_CALLOC_ARR(plan_pddl_ground_facts_t, sas->inv_size);
        invariantsToGroundFacts(sas, inv);

        while (inv[0].size > 1){
            invariantAddVar(sas, inv);
            invariantRemoveTop(sas, inv);
        }
    }

    one.fact = alloca(sizeof(int));
    one.size = 1;
    for (i = 0; i < sas->fact_size; ++i){
        if (sas->fact[i].var == PLAN_VAR_ID_UNDEFINED){
            one.fact[0] = i;
            invariantAddVar(sas, &one);
        }
    }

    if (inv != NULL){
        invariantGroundFactsFree(sas, inv);
        BOR_FREE(inv);
    }

    setVarNeg(sas);
}
/*** INVARIANT-TO-VAR END ***/



/*** CAUSAL GRAPH ***/
static void causalGraphBuildAddEdges(const plan_pddl_sas_t *sas,
                                     const plan_pddl_ground_facts_t *pre,
                                     const plan_pddl_ground_facts_t *eff,
                                     plan_causal_graph_build_t *build)
{
    int i, j, pre_var, eff_var;

    for (i = 0; i < pre->size; ++i){
        for (j = 0; j < eff->size; ++j){
            pre_var = sas->fact[pre->fact[i]].var;
            eff_var = sas->fact[eff->fact[j]].var;
            planCausalGraphBuildAdd(build, pre_var, eff_var);
        }
    }
}

static void causalGraphBuildAddAction(const plan_pddl_sas_t *sas,
                                      const plan_pddl_ground_action_t *action,
                                      plan_causal_graph_build_t *build)
{
    const plan_pddl_ground_cond_eff_t *ce;
    int i;

    causalGraphBuildAddEdges(sas, &action->pre, &action->eff_add, build);
    causalGraphBuildAddEdges(sas, &action->pre, &action->eff_del, build);
    causalGraphBuildAddEdges(sas, &action->pre_neg, &action->eff_add, build);
    causalGraphBuildAddEdges(sas, &action->pre_neg, &action->eff_del, build);

    for (i = 0; i < action->cond_eff.size; ++i){
        ce = action->cond_eff.cond_eff + i;
        causalGraphBuildAddEdges(sas, &action->pre, &ce->eff_add, build);
        causalGraphBuildAddEdges(sas, &action->pre, &ce->eff_del, build);
        causalGraphBuildAddEdges(sas, &action->pre_neg, &ce->eff_add, build);
        causalGraphBuildAddEdges(sas, &action->pre_neg, &ce->eff_del, build);
        causalGraphBuildAddEdges(sas, &ce->pre, &ce->eff_add, build);
        causalGraphBuildAddEdges(sas, &ce->pre_neg, &ce->eff_del, build);
    }
}

static plan_causal_graph_t *causalGraphBuild(const plan_pddl_sas_t *sas)
{
    plan_causal_graph_t *cg;
    plan_causal_graph_build_t cg_build;
    const plan_pddl_ground_action_t *action;
    plan_part_state_t *goal;
    int i;

    planCausalGraphBuildInit(&cg_build);
    for (i = 0; i < sas->ground->action_pool.size; ++i){
        action = planPDDLGroundActionPoolGet(&sas->ground->action_pool, i);
        causalGraphBuildAddAction(sas, action, &cg_build);
    }

    cg = planCausalGraphNew(sas->var_size);
    planCausalGraphBuild(cg, &cg_build);
    planCausalGraphBuildFree(&cg_build);

    goal = planPartStateNew(sas->var_size);
    for (i = 0; i < sas->goal.size; ++i){
        planPartStateSet(goal, sas->fact[sas->goal.fact[i]].var,
                               sas->fact[sas->goal.fact[i]].val);
    }
    planCausalGraph(cg, goal);
    planPartStateDel(goal);

    return cg;
}

static void causalGraph(plan_pddl_sas_t *sas)
{
    plan_causal_graph_t *cg;
    plan_var_id_t *var_map;
    int i, var_size;
    plan_val_t *var_range;

    cg = causalGraphBuild(sas);

    // Create mapping from old var ID to the new ID
    var_map = (plan_var_id_t *)alloca(sizeof(plan_var_id_t) * sas->var_size);
    var_size = cg->var_order_size;
    for (i = 0; i < sas->var_size; ++i)
        var_map[i] = PLAN_VAR_ID_UNDEFINED;
    for (i = 0; i < cg->var_order_size; ++i)
        var_map[cg->var_order[i]] = i;

    // Fix fact variables
    for (i = 0; i < sas->fact_size; ++i)
        sas->fact[i].var = var_map[sas->fact[i].var];

    // Fix ranges of variables
    var_range = BOR_ALLOC_ARR(plan_val_t, var_size);
    for (i = 0; i < sas->var_size; ++i){
        if (var_map[i] != PLAN_VAR_ID_UNDEFINED){
            var_range[var_map[i]] = sas->var_range[i];
        }
    }
    BOR_FREE(sas->var_range);
    sas->var_range = var_range;
    sas->var_size = var_size;

    // Copy var-order array
    sas->var_order = BOR_ALLOC_ARR(plan_var_id_t, cg->var_order_size + 1);
    memcpy(sas->var_order, cg->var_order,
           sizeof(plan_var_id_t) * cg->var_order_size);
    sas->var_order[cg->var_order_size] = PLAN_VAR_ID_UNDEFINED;
    for (i = 0; i < cg->var_order_size; ++i)
        sas->var_order[i] = var_map[sas->var_order[i]];

    planCausalGraphDel(cg);
}
/*** CAUSAL GRAPH END ***/



/*** DELSET ***/
static int cmpDelset(const void *a, const void *b)
{
    const plan_pddl_sas_delset_t *ds1 = a;
    const plan_pddl_sas_delset_t *ds2 = b;
    int i, j;

    for (i = 0, j = 0; i < ds1->size && j < ds2->size; ++i, ++j){
        if (ds1->node[i] < ds2->node[j])
            return -1;
        if (ds1->node[i] > ds2->node[j])
            return 1;
    }
    if (i == ds1->size && j == ds2->size)
        return 0;
    if (i == ds1->size)
        return -1;
    return 1;
}

static int cmpInt(const void *a, const void *b)
{
    int v1 = *(int *)a;
    int v2 = *(int *)b;
    return v1 - v2;
}

static void sasDelsetUnique(plan_pddl_sas_delset_t *ds)
{
    int i, ins;

    if (ds->size <= 1)
        return;

    qsort(ds->node, ds->size, sizeof(int), cmpInt);
    for (i = 1, ins = 0; i < ds->size; ++i){
        if (ds->node[i] != ds->node[ins]){
            ds->node[++ins] = ds->node[i];
        }
    }
    ds->size = ins + 1;
}

static void sasDelsetsUnique(plan_pddl_sas_node_t *node)
{
    int i, ins;

    if (node->delset_size <= 1)
        return;

    qsort(node->delset, node->delset_size,
          sizeof(plan_pddl_sas_delset_t), cmpDelset);
    for (i = 1, ins = 0; i < node->delset_size; ++i){
        if (cmpDelset(node->delset + i, node->delset + ins) == 0){
            BOR_FREE(node->delset[i].node);
        }else{
            node->delset[++ins] = node->delset[i];
        }
    }

    node->delset_size = ins + 1;
}

static int sasDelsetIsBound(const plan_pddl_sas_delset_t *ds, const int *ns)
{
    int i;

    for (i = 0; i < ds->size; ++i){
        if (ns[ds->node[i]])
            return 1;
    }

    return 0;
}

static int sasDelsetDelNodes(plan_pddl_sas_delset_t *ds, const int *ns)
{
    int i, ins, change = 0;

    if (ds->size == 0)
        return 0;

    for (i = 0, ins = 0; i < ds->size; ++i){
        if (!ns[ds->node[i]]){
            ds->node[ins++] = ds->node[i];
        }else{
            change = 1;
        }
    }

    if (ins == 0){
        BOR_FREE(ds->node);
        ds->node = NULL;

    }else if (ds->size != ins){
        ds->node = BOR_REALLOC_ARR(ds->node, int, ins);
    }

    ds->size = ins;
    return change;
}

static void sasDelsetCopy(plan_pddl_sas_delset_t *dst,
                          const plan_pddl_sas_delset_t *src)
{
    dst->size = src->size;
    dst->node = BOR_ALLOC_ARR(int, dst->size);
    memcpy(dst->node, src->node, sizeof(int) * dst->size);
}
/*** DELSET END ***/



/*** NODE ***/
static plan_pddl_sas_node_t *sasNodesNew(int node_size, int fact_size)
{
    plan_pddl_sas_node_t *nodes;
    int i;

    nodes = BOR_ALLOC_ARR(plan_pddl_sas_node_t, node_size);
    for (i = 0; i < node_size; ++i)
        sasNodeInit(nodes + i, i, node_size, fact_size);

    return nodes;
}

static void sasNodesDel(plan_pddl_sas_node_t *nodes, int node_size)
{
    int i;

    for (i = 0; i < node_size; ++i)
        sasNodeFree(nodes + i);
    BOR_FREE(nodes);
}

static void sasNodeInit(plan_pddl_sas_node_t *node,
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

static void sasNodeFree(plan_pddl_sas_node_t *node)
{
    if (node->inv)
        BOR_FREE(node->inv);
    if (node->conflict)
        BOR_FREE(node->conflict);
    if (node->must)
        BOR_FREE(node->must);
}

static void sasNodeAddEdge(plan_pddl_sas_node_t *node,
                           const plan_pddl_ground_facts_t *edge)
{
    plan_pddl_sas_delset_t *delset;

    if (edge->size == 0){
        node->must[node->conflict_node_id] = 1;

    }else{
        ++node->delset_size;
        node->delset = BOR_REALLOC_ARR(node->delset, plan_pddl_sas_delset_t,
                                       node->delset_size);
        delset = node->delset + node->delset_size - 1;
        delset->size = edge->size;
        delset->node = BOR_ALLOC_ARR(int, delset->size);
        memcpy(delset->node, edge->fact, sizeof(int) * delset->size);
        sasDelsetUnique(delset);
    }
}

static void sasNodesAddActionEdges(plan_pddl_sas_t *sas,
                                   const plan_pddl_ground_facts_t *del,
                                   const plan_pddl_ground_facts_t *add)
{
    plan_pddl_sas_node_t *node;
    int i;

    for (i = 0; i < add->size; ++i){
        node = sas->node + add->fact[i];
        sasNodeAddEdge(node, del);
    }
}

static void sasNodesAddFactConflicts(plan_pddl_sas_t *sas,
                                     const plan_pddl_ground_facts_t *fs)
{
    int i, j, f1, f2;

    for (i = 0; i < fs->size; ++i){
        f1 = fs->fact[i];
        for (j = i + 1; j < fs->size; ++j){
            f2 = fs->fact[j];
            sas->node[f1].conflict[f2] = 1;
            sas->node[f2].conflict[f1] = 1;
        }
    }
}

static void sasNodePrint(const plan_pddl_sas_node_t *node, FILE *fout)
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

    fprintf(fout, ", delset:");
    for (i = 0; i < node->delset_size; ++i){
        fprintf(fout, " [");
        for (j = 0; j < node->delset[i].size; ++j){
            if (j != 0)
                fprintf(fout, " ");
            fprintf(fout, "%d", node->delset[i].node[j]);
        }
        fprintf(fout, "]");
    }

    fprintf(fout, "\n");
}

static void sasNodesPrint(const plan_pddl_sas_node_t *nodes, int nodes_size,
                          FILE *fout)
{
    int i;

    for (i = 0; i < nodes_size; ++i)
        sasNodePrint(nodes + i, fout);
    fprintf(fout, "\n");
}

static void sasNodeRemoveEmptyDelsets(plan_pddl_sas_node_t *node)
{
    int i, ins;

    for (i = 0, ins = 0; i < node->delset_size; ++i){
        if (node->delset[i].size == 0){
            if (node->delset[i].node)
                BOR_FREE(node->delset[i].node);
        }else{
            node->delset[ins++] = node->delset[i];
        }
    }

    if (ins == 0){
        BOR_FREE(node->delset);
        node->delset = NULL;

    }else if (node->delset_size != ins){
        node->delset = BOR_REALLOC_ARR(node->delset,
                                       plan_pddl_sas_delset_t, ins);
    }
    node->delset_size = ins;
}

static int sasNodeRemoveBoundDelsets(plan_pddl_sas_node_t *node)
{
    int i, change = 0;

    for (i = 0; i < node->delset_size; ++i){
        if (sasDelsetIsBound(node->delset + i, node->must)){
            node->delset[i].size = 0;
            change = 1;
        }
    }

    if (change)
        sasNodeRemoveEmptyDelsets(node);
    return change;
}

static int sasNodeRemoveConflictNodesFromDelsets(plan_pddl_sas_node_t *node)
{
    int i, change = 0;

    for (i = 0; i < node->delset_size; ++i){
        if (sasDelsetDelNodes(node->delset + i, node->conflict)){
            change = 1;
            if (node->delset[i].size == 0){
                node->must[node->conflict_node_id] = 1;
                fprintf(stderr, "CONFLICT!! %d\n", node->id);
            }
        }
    }

    if (change)
        sasNodeRemoveEmptyDelsets(node);
    return change;
}

static int sasNodeAddMust(plan_pddl_sas_node_t *node, int add_id)
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

static int sasNodeAddConflict(plan_pddl_sas_node_t *node,
                              plan_pddl_sas_node_t *nodes, int add_id)
{
    if (!node->conflict[add_id]){
        node->conflict[add_id] = 1;
        nodes[add_id].conflict[node->id] = 1;

        if (node->must[add_id]){
            node->must[node->conflict_node_id] = 1;
            fprintf(stderr, "ADDCONFLICT!! %d %d\n", node->id, add_id);
        }
        if (nodes[add_id].must[node->id]){
            nodes[add_id].must[node->conflict_node_id] = 1;
            fprintf(stderr, "ADDCONFLICT 2!! %d %d\n", node->id, add_id);
        }
        return 1;
    }

    return 0;
}

static int sasNodeAddMustFromSingleDelsets(plan_pddl_sas_node_t *node)
{
    int i, change = 0;

    for (i = 0; i < node->delset_size; ++i){
        if (node->delset[i].size == 1
                && sasNodeAddMust(node, node->delset[i].node[0]))
            change = 1;
    }

    return change;
}

static int sasNodeAddMustsOfMusts(plan_pddl_sas_node_t *node,
                                  const plan_pddl_sas_node_t *nodes)
{
    const plan_pddl_sas_node_t *node2;
    int i, j, change = 0;

    for (i = 0; i < node->node_size; ++i){
        if (!node->must[i])
            continue;

        node2 = nodes + i;
        for (j = 0; j < node2->node_size; ++j){
            if (node2->must[j] && sasNodeAddMust(node, j))
                change = 1;
        }
    }

    return change;
}

static int sasNodeAddConflictsOfMusts(plan_pddl_sas_node_t *node,
                                      plan_pddl_sas_node_t *nodes)
{
    const plan_pddl_sas_node_t *node2;
    int i, j, change = 0;

    for (i = 0; i < node->node_size; ++i){
        if (!node->must[i])
            continue;

        node2 = nodes + i;
        for (j = 0; j < node2->node_size; ++j){
            if (node2->conflict[j] && sasNodeAddConflict(node, nodes, j))
                change = 1;
        }
    }

    return change;
}

static int sasNodeRefine(plan_pddl_sas_node_t *node,
                         plan_pddl_sas_node_t *nodes)
{
    int change, anychange = 0;

    do {
        change = 0;

        // Sort and unique delsets.
        sasDelsetsUnique(node);

        change |= sasNodeRemoveBoundDelsets(node);
        change |= sasNodeRemoveConflictNodesFromDelsets(node);
        change |= sasNodeAddMustFromSingleDelsets(node);
        change |= sasNodeAddMustsOfMusts(node, nodes);
        change |= sasNodeAddConflictsOfMusts(node, nodes);

        anychange |= change;
    } while (change);

    return anychange;
}

static int sasNodeCopyDelsets(plan_pddl_sas_node_t *dst,
                              const plan_pddl_sas_node_t *src)
{
    int i, oldsize;

    oldsize = dst->delset_size;
    dst->delset = BOR_REALLOC_ARR(dst->delset, plan_pddl_sas_delset_t,
                                  dst->delset_size + src->delset_size);
    for (i = 0; i < src->delset_size; ++i)
        sasDelsetCopy(dst->delset + dst->delset_size++, src->delset + i);

    sasDelsetsUnique(dst);
    sasNodeRemoveBoundDelsets(dst);

    return oldsize != dst->delset_size;
}

static int sasNodeCopyMustDelsets(plan_pddl_sas_node_t *node,
                                  const plan_pddl_sas_node_t *nodes)
{
    int i, change = 0;

    for (i = 0; i < node->node_size; ++i){
        if (node->must[i] && i != node->id)
            change |= sasNodeCopyDelsets(node, nodes + i);
        if (node->must[i] && i < node->fact_size)
            node->inv[i] = 1;
    }

    return change;
}


/** Returns true if .inv[] and .must[] does not equal */
static int sasNodeInvAndMustEq(const plan_pddl_sas_node_t *node)
{
    int i;

    for (i = 0; i < node->fact_size; ++i){
        if ((node->must[i] && !node->inv[i])
                || (!node->must[i] && node->inv[i]))
            return 0;
    }

    return 1;
}

static void sasNodesRefine(plan_pddl_sas_node_t *nodes, int nodes_size)
{
    int i, change;
    int cycle = 1;

    do {
        do {
            change = 0;
            for (i = 0; i < nodes_size; ++i)
                change |= sasNodeCopyMustDelsets(nodes + i, nodes);
        } while (change);

        fprintf(stderr, "DELSET COPY %d\n", cycle);
        sasNodesPrint(nodes, nodes_size, stderr);

        do {
            change = 0;
            for (i = 0; i < nodes_size; ++i)
                change |= sasNodeRefine(nodes + i, nodes);
        } while (change);

        fprintf(stderr, "REFINE %d\n", cycle);
        sasNodesPrint(nodes, nodes_size, stderr);

        change = 0;
        for (i = 0; i < nodes_size; ++i)
            change |= !sasNodeInvAndMustEq(nodes + i);

        ++cycle;
    } while (change);
}

static void sasNodesCreateInvariants(const plan_pddl_sas_node_t *nodes,
                                     int nodes_size,
                                     plan_pddl_sas_t *sas)
{
    int i;

    for (i = 0; i < nodes_size; ++i){
        if (nodes[i].must[nodes[i].conflict_node_id])
            continue;
        if (nodes[i].delset_size == 0)
            createInvariant(sas, nodes[i].inv);
    }
}

/*** NODE END ***/
