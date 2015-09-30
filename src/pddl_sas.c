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
#include "plan/pddl_sas.h"
#include "plan/causal_graph.h"

struct _inv_t {
    int *fact;
    int size;
    bor_htable_key_t key;
    bor_list_t htable;
};
typedef struct _inv_t inv_t;

/** Computes hash key of the invariant */
_bor_inline bor_htable_key_t invComputeHash(const inv_t *inv);
/** Callbacks for hash table structure */
static bor_htable_key_t invHash(const bor_list_t *k, void *_);
static int invEq(const bor_list_t *k1, const bor_list_t *k2, void *_);
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
    inv_t inv_init;
    int i;

    sas->ground = g;
    sas->fact_size = g->fact_pool.size;
    sas->comp = BOR_ALLOC_ARR(int, sas->fact_size);
    sas->close = BOR_ALLOC_ARR(int, sas->fact_size);
    sas->fact = BOR_CALLOC_ARR(plan_pddl_sas_fact_t, sas->fact_size);
    for (i = 0; i < sas->fact_size; ++i){
        sas->fact[i].id = i;
        sas->fact[i].conflict.fact = BOR_CALLOC_ARR(int, sas->fact_size);
        sas->fact[i].var = PLAN_VAR_ID_UNDEFINED;
        sas->fact[i].val = PLAN_VAL_UNDEFINED;
    }

    bzero(&inv_init, sizeof(inv_init));
    sas->inv_pool = borExtArrNew(sizeof(inv_init), NULL, &inv_init);
    sas->inv_htable = borHTableNew(invHash, invEq, NULL);
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

    borHTableDel(sas->inv_htable);
    for (i = 0; i < sas->inv_size; ++i){
        inv = borExtArrGet(sas->inv_pool, i);
        if (inv->fact != NULL)
            BOR_FREE(inv->fact);
    }
    borExtArrDel(sas->inv_pool);
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
    const inv_t *inv;
    int i, j;

    for (i = 0; i < sas->inv_size; ++i){
        inv = borExtArrGet(sas->inv_pool, i);
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
        //fprintf(fout, "    [%d] ", i);
        fprintf(fout, "    ");
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




_bor_inline bor_htable_key_t invComputeHash(const inv_t *inv)
{
    return borCityHash_64(inv->fact, sizeof(int) * inv->size);
}

static bor_htable_key_t invHash(const bor_list_t *k, void *_)
{
    inv_t *inv = BOR_LIST_ENTRY(k, inv_t, htable);
    return inv->key;
}

static int invEq(const bor_list_t *k1, const bor_list_t *k2, void *_)
{
    const inv_t *inv1 = BOR_LIST_ENTRY(k1, inv_t, htable);
    const inv_t *inv2 = BOR_LIST_ENTRY(k2, inv_t, htable);

    if (inv1->size != inv2->size)
        return 0;
    return memcmp(inv1->fact, inv2->fact, sizeof(int) * inv1->size) == 0;
}

static void sasFactFree(plan_pddl_sas_fact_t *f)
{
    int i;

    if (f->conflict.fact)
        BOR_FREE(f->conflict.fact);

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
    int i;

    f->conflict.size = 0;
    for (i = 0; i < sas->fact_size; ++i){
        if (f->conflict.fact[i])
            f->conflict.fact[f->conflict.size++] = i;
    }

    f->conflict.fact = BOR_REALLOC_ARR(f->conflict.fact, int,
                                       f->conflict.size);
}

static void sasFactAddEdge(plan_pddl_sas_fact_t *f,
                           const plan_pddl_ground_facts_t *fs)
{
    plan_pddl_ground_facts_t *dst;

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
            sas->fact[f1].conflict.fact[f2] = 1;
            sas->fact[f2].conflict.fact[f1] = 1;
        }
    }
}

