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

struct _inv_t {
    int *fact;
    int size;
    bor_htable_key_t key;
    bor_list_t htable;
};
typedef struct _inv_t inv_t;

static bor_htable_key_t invComputeHash(const inv_t *inv)
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
    if (f->single_edge.fact != NULL)
        BOR_FREE(f->single_edge.fact);

    for (i = 0; i < f->multi_edge_size; ++i){
        if (f->multi_edge[i].fact != NULL)
            BOR_FREE(f->multi_edge[i].fact);
    }
    if (f->multi_edge != NULL)
        BOR_FREE(f->multi_edge);
}

static void sasFactMergeConflict(plan_pddl_sas_fact_t *dst,
                                 const plan_pddl_sas_fact_t *src,
                                 int fact_size)
{
    int i;

    for (i = 0; i < fact_size; ++i)
        dst->conflict.fact[i] |= src->conflict.fact[i];
}

static void sasFactMergeConflicts(plan_pddl_sas_t *sas)
{
    plan_pddl_sas_fact_t *fact, *fact2;
    int i, j;

    for (i = 0; i < sas->fact_size; ++i){
        fact = sas->fact + i;
        for (j = 0; j < fact->single_edge.size; ++j){
            fact2 = sas->fact + fact->single_edge.fact[j];
            sasFactMergeConflict(fact, fact2, sas->fact_size);
        }
    }
}

static void sasFactFinalize(plan_pddl_sas_t *sas,
                            plan_pddl_sas_fact_t *f)
{
    int i, single = 0;

    for (i = 0; i < f->single_edge.size; ++i){
        if (f->conflict.fact[f->single_edge.fact[i]])
            single = 1;
    }

    if (single == 1){
        for (i = 0; i < sas->fact_size; ++i){
            if (i != f->id)
                f->conflict.fact[f->conflict.size++] = i;
        }

    }else{
        for (i = 0; i < sas->fact_size; ++i){
            if (f->conflict.fact[i])
                f->conflict.fact[f->conflict.size++] = i;
        }
    }

    f->conflict.fact = BOR_REALLOC_ARR(f->conflict.fact, int,
                                       f->conflict.size);
}

static void sasFactAddSingleEdge(plan_pddl_sas_fact_t *f,
                                 int fact_id)
{
    plan_pddl_ground_facts_t *fs;

    fs = &f->single_edge;
    ++fs->size;
    fs->fact = BOR_REALLOC_ARR(fs->fact, int, fs->size);
    fs->fact[fs->size - 1] = fact_id;
}

static void sasFactAddMultiEdge(plan_pddl_sas_fact_t *f,
                                const plan_pddl_ground_facts_t *fs)
{
    plan_pddl_ground_facts_t *dst;

    ++f->multi_edge_size;
    f->multi_edge = BOR_REALLOC_ARR(f->multi_edge,
                                    plan_pddl_ground_facts_t,
                                    f->multi_edge_size);
    dst = f->multi_edge + f->multi_edge_size - 1;
    dst->size = fs->size;
    dst->fact = BOR_ALLOC_ARR(int, fs->size);
    memcpy(dst->fact, fs->fact, sizeof(int) * fs->size);
}

static void addActionEdges(plan_pddl_sas_t *sas,
                           const plan_pddl_ground_facts_t *del,
                           const plan_pddl_ground_facts_t *add)
{
    plan_pddl_sas_fact_t *fact;
    int i;

    for (i = 0; i < del->size; ++i){
        fact = sas->fact + del->fact[i];

        if (add->size == 1){
            sasFactAddSingleEdge(fact, add->fact[0]);
        }else{
            sasFactAddMultiEdge(fact, add);
        }
    }

    for (i = 0; i < add->size; ++i)
        ++sas->fact[add->fact[i]].in_rank;
}

