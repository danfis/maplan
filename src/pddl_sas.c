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

/** Frees allocated resources of sas-fact */
static void sasFactFree(plan_pddl_sas_fact_t *f);
/** Add conflicts between all facts in fs */
static void sasFactsAddConflicts(plan_pddl_sas_t *sas,
                                 const plan_pddl_ground_facts_t *fs);
/** Finalizes a fact structure */
static void sasFactFinalize(plan_pddl_sas_t *sas,
                            plan_pddl_sas_fact_t *f);
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
    sas->fact_size = g->fact_pool.size;
    sas->comp = BOR_ALLOC_ARR(int, sas->fact_size);
    sas->close = BOR_ALLOC_ARR(int, sas->fact_size);
    sas->fact = BOR_CALLOC_ARR(plan_pddl_sas_fact_t, sas->fact_size);
    for (i = 0; i < sas->fact_size; ++i){
        sas->fact[i].id = i;
        sas->fact[i].fact_size = sas->fact_size;
        sas->fact[i].conflict = BOR_CALLOC_ARR(int, sas->fact_size);
        sas->fact[i].must = BOR_CALLOC_ARR(int, sas->fact_size);
        sas->fact[i].must[i] = 1;
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
    for (i = 0; i < sas->fact_size; ++i)
        sasFactFinalize(sas, sas->fact + i);
}