static void addActionEdges(plan_pddl_sas_t *sas,
                           const plan_pddl_ground_facts_t *del,
                           const plan_pddl_ground_facts_t *add)
{
    plan_pddl_sas_fact_t *fact;
    int i;

    for (i = 0; i < add->size; ++i){
        fact = sas->fact + add->fact[i];
        sasFactAddEdge(fact, del);
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
    for (i = 0; i < f->conflict.size; ++i)
        I[f->conflict.fact[i]] -= val;
    return 1;
}

/** Returns:
 *    - -1 if all facts on edge are in conflict,
 *    -  0 if there is unbound fact on edge, and
 *    -  1 if the edge is already bound. */
static int edgeState(const plan_pddl_ground_facts_t *fs, const int *I)
{
    int i, bound = 0, unbound = 0, val;

    for (i = 0; i < fs->size; ++i){
        val = I[fs->fact[i]];
        if (val == 0){
            unbound += 1;
        }else if (val > 0){
            bound += 1;
        }
    }

    if (bound == 0 && unbound == 0)
        return -1;
    if (bound > 0)
        return 1;
    return 0;
}

/** Returns:
 *    - -2 if all there is an edge that has all facts in conflict,
 *    - -1 if all edges are bound, or
 *    - id>=0 of the first unbound edge */
static int nextUnboundEdge(const plan_pddl_sas_fact_t *f, const int *I)
{
    int i, state;

    for (i = 0; i < f->edge_size; ++i){
        state = edgeState(f->edge + i, I);
        if (state == -1)
            return -2;
        if (state == 0)
            return i;
    }

    return -1;
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

/** Returns true if a new invariant was created */
static int createInvariant(plan_pddl_sas_t *sas, const int *I)
{
    inv_t *inv;
    bor_list_t *ins;
    int i, size;

    // Compute size of the component.
    for (i = 0, size = 0; i < sas->fact_size; ++i){
        if (I[i] > 0)
            ++size;
    }

    // Ignore single fact invariants
    if (size <= 1)
        return 0;

    // Create invariant
    inv = borExtArrGet(sas->inv_pool, sas->inv_size);
    inv->fact = BOR_ALLOC_ARR(int, size);
    inv->size = 0;
    for (i = 0; i < sas->fact_size; ++i){
        if (I[i] > 0)
            inv->fact[inv->size++] = i;
    }
    inv->key = invComputeHash(inv);
    borListInit(&inv->htable);

    // Insert the invariant to the hash table
    ins = borHTableInsertUnique(sas->inv_htable, &inv->htable);
    if (ins != NULL){
        BOR_FREE(inv->fact);
    }else{
        ++sas->inv_size;
    }

    return 1;
}

static int processNextFact(plan_pddl_sas_t *sas, int min_fact_id, int *C, int *I);
static int processFact(plan_pddl_sas_t *sas, int min_fact_id,
                       const plan_pddl_sas_fact_t *f, int *C, int *I)
{
    const plan_pddl_ground_facts_t *fs;
    plan_pddl_sas_fact_t *next_fact;
    int i, eid, ret = 0;

    eid = nextUnboundEdge(f, I);
    if (eid == -1){
        // All edges are bound, close this fact and continue with the next
        // one.
        C[f->id] = 1;
        ret |= processNextFact(sas, min_fact_id, C, I);
        C[f->id] = 0;

    }else if (eid >= 0){
        // There is at least one unbound edge, process it.
        fs = f->edge + eid;
        for (i = 0; i < fs->size; ++i){
            if (fs->fact[i] < min_fact_id)
                continue;

            next_fact = sas->fact + fs->fact[i];

            if (I[next_fact->id] == 0 && setFact(next_fact, I, 1)){
                ret |= processFact(sas, min_fact_id, next_fact, C, I);
                setFact(next_fact, I, -1);
            }
        }
    }

    return ret;
}

static int processNextFact(plan_pddl_sas_t *sas, int min_fact_id,
                           int *C, int *I)
{
    int i, ret = 0;

    // Try to continue with recursion on the next fact that is assigned to
    // the invariant but is not closed yet (hasn't bound all edges).
    for (i = min_fact_id; i < sas->fact_size; ++i){
        if (I[i] > 0 && !C[i])
            return processFact(sas, min_fact_id, sas->fact + i, C, I);
    }

    // Try to continue with the fact that can be added to the invariant,
    // i.e., has already bound an edge but is not added to the invariant.
    for (i = min_fact_id; i < sas->fact_size; ++i){
        if (I[i] == 0 && hasBoundEdge(sas->fact + i, I)){
            if (setFact(sas->fact + i, I, 1)){
                ret |= processFact(sas, min_fact_id, sas->fact + i, C, I);
                setFact(sas->fact + i, I, -1);
            }
        }
    }

    // We don't need to create an invariant in case that some bigger
    // invariant was already created.
    if (ret == 0)
        ret |= createInvariant(sas, I);

    return ret;
}

static void findInvariants(plan_pddl_sas_t *sas)
{
    int i;

    for (i = 0; i < sas->fact_size; ++i){
        bzero(sas->comp, sizeof(int) * sas->fact_size);
        bzero(sas->close, sizeof(int) * sas->fact_size);
        setFact(sas->fact + i, sas->comp, 1);
        processFact(sas, i, sas->fact + i, sas->close, sas->comp);
    }
}
/*** FIND-INVARIANTS END ***/





static int invariantCmp(const void *a, const void *b)
{
    const plan_pddl_ground_facts_t *f1 = a;
    const plan_pddl_ground_facts_t *f2 = b;
    return f2->size - f1->size;
}

static void invariantsToGroundFacts(plan_pddl_sas_t *sas,
                                    plan_pddl_ground_facts_t *fs)
{
    inv_t *inv;
    plan_pddl_ground_facts_t *f;
    int i;

    for (i = 0; i < sas->inv_size; ++i){
        inv = borExtArrGet(sas->inv_pool, i);
        f = fs + i;
        f->size = inv->size;
        f->fact = BOR_ALLOC_ARR(int, inv->size);
        memcpy(f->fact, inv->fact, sizeof(int) * inv->size);
    }

    qsort(fs, sas->inv_size, sizeof(plan_pddl_ground_facts_t),
          invariantCmp);
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
    plan_pddl_ground_facts_t *inv;
    plan_pddl_ground_facts_t one;
    int i;

    if (sas->inv_size == 0)
        return;

    inv = BOR_CALLOC_ARR(plan_pddl_ground_facts_t, sas->inv_size);
    invariantsToGroundFacts(sas, inv);

    while (inv[0].size > 1){
        invariantAddVar(sas, inv);
        invariantRemoveTop(sas, inv);
    }

    one.fact = alloca(sizeof(int));
    one.size = 1;
    for (i = 0; i < sas->fact_size; ++i){
        if (sas->fact[i].var == PLAN_VAR_ID_UNDEFINED){
            one.fact[0] = i;
            invariantAddVar(sas, &one);
        }
    }

    invariantGroundFactsFree(sas, inv);
    BOR_FREE(inv);

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