static void addConflicts(plan_pddl_sas_t *sas,
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

static void setSingleFact(plan_pddl_sas_fact_t *fact, int fact_size)
{
    int i;

    for (i = 0; i < fact_size; ++i){
        if (fact->id != i)
            fact->conflict.fact[i] = 1;
    }
}

static void setSingleFacts(plan_pddl_sas_t *sas,
                           const plan_pddl_ground_facts_t *fs)
{
    int i;

    for (i = 0; i < fs->size; ++i)
        setSingleFact(sas->fact + fs->fact[i], sas->fact_size);
}

static void processAction(plan_pddl_sas_t *sas,
                          const plan_pddl_ground_facts_t *pre,
                          const plan_pddl_ground_facts_t *pre_neg,
                          const plan_pddl_ground_facts_t *eff_add,
                          const plan_pddl_ground_facts_t *eff_del)
{
    addActionEdges(sas, eff_del, eff_add);
    addConflicts(sas, eff_del);
    addConflicts(sas, eff_add);
    addConflicts(sas, pre);
    if (eff_del->size == 0)
        setSingleFacts(sas, eff_add);
    if (eff_add->size == 0)
        setSingleFacts(sas, eff_del);
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

static void processInit(plan_pddl_sas_t *sas,
                        const plan_pddl_facts_t *init_fact,
                        const plan_pddl_fact_pool_t *fact_pool)
{
    plan_pddl_ground_facts_t gfs;
    const plan_pddl_fact_t *f;
    int i, id;

    gfs.size = 0;
    gfs.fact = BOR_ALLOC_ARR(int, init_fact->size);
    for (i = 0; i < init_fact->size; ++i){
        f = init_fact->fact + i;
        id = planPDDLFactPoolFind((plan_pddl_fact_pool_t *)fact_pool, f);
        if (id >= 0){
            gfs.fact[gfs.size++] = id;
        }
    }
    addConflicts(sas, &gfs);

    BOR_FREE(gfs.fact);
}


void planPDDLSasInit(plan_pddl_sas_t *sas, const plan_pddl_ground_t *g)
{
    inv_t inv_init;
    int i;

    sas->fact_size = g->fact_pool.size;
    sas->comp = BOR_ALLOC_ARR(int, sas->fact_size);
    sas->close = BOR_ALLOC_ARR(int, sas->fact_size);
    sas->fact = BOR_CALLOC_ARR(plan_pddl_sas_fact_t, sas->fact_size);
    for (i = 0; i < sas->fact_size; ++i){
        sas->fact[i].id = i;
        sas->fact[i].conflict.fact = BOR_CALLOC_ARR(int, sas->fact_size);
        sas->fact[i].var = -1;
        sas->fact[i].val = -1;
    }

    bzero(&inv_init, sizeof(inv_init));
    sas->inv_pool = borExtArrNew(sizeof(inv_init), NULL, &inv_init);
    sas->inv_htable = borHTableNew(invHash, invEq, NULL);
    sas->inv_size = 0;

    processActions(sas, &g->action_pool);
    processInit(sas, &g->pddl->init_fact, &g->fact_pool);
    sasFactMergeConflicts(sas);
    for (i = 0; i < sas->fact_size; ++i)
        sasFactFinalize(sas, sas->fact + i);

    sas->var_range = NULL;
    sas->var_size = 0;
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
}

static int checkFact(const plan_pddl_sas_fact_t *fact,
                     const int *comp)
{
    int i, fact_id;

    if (comp[fact->id] < 0)
        return -1;

    for (i = 0; i < fact->single_edge.size; ++i){
        if (comp[fact->single_edge.fact[i]] < 0)
            return -1;
    }

    for (i = 0; i < fact->conflict.size; ++i){
        fact_id = fact->conflict.fact[i];
        if (comp[fact_id] > 0)
            return -1;
    }

    return 0;
}

static int setFact(const plan_pddl_sas_fact_t *fact,
                   int *comp, int val)
{
    int i, fact_id;

    comp[fact->id] += val;
    for (i = 0; i < fact->single_edge.size; ++i)
        comp[fact->single_edge.fact[i]] += val;

    for (i = 0; i < fact->conflict.size; ++i){
        fact_id = fact->conflict.fact[i];
        comp[fact_id] -= val;
    }

    return 0;
}

static void analyzeMultiEdge(const plan_pddl_ground_facts_t *me,
                             const int *comp,
                             int *num_bound, int *num_unbound)
{
    int i, fact;

    *num_bound = *num_unbound = 0;
    for (i = 0; i < me->size; ++i){
        fact = me->fact[i];
        if (comp[fact] > 0){
            *num_bound += 1;
        }else if (comp[fact] == 0){
            *num_unbound += 1;
        }
    }
}

static int nextMultiEdge(int mi, const plan_pddl_sas_fact_t *f,
                         const int *comp)
{
    int num_bound, num_unbound;

    for (; mi < f->multi_edge_size; ++mi){
        analyzeMultiEdge(f->multi_edge + mi, comp,
                         &num_bound, &num_unbound);
        if (num_bound > 1){
            // This is conflicting multi edge where two edges should be
            // chosen at once
            return -1;
        }

        if (num_bound == 1){
            // This means that one edge is chosen automatically because it
            // was already chosen in upper level.
            continue;
        }

        if (num_unbound == 0){
            // This means that all edges are forbidden.
            return -1;
        }

        // If we got here, it means that multi-edge is not automatically
        // bound but there are some edges that can be bound.
        return mi;
    }

    return mi;
}

static void updateInRank(const plan_pddl_sas_fact_t *fact,
                         const int *comp, int *in_rank)
{
    const plan_pddl_ground_facts_t *me;
    int i, j;

    for (i = 0; i < fact->single_edge.size; ++i)
        ++in_rank[fact->single_edge.fact[i]];

    for (i = 0; i < fact->multi_edge_size; ++i){
        me = fact->multi_edge + i;
        for (j = 0; j < me->size; ++j){
            if (comp[me->fact[j]] > 0){
                ++in_rank[me->fact[j]];
                break;
            }
        }
    }
}

static int checkInvariant(const plan_pddl_sas_t *sas, const int *comp)
{
    int i, *in_rank, ret = 0;

    in_rank = BOR_CALLOC_ARR(int, sas->fact_size);
    for (i = 0; i < sas->fact_size; ++i){
        if (comp[i] > 0){
            updateInRank(sas->fact + i, comp, in_rank);
        }
    }

    for (i = 0; i < sas->fact_size; ++i){
        if (comp[i] > 0 && in_rank[i] != sas->fact[i].in_rank){
            ret = -1;
            break;
        }
    }

    BOR_FREE(in_rank);

    return ret;
}

static void addInvariant(plan_pddl_sas_t *sas, const int *comp)
{
    inv_t *inv;
    bor_list_t *ins;
    int i, size;

    if (checkInvariant(sas, comp) != 0)
        return;

    for (i = 0, size = 0; i < sas->fact_size; ++i){
        if (comp[i] > 0)
            ++size;
    }

    if (size <= 1)
        return;

    inv = borExtArrGet(sas->inv_pool, sas->inv_size);
    inv->fact = BOR_ALLOC_ARR(int, size);
    inv->size = 0;
    for (i = 0; i < sas->fact_size; ++i){
        if (comp[i] > 0)
            inv->fact[inv->size++] = i;
    }
    inv->key = invComputeHash(inv);
    borListInit(&inv->htable);

    ins = borHTableInsertUnique(sas->inv_htable, &inv->htable);
    if (ins != NULL){
        BOR_FREE(inv->fact);
    }else{
        ++sas->inv_size;
    }
}

static void processFact(plan_pddl_sas_t *sas,
                        const plan_pddl_sas_fact_t *fact,
                        int *comp, int *close);

static void processNextFact(plan_pddl_sas_t *sas, int *comp, int *close)
{
    int i;

    for (i = 0; i < sas->fact_size; ++i){
        if (comp[i] > 0 && !close[i]){
            processFact(sas, sas->fact + i, comp, close);
            return;
        }
    }

    addInvariant(sas, comp);
}

static void processFactMultiEdges(plan_pddl_sas_t *sas,
                                  const plan_pddl_sas_fact_t *fact,
                                  int *comp, int *close, int mi)
{
    const plan_pddl_ground_facts_t *me;
    int i, fact_id;

    // Get next multi edge to split on
    mi = nextMultiEdge(mi, fact, comp);
    if (mi == -1)
        return;

    if (mi == fact->multi_edge_size){
        close[fact->id] = 1;
        processNextFact(sas, comp, close);
        close[fact->id] = 0;
        return;
    }

    me = fact->multi_edge + mi;
    for (i = 0; i < me->size; ++i){
        fact_id = me->fact[i];
        if (comp[fact_id] != 0)
            continue;

        if (checkFact(sas->fact + fact_id, comp) != 0)
            continue;

        setFact(sas->fact + fact_id, comp, 1);
        processFactMultiEdges(sas, fact, comp, close, mi + 1);
        setFact(sas->fact + fact_id, comp, -1);
    }
}

static void processFact(plan_pddl_sas_t *sas,
                        const plan_pddl_sas_fact_t *fact,
                        int *comp, int *close)
{
    if (checkFact(fact, comp) != 0)
        return;

    setFact(fact, comp, 1);
    processFactMultiEdges(sas, fact, comp, close, 0);
    setFact(fact, comp, -1);
}

static void factToSas(plan_pddl_sas_t *sas, int fact_id)
{
    bzero(sas->comp, sizeof(int) * sas->fact_size);
    bzero(sas->close, sizeof(int) * sas->fact_size);
    sas->comp[fact_id] = 1;
    processFact(sas, sas->fact + fact_id, sas->comp, sas->close);
}


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
    sas->var_range = BOR_REALLOC_ARR(sas->var_range, int, sas->var_size);
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
        if (sas->fact[i].var == -1){
            one.fact[0] = i;
            invariantAddVar(sas, &one);
        }
    }

    invariantGroundFactsFree(sas, inv);
    BOR_FREE(inv);
}

void planPDDLSas(plan_pddl_sas_t *sas)
{
    int i;

    for (i = 0; i < sas->fact_size; ++i)
        factToSas(sas, i);
    invariantToVar(sas);
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
            fprintf(fout, "\n");
        }
    }
}