void planPDDLSasFree(plan_pddl_sas_t *sas)
{
    bor_list_t *item;
    inv_t *inv;
    int i;

    if (sas->comp != NULL)
        BOR_FREE(sas->comp);
    if (sas->close != NULL)
        BOR_FREE(sas->close);

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




static void sasFactFree(plan_pddl_sas_fact_t *f)
{
    int i;

    if (f->conflict)
        BOR_FREE(f->conflict);
    if (f->must)
        BOR_FREE(f->must);
    if (f->edge_fact.fact)
        BOR_FREE(f->edge_fact.fact);

    for (i = 0; i < f->edge_size; ++i){
        if (f->edge[i].fact != NULL)
            BOR_FREE(f->edge[i].fact);
    }
    if (f->edge != NULL)
        BOR_FREE(f->edge);
}

static void sasFactFinalize(plan_pddl_sas_t *sas,
                            plan_pddl_sas_fact_t *f)
{
    // TODO: Do we need this?
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


/*** FIND-INVARIANTS ***/
// TODO: DEBUG
static void pFact(const plan_pddl_sas_fact_t *fact)
{
    int i, j;

    fprintf(stderr, "Fact %d: ", fact->id);

    fprintf(stderr, "conflict:");
    for (i = 0; i < fact->fact_size; ++i){
        if (fact->conflict[i])
            fprintf(stderr, " %d", i);
    }

    fprintf(stderr, ", must:");
    for (i = 0; i < fact->fact_size; ++i){
        if (fact->must[i])
            fprintf(stderr, " %d", i);
    }

    fprintf(stderr, ", edge[%d]:", fact->edge_size);
    for (i = 0; i < fact->edge_size; ++i){
        fprintf(stderr, " [");
        for (j = 0; j < fact->edge[i].size; ++j){
            if (j > 0)
                fprintf(stderr, " ");
            fprintf(stderr, "%d", fact->edge[i].fact[j]);
        }
        fprintf(stderr, "]");
    }

    fprintf(stderr, ", edge-fact:");
    for (i = 0; i < fact->edge_fact.size; ++i){
        fprintf(stderr, " %d:%d",
                fact->edge_fact.fact[i].fact, fact->edge_fact.fact[i].freq);
    }
    fprintf(stderr, "\n");
}

static int cmpInt(const void *a, const void *b)
{
    int v1 = *(int *)a;
    int v2 = *(int *)b;
    return v1 - v2;
}

static int cmpEdge(const void *a, const void *b)
{
    const plan_pddl_ground_facts_t *fs1 = a;
    const plan_pddl_ground_facts_t *fs2 = b;
    int i, j;

    for (i = 0, j = 0; i < fs1->size && j < fs2->size; ++i, ++j){
        if (fs1->fact[i] < fs2->fact[j])
            return -1;
        if (fs1->fact[i] > fs2->fact[j])
            return 1;
    }
    if (i == fs1->size && j == fs2->size)
        return 0;
    if (i == fs1->size)
        return -1;
    return 1;
}

/** Returns number of bound, unbound, and conflicting facts */
static void edgeState(const plan_pddl_ground_facts_t *fs, const int *I,
                      int *bound, int *unbound, int *conflict)
{
    int i, val;

    *bound = *unbound = *conflict = 0;
    for (i = 0; i < fs->size; ++i){
        val = I[fs->fact[i]];
        if (val == 0){
            *unbound += 1;
        }else if (val > 0){
            *bound += 1;
        }else{ // val < 0
            *conflict += 1;
        }
    }
}

/** Returns true if all edge's facts are in conflict */
static int edgeInConflict(const plan_pddl_ground_facts_t *fs, const int *I)
{
    int bound, unbound, conflict;
    edgeState(fs, I, &bound, &unbound, &conflict);
    if (bound == 0 && unbound == 0 && conflict > 0)
        return 1;
    return 0;
}

/** Returns ID of the only fact that can bind this edge if it is not
 *  already bound. Returns -1 if edge is already bound or there is not only
 *  one way to bind it. */
static int edgeOnlyBind(const plan_pddl_ground_facts_t *fs, const int *I)
{
    int i, val, only = -1;

    for (i = 0; i < fs->size; ++i){
        val = I[fs->fact[i]];
        if (val == 0){
            if (only != -1)
                return -1;
            only = fs->fact[i];

        }else if (val > 0)
            return -1;
    }

    return only;
}

/** Returns true if all edges of the fact are bound. */
static int allEdgesBound(const plan_pddl_sas_fact_t *f, const int *I)
{
    int i, bound, unbound, conflict;

    for (i = 0; i < f->edge_size; ++i){
        edgeState(f->edge + i, I, &bound, &unbound, &conflict);
        if (bound == 0)
            return 0;
    }

    return 1;
}

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

/** Returns true if invariant store in I is subset of inv2. */
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

/** Returns true if the set of facts is really invariant. */
static int checkInvariant(const plan_pddl_sas_t *sas, const int *I)
{
    int i;

    for (i = 0; i < sas->fact_size; ++i){
        if (I[i] > 0 && !allEdgesBound(sas->fact + i, I))
            return 0;
    }

    return 1;
}

/** Returns true if a new invariant was created
 *  or the invariant (possibly as sub-invariant) is already in the
 *  database. */
static int createInvariant(plan_pddl_sas_t *sas, const int *I)
{
    inv_t *inv;
    int i, size;

    if (!checkInvariant(sas, I))
        return 0;

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
    if (hasSuperInvariant(sas, I)){
        fprintf(stderr, "Has super\n");
        return 1;
    }

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

    fprintf(stderr, "Created\n");
    return 1;
}

/** Returns true if the invariants have at least one common fact */
static int hasCommonFact(const inv_t *inv, const inv_t *inv2)
{
    int i, j;

    for (i = 0, j = 0; i < inv->size && j < inv2->size;){
        if (inv->fact[i] == inv2->fact[j]){
            fprintf(stderr, "Common: %d %d\n", inv->fact[i],
                    inv2->fact[j]);
            return 1;
        }else if (inv->fact[i] < inv2->fact[j]){
            ++i;
        }else{
            ++j;
        }
    }

    return 0;
}

static int setFact(const plan_pddl_sas_fact_t *f, int *I, int val);
/** Tries to merge invariant inv2 into inv. Returns true on success. */
static int mergeInvariant(const plan_pddl_sas_t *sas,
                          inv_t *inv, const inv_t *inv2)
{
    int *I, i, size, ret = 0;

    fprintf(stderr, "merge-invariants\n");

    // Check that there is at least one common fact
    if (!hasCommonFact(inv, inv2))
        return 0;

    I = BOR_CALLOC_ARR(int, sas->fact_size);
    for (i = 0; i < inv->size; ++i)
        setFact(sas->fact + inv->fact[i], I, 1);
    for (i = 0; i < inv2->size; ++i){
        if (!setFact(sas->fact + inv2->fact[i], I, 1)){
            BOR_FREE(I);
            return 0;
        }
    }

    if (checkInvariant(sas, I)){
        fprintf(stderr, "inv:");
        for (i = 0; i < inv->size; ++i)
            fprintf(stderr, " %d", inv->fact[i]);
        fprintf(stderr, "\ninv2:");
        for (i = 0; i < inv2->size; ++i)
            fprintf(stderr, " %d", inv2->fact[i]);
        fprintf(stderr, "\n");

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

static void removeEmptyEdges(plan_pddl_sas_fact_t *fact)
{
    int i, ins;

    for (i = 0, ins = 0; i < fact->edge_size; ++i){
        if (fact->edge[i].size == 0){
            if (fact->edge[i].fact)
                BOR_FREE(fact->edge[i].fact);
        }else{
            fact->edge[ins++] = fact->edge[i];
        }
    }
    fact->edge_size = ins;
}

/** Sort edges and keep only the unique one, also removes a single-edge and
 *  adds the fact on the single-edge into .must[] array. */
static void uniqueEdges(plan_pddl_sas_fact_t *fact)
{
    int i;

    // Sort fact within all edges and lexicographicly sort all edges
    for (i = 0; i < fact->edge_size; ++i)
        qsort(fact->edge[i].fact, fact->edge[i].size, sizeof(int), cmpInt);
    qsort(fact->edge, fact->edge_size,
          sizeof(plan_pddl_ground_facts_t), cmpEdge);

    // Mark duplicate edges
    for (i = 1; i < fact->edge_size; ++i){
        if (cmpEdge(fact->edge + i - 1, fact->edge + i) == 0)
            fact->edge[i - 1].size = 0;
    }

    // Keep only the non-duplicate edges
    removeEmptyEdges(fact);
}

/** Adds conflicts from src into dst. */
static int addConflictsFromFact(plan_pddl_sas_t *sas,
                                 plan_pddl_sas_fact_t *dst,
                                 const plan_pddl_sas_fact_t *src)
{
    int i, change = 0;

    for (i = 0; i < src->fact_size; ++i){
        if (src->conflict[i] && !dst->conflict[i]){
            dst->conflict[i] = 1;
            sas->fact[i].conflict[dst->id] = 1;
            change = 1;
        }
    }

    return change;
}

/** Removes i'th fact from the edge */
static void removeFactFromEdge(plan_pddl_ground_facts_t *fs, int i)
{
    for (++i; i < fs->size; ++i)
        fs->fact[i - 1] = fs->fact[i];
    --fs->size;
}

/** Keeps only i'th fact in the edge. */
static void keepOnlyFactInEdge(plan_pddl_ground_facts_t *fs, int i)
{
    fs->fact[0] = fs->fact[i];
    fs->size = 0;
}

/** Extend fact's conflict array with conflicts from all facts that are in
 *  .must[] array. */
static int addConflictsFromMust(plan_pddl_sas_t *sas,
                                plan_pddl_sas_fact_t *fact)
{
    int i, change = 0;

    for (i = 0; i < fact->fact_size; ++i){
        if (fact->must[i])
            change |= addConflictsFromFact(sas, fact, sas->fact + i);
    }

    return change;
}

static int numMustFacts(const plan_pddl_sas_fact_t *fact)
{
    int i, sum;

    for (sum = 0, i = 0; i < fact->fact_size; ++i)
        sum += fact->must[i];
    return sum;
}

static int numConflictFacts(const plan_pddl_sas_fact_t *fact)
{
    int i, sum;

    for (sum = 0, i = 0; i < fact->fact_size; ++i)
        sum += fact->conflict[i];
    return sum;
}

static int refineFact(plan_pddl_sas_t *sas, plan_pddl_sas_fact_t *fact)
{
    plan_pddl_ground_facts_t *edge;
    const plan_pddl_sas_fact_t *fact2;
    int i, j, change, anychange = 0, fact_id;

    do {
        change = 0;

        // Make unique list of edges
        uniqueEdges(fact);

        // Mark single-edges, add facts from them into .must[] array and add
        // conflicts from this fact.
        for (i = 0; i < fact->edge_size; ++i){
            if (fact->edge[i].size == 1){
                fact_id = fact->edge[i].fact[0];
                fact->must[fact_id] = 1;
                addConflictsFromFact(sas, fact, sas->fact + fact_id);
                fact->edge[i].size = 0;
                change = 1;
            }
        }

        // Remove conflicting facts from edges and if an edge contains a
        // must-fact keep only this fact.
        for (i = 0; i < fact->edge_size; ++i){
            edge = fact->edge + i;
            for (j = 0; j < edge->size; ++j){
                if (fact->conflict[edge->fact[j]]){
                    removeFactFromEdge(edge, j);
                    change = 1;
                }

                if (fact->must[edge->fact[j]]){
                    keepOnlyFactInEdge(edge, j);
                    break;
                }
            }
        }

        // Add musts of musts in .must[] and add their conflicts.
        for (i = 0; i < fact->fact_size; ++i){
            if (fact->must[i]){
                fact2 = sas->fact + i;
                for (j = 0; j < fact2->fact_size; ++j){
                    if (fact2->must[j] && !fact->must[j]){
                        fact->must[j] = 1;
                        addConflictsFromFact(sas, fact, sas->fact + j);
                        change = 1;
                    }
                }
            }
        }

        // If this fact must be in invariant with the same fact that it is
        // in conflict with, the fact cannot be in any invariant.
        for (i = 0; i < fact->fact_size; ++i){
            if (fact->conflict[i] && fact->must[i]){
                for (i = 0; i < fact->fact_size; ++i){
                    if (i != fact->id && !fact->conflict[i]){
                        fact->conflict[i] = 1;
                        sas->fact[i].conflict[fact->id] = 1;
                    }
                }
                bzero(fact->must, sizeof(int) * fact->fact_size);
                for (i = 0; i < fact->edge_size; ++i)
                    fact->edge[i].size = 0;
                return 1;
            }
        }

        anychange |= change;
    } while (change);

    return anychange;
}

static int cmpFactFreq(const void *a, const void *b)
{
    const plan_pddl_sas_fact_freq_t *f1 = a;
    const plan_pddl_sas_fact_freq_t *f2 = b;
    return f2->freq - f1->freq;
}

static void factSetEdgeFact(plan_pddl_sas_fact_t *f)
{
    int i, j, size, *fs;

    fs = BOR_CALLOC_ARR(int, f->fact_size);
    for (i = 0; i < f->edge_size; ++i){
        for (j = 0; j < f->edge[i].size; ++j)
            ++fs[f->edge[i].fact[j]];
    }

    for (size = 0, i = 0; i < f->fact_size; ++i){
        if (fs[i] > 0)
            ++size;
    }

    f->edge_fact.size = 0;
    f->edge_fact.fact = BOR_ALLOC_ARR(plan_pddl_sas_fact_freq_t, size);
    for (i = 0; i < f->fact_size; ++i){
        if (fs[i] > 0){
            f->edge_fact.fact[f->edge_fact.size].fact = i;
            f->edge_fact.fact[f->edge_fact.size++].freq = fs[i];
        }
    }
    qsort(f->edge_fact.fact, f->edge_fact.size,
          sizeof(plan_pddl_sas_fact_freq_t), cmpFactFreq);
    BOR_FREE(fs);
}

/** Returns true if adding the fact to the component would cause conflict
 *  with other facts. */
static int factInConflict(const plan_pddl_sas_fact_t *f, const int *I)
{
    int i;

    for (i = 0; i < f->fact_size; ++i){
        if (f->conflict[i] && I[i] > 0)
            return 1;
    }

    return 0;
}

/** Check if fact can be in the current invariant and if so the fact is
 *  added/removed from the component and the conflicting facts are
 *  removed/added from the component (depending on the val).
 *  Returns true if the operation was successful. */
static int setFact(const plan_pddl_sas_fact_t *f, int *I, int val)
{
    int i;

    if (I[f->id] < 0 || f->conflict[f->id] || factInConflict(f, I)){
        if (val == -1){
            fprintf(stderr, "HU?\n");
        }
        return 0;
    }

    for (i = 0; i < f->fact_size; ++i){
        if (f->must[i])
            I[i] += val;
        if (f->conflict[i])
            I[i] -= val;
    }
    return 1;
}

static int createFactInvariants(plan_pddl_sas_t *sas,
                                const plan_pddl_sas_fact_t *fact,
                                int *I, int *C)
{
    int i, created = 0, fact_id, done = 0;
    int *edge_conflict, edge_conflict_size;

    fprintf(stderr, "fact: %d\n", fact->id);
    /*
    for (i = 0; i < sas->fact_size; ++i){
        if (I[i] != 0)
            fprintf(stderr, " %d:%d:%d", i, I[i], C[i]);
    }
    fprintf(stderr, "\n");
    */

    if (allEdgesBound(fact, I)){
        C[fact->id] = 1;
        fprintf(stderr, "ALL BOUND %d\n", fact->id);
        for (i = 0; i < sas->fact_size; ++i){
            if (I[i] > 0 && !C[i]){
                created |= createFactInvariants(sas, sas->fact + i, I, C);
                break;
            }
        }

        if (created == 0){
            created = 1;
            for (i = 0; i < sas->fact_size; ++i){
                if (I[i] > 0 && !C[i]){
                    created = 0;
                    break;
                }
            }

            if (created){
                fprintf(stderr, "CCC\n");
                createInvariant(sas, I);
            }

            fprintf(stderr, "INV:");
            for (i = 0; i < sas->fact_size; ++i){
                if (I[i] > 0){
                    fprintf(stderr, " %d:%d", i, C[i]);
                }
            }
            fprintf(stderr, "\n");
            created = 1;
        }
        C[fact->id] = 0;

    }else{
        // Make sure that are no edges in conflict
        for (i = 0; i < fact->edge_size; ++i){
            if (edgeInConflict(fact->edge + i, I)){
                done = 1;
                break;
            }
        }

        // Try to find next edge that can be bound only by one fact
        for (i = 0; !done && i < fact->edge_size; ++i){
            fact_id = edgeOnlyBind(fact->edge + i, I);
            if (fact_id < 0)
                continue;

            done = 1;
            fprintf(stderr, "edge-only-fact: %d (%d)", fact_id, i);
            if (I[fact_id] == 0 && setFact(sas->fact + fact_id, I, 1)){
                fprintf(stderr, " YES :(\n");
                created |= createFactInvariants(sas, fact, I, C);
                setFact(sas->fact + fact_id, I, -1);
            }else{
                fprintf(stderr, " NO :(\n");
            }
        }

        if (fact->edge_fact.size > 0){
        edge_conflict = BOR_ALLOC_ARR(int, fact->edge_fact.size);
        edge_conflict_size = 0;
        for (i = 0; !done && i < fact->edge_fact.size; ++i){
            fact_id = fact->edge_fact.fact[i].fact;
            if (I[fact_id] == 0 && setFact(sas->fact + fact_id, I, 1)){
                created |= createFactInvariants(sas, fact, I, C);
                setFact(sas->fact + fact_id, I, -1);
                edge_conflict[edge_conflict_size++] = fact_id;
                I[fact_id] = -1;
            }
            fprintf(stderr, "I[%d] = %d\n", fact_id, I[fact_id]);
            //I[fact_id] -= 1;
        }

        for (i = 0; !done && i < edge_conflict_size; ++i){
            I[edge_conflict[i]] = 0;
        }

        BOR_FREE(edge_conflict);
        }
    }

    return created;
}

static void generateBoundEdges(plan_pddl_sas_t *sas,
                               const plan_pddl_sas_fact_t *fact)
{
    int *I;

    I = BOR_CALLOC_ARR(int, fact->fact_size);
    BOR_FREE(I);
}

/*
static int createFactInvariants(plan_pddl_sas_t *sas, const int *fact_ids,
                                const plan_pddl_sas_fact_t *fact,
                                int *I, int *C)
{
    int i, state, created = 0;

    if (!setFact(fact, I, 1))
        return 0;

    if (allEdgesBound(fact, I)){
        C[fact->id] = 1;
        for (i = 0; i < sas->fact_size; ++i){
            if (I[fact_ids[i]] > 0 && !C[fact_ids[i]]){
                fprintf(stderr, ">> %d->%d\n", fact->id, fact_ids[i]);
                created |= createFactInvariants(sas, fact_ids,
                                                sas->fact + fact_ids[i],
                                                I, C);
            }
        }

        if (!created){
            fprintf(stderr, "XXX:");
            for (i = 0; i < sas->fact_size; ++i){
                if (I[i] > 0)
                    fprintf(stderr, " %d:%d", i, C[i]);
            }
            fprintf(stderr, "\n");
            createInvariant(sas, I);
            created = 1;
        }
        C[fact->id] = 0;

    }else{

        for (i = 0; i < fact->edge_size; ++i){
            state = edgeState(fact->edge + i, I);
            if (state == -1){
                return created;

            }else if (state == 0){
                int j;
                for (j = 0; j < fact->edge[i].size; ++j){
                    if (setFact(sas->fact + fact->edge[i].fact[j], I, 1)){
                        created |= createFactInvariants(sas, fact_ids,
                                        fact,
                                        I, C);
                        setFact(sas->fact + fact->edge[i].fact[j], I, -1);
                    }
                }
            }
        }
    }

    setFact(fact, I, -1);

    return created;
}
*/

static int cmpFact(const void *a, const void *b, void *data)
{
    const plan_pddl_sas_t *sas = data;
    int id1 = *(int *)a;
    int id2 = *(int *)b;
    const plan_pddl_sas_fact_t *f1 = sas->fact + id1;
    const plan_pddl_sas_fact_t *f2 = sas->fact + id2;
    int cmp;

    cmp = f1->edge_size - f2->edge_size;
    if (cmp == 0)
        cmp = numMustFacts(f2) - numMustFacts(f1);
    if (cmp == 0)
        cmp = numConflictFacts(f2) - numConflictFacts(f1);
    return cmp;
}

static void findInvariants(plan_pddl_sas_t *sas)
{
    int i, change, *I, *C;

    fprintf(stderr, "Num-facts: %d\n", sas->fact_size);
    fflush(stderr);
    for (i = 0; i < sas->fact_size; ++i)
        pFact(sas->fact + i);
    fprintf(stderr, "\n");
    fflush(stderr);

    do {
        change = 0;
        for (i = 0; i < sas->fact_size; ++i)
            change |= refineFact(sas, sas->fact + i);
        for (i = 0; i < sas->fact_size; ++i)
            pFact(sas->fact + i);
        fprintf(stderr, "\n");
        fflush(stderr);
    } while (change);

    /*
    for (i = 0; i < sas->fact_size; ++i)
        factSetRepr(sas, sas->fact + i);
    */

    for (i = 0; i < sas->fact_size; ++i){
        factSetEdgeFact(sas->fact + i);
        pFact(sas->fact + i);
    }
    fprintf(stderr, "\n");

    I = BOR_CALLOC_ARR(int, sas->fact_size);
    C = BOR_CALLOC_ARR(int, sas->fact_size);
    for (i = 0; i < sas->fact_size; ++i){
        fprintf(stderr, "CFI %d\n", i);
        setFact(sas->fact + i, I, 1);
        createFactInvariants(sas, sas->fact + i, I, C);
        setFact(sas->fact + i, I, -1);

        for (int j = 0; j < sas->fact_size; ++j){
            if (I[j] != 0 || C[j] != 0){
                fprintf(stderr, "ERR\n");
                exit(-1);
            }
        }
    }
    BOR_FREE(C);
    BOR_FREE(I);

    mergeInvariants(sas);
}

#if 0
/** Returns true if adding the fact to the component would cause conflict
 *  with other facts. */
static int factInConflict(const plan_pddl_sas_fact_t *f, const int *I)
{
    int i;

    for (i = 0; i < f->conflict.size; ++i){
        if (I[f->conflict.fact[i]] > 0)
            return 1;
    }

    return 0;
}

/** Check if fact can be in the current invariant and if so the fact is
 *  added/removed from the component and the conflicting facts are
 *  removed/added from the component (depending on the val).
 *  Returns true if the operation was successful. */
static int setFact(const plan_pddl_sas_fact_t *f, int *I, int val)
{
    int i;

    if (I[f->id] < 0 || factInConflict(f, I))
        return 0;

    I[f->id] += val;
    for (i = 0; i < f->fact_size; ++i){
        if (f->neigh[i] < 0)
            I[i] -= val;
    }
    return 1;
}

static int hasConflictEdge(const plan_pddl_sas_fact_t *f, const int *I)
{
    int i;

    for (i = 0; i < f->edge_size; ++i){
        if (edgeState(f->edge + i, I) == -1)
            return 1;
    }

    return 0;
}

/** Returns true if the fact has at least one bound edge */
static int hasBoundEdge(const plan_pddl_sas_fact_t *f, const int *I)
{
    int i, state;

    for (i = 0; i < f->edge_size; ++i){
        state = edgeState(f->edge + i, I);
        if (state == 1)
            return 1;
    }

    return 0;
}


static int processFact(plan_pddl_sas_t *sas, int min_fact_id,
                       const plan_pddl_sas_fact_t *fact,
                       int *C, int *I)
{
    int i, ret = 0, chosen = 1;
    int *conflict;

    /*
    fprintf(stderr, "call %d/%d:", min_fact_id, fact->id);
    for (i = 0; i < sas->fact_size; ++i){
        if (I[i] > 0)
            fprintf(stderr, " %d", i);
    }
    fprintf(stderr, " -|- ");
    for (i = 0; i < sas->fact_size; ++i){
        if (I[i] < 0)
            fprintf(stderr, " %d", i);
    }
    fprintf(stderr, "\n");
    */

    if (hasConflictEdge(fact, I)){
        //fprintf(stderr, "<< C %d/%d\n", min_fact_id, fact->id);
        return 0;
    }

    conflict = BOR_CALLOC_ARR(int, sas->fact_size);

    if (allEdgesBound(fact, I)){
        fprintf(stderr, "all-bound %d/%d:", min_fact_id, fact->id);
        for (i = 0; i < sas->fact_size; ++i){
            if (I[i] > 0)
                fprintf(stderr, " %d", i);
        }
        fprintf(stderr, " -|- ");
        for (i = 0; i < sas->fact_size; ++i){
            if (I[i] < 0)
                fprintf(stderr, " %d", i);
        }
        fprintf(stderr, "\n");
        return 0;

        C[fact->id] = 1;

        for (i = 0; i < sas->fact_size; ++i){
            if (I[i] > 0 && !C[i]){
                ret |= processFact(sas, min_fact_id, sas->fact + i, C, I);

                //fprintf(stderr, "|| ret: %d\n", ret);
                conflict[i] = 1;
                I[i] = -1;
            }
        }

        /*
        if (!ret)
        for (i = min_fact_id; i < sas->fact_size; ++i){
            if (I[i] == 0 && hasBoundEdge(sas->fact + i, I)){
                if (setFact(sas->fact + i, I, 1)){
                    ret |= processFact(sas, min_fact_id, sas->fact + i, C, I);
                    setFact(sas->fact + i, I, -1);
                }

                conflict[i] = 1;
                I[i] = -1;
            }
        }
        */

        if (!ret){
            fprintf(stderr, "INV:");
            for (i = 0; i < sas->fact_size; ++i){
                if (I[i] > 0 && C[i])
                    fprintf(stderr, " %d", i);
                if (I[i] > 0 && !C[i])
                    fprintf(stderr, " X%d", i);
            }
            fprintf(stderr, "\n");
            ret = createInvariant(sas, I);
        }

        C[fact->id] = 0;
        //fprintf(stderr, "<< %d/%d -- ret: %d\n", min_fact_id, fact->id, ret);
        for (i = 0; i < sas->fact_size; ++i){
            if (conflict[i])
                I[i] = 0;
        }
        BOR_FREE(conflict);
        return ret;
    }

    for (i = 0; i < sas->fact_size; ++i){
        if (fact->neigh[i] && I[i] == 0){
            if (setFact(sas->fact + i, I, 1)){
                ret |= processFact(sas, min_fact_id, fact, C, I);
                //fprintf(stderr, "[[ %d/%d\n", min_fact_id, fact->id);
                setFact(sas->fact + i, I, -1);
            }

            conflict[i] = 1;
            I[i] = -1;
        }
    }

    /*
    if (!allEdgesBound(fact, I)){
        BOR_FREE(conflict);
        return ret;
    }
    */
    for (i = 0; i < sas->fact_size; ++i){
        if (conflict[i])
            I[i] = 0;
    }
    BOR_FREE(conflict);
    //fprintf(stderr, "<< N %d/%d\n", min_fact_id, fact->id);
    return ret;
    /*
    exit(-1);

    for (i = 0; i < sas->fact_size; ++i){
        if (I[i] == 0 && hasBoundEdge(sas->fact + i, I)){
            if (setFact(sas->fact + i, I, 1)){
                ret |= processFact(sas, min_fact_id, sas->fact + i, C, I);
                setFact(sas->fact + i, I, -1);
            }

            conflict[i] = 1;
            I[i] = -1;
        }
    }

    if (!ret){
        ret |= createInvariant(sas, I);
        //fprintf(stderr, "RET: %d\n", ret);
    }

    for (i = 0; i < sas->fact_size; ++i){
        if (conflict[i])
            I[i] = 0;
    }

    BOR_FREE(conflict);
    return ret;
    */
}


static void removeConflictsFromEdges(plan_pddl_sas_fact_t *fact)
{
    plan_pddl_ground_facts_t *edge;
    int i, j, ins;

    for (i = 0; i < fact->edge_size; ++i){
        edge = fact->edge + i;
        for (j = 0, ins = 0; j < edge->size; ++j){
            if (fact->neigh[edge->fact[j]] > 0)
                edge->fact[ins++] = edge->fact[j];
            /*
               TODO
            else
                fprintf(stderr, "XXX %d %d\n", fact->id, j);
            */
        }
        edge->size = ins;
    }
    uniqueEdges(fact);
}

static int pruneEdgesRec(plan_pddl_sas_t *sas, plan_pddl_sas_fact_t *fact,
                         int *fs, int *conflict)
{
    int i, j, num_bounds = 0, *conf_local, *ban;

    fprintf(stderr, "Y:");
    for (i = 0; i < sas->fact_size; ++i){
        if (fs[i] > 0){
            fprintf(stderr, " %d", i);
        }
    }
    fprintf(stderr, "\n");

    if (allEdgesBound(fact, fs)){
        fprintf(stderr, "X:");
        conf_local = BOR_CALLOC_ARR(int, sas->fact_size);

        for (i = 0; i < sas->fact_size; ++i){
            if (fs[i] > 0){
                for (j = 0; j < sas->fact_size; ++j){
                    if (sas->fact[i].neigh[j] < 0)
                        conf_local[j] += 1;
                }
                fprintf(stderr, " %d", i);
            }
        }
        fprintf(stderr, "\n");

        for (i = 0; i < sas->fact_size; ++i){
            if (conf_local[i] > 0)
                conflict[i] += 1;
        }

        BOR_FREE(conf_local);
        return 1;
    }

    ban = BOR_CALLOC_ARR(int, sas->fact_size);
    for (i = 0; i < sas->fact_size; ++i){
        if (fact->neigh[i] > 0 && fs[i] == 0){
            if (setFact(sas->fact + i, fs, 1)){
                num_bounds += pruneEdgesRec(sas, fact, fs, conflict);
                //fprintf(stderr, "[[ %d/%d\n", min_fact_id, fact->id);
                setFact(sas->fact + i, fs, -1);
            }

            ban[i] = 1;
            fs[i] = -1;
        }
    }

    for (i = 0; i < sas->fact_size; ++i){
        if (ban[i])
            fs[i] = 0;
    }

    BOR_FREE(ban);
    return num_bounds;
}

static int pruneEdges(plan_pddl_sas_t *sas, plan_pddl_sas_fact_t *fact)
{
    int *fs, *conflict, i, num_bounds, change = 0;

    fs = BOR_CALLOC_ARR(int, sas->fact_size);
    conflict = BOR_CALLOC_ARR(int, sas->fact_size);

    num_bounds = pruneEdgesRec(sas, fact, fs, conflict);
    fprintf(stderr, "Num bounds: %d\n", num_bounds);
    if (num_bounds == 0){
        for (i = 0; i < sas->fact_size; ++i){
            if (i != fact->id)
                fact->neigh[i] = -1;
        }
        change = 1;

    }else{
        for (i = 0; i < sas->fact_size; ++i){
            if (conflict[i] == num_bounds && fact->neigh[i] >= 0){
                fact->neigh[i] = -1;
                change = 1;
            }
        }
    }

    BOR_FREE(conflict);
    BOR_FREE(fs);

    if (change)
        removeConflictsFromEdges(sas->fact + i);
    return change;
}

static void pFact(const plan_pddl_sas_t *sas, const plan_pddl_sas_fact_t *fact)
{
    int i, j, size = 0;
    for (i = 0; i < sas->fact_size; ++i)
        if (fact->neigh[i] > 0)
            ++size;

    fprintf(stderr, "Fact %d: neigh: %d, conflicts[%d]:",
            fact->id, size, fact->conflict.size);

    for (i = 0; i < sas->fact_size; ++i){
        if (fact->neigh[i] < 0)
            fprintf(stderr, " %d", i);
    }

    fprintf(stderr, ", edges[%d]:", fact->edge_size);
    for (i = 0; i < fact->edge_size; ++i){
        fprintf(stderr, " [");
        for (j = 0; j < fact->edge[i].size; ++j){
            if (j > 0)
                fprintf(stderr, " ");
            fprintf(stderr, "%d", fact->edge[i].fact[j]);
        }
        fprintf(stderr, "]");
    }
    fprintf(stderr, "\n");
}

static void findInvariants(plan_pddl_sas_t *sas)
{
    int i;

    for (i = 0; i < sas->fact_size; ++i){
        uniqueEdges(sas->fact + i);
        pFact(sas, sas->fact + i);
        removeConflictsFromEdges(sas->fact + i);
        pFact(sas, sas->fact + i);
        pruneEdges(sas, sas->fact + i);
        pFact(sas, sas->fact + i);
        fprintf(stderr, "\n");
    }

    exit(-1);
    for (i = 0; i < sas->fact_size; ++i){
        bzero(sas->comp, sizeof(int) * sas->fact_size);
        bzero(sas->close, sizeof(int) * sas->fact_size);
        setFact(sas->fact + i, sas->comp, 1);
        fprintf(stderr, "I %d / %d\n", i, sas->fact_size);
        processFact(sas, i, sas->fact + i, sas->close, sas->comp);
    }
        exit(-1);
}
/*** FIND-INVARIANTS END ***/
#endif





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
