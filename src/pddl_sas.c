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

static void sasFactFree(plan_pddl_sas_fact_t *f)
{
    int i;

    if (f->single_edge.fact != NULL)
        BOR_FREE(f->single_edge.fact);

    for (i = 0; i < f->multi_edge_size; ++i){
        if (f->multi_edge[i].fact != NULL)
            BOR_FREE(f->multi_edge[i].fact);
    }
    if (f->multi_edge != NULL)
        BOR_FREE(f->multi_edge);
    if (f->conflict)
        BOR_FREE(f->conflict);
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
}

static void addConflicts(plan_pddl_sas_t *sas,
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

static void addEdges(plan_pddl_sas_t *sas,
                     const plan_pddl_ground_action_pool_t *pool)
{
    const plan_pddl_ground_action_t *a;
    int i;

    for (i = 0; i < pool->size; ++i){
        a = planPDDLGroundActionPoolGet(pool, i);
        addActionEdges(sas, &a->eff_del, &a->eff_add);
        addConflicts(sas, &a->eff_del);
        addConflicts(sas, &a->eff_add);
        addConflicts(sas, &a->pre);
    }
}

void planPDDLSasInit(plan_pddl_sas_t *sas, const plan_pddl_ground_t *g)
{
    int i;

    sas->fact_size = g->fact_pool.size;
    sas->comp = BOR_ALLOC_ARR(int, sas->fact_size);
    sas->close = BOR_ALLOC_ARR(int, sas->fact_size);
    sas->fact = BOR_CALLOC_ARR(plan_pddl_sas_fact_t, sas->fact_size);
    for (i = 0; i < sas->fact_size; ++i){
        sas->fact[i].id = i;
        sas->fact[i].conflict = BOR_CALLOC_ARR(int, sas->fact_size);
    }

    addEdges(sas, &g->action_pool);
}

void planPDDLSasFree(plan_pddl_sas_t *sas)
{
    int i;

    if (sas->comp)
        BOR_FREE(sas->comp);
    if (sas->close)
        BOR_FREE(sas->close);

    for (i = 0; i < sas->fact_size; ++i)
        sasFactFree(sas->fact + i);
    if (sas->fact != NULL)
        BOR_FREE(sas->fact);
}

static void pComp(const plan_pddl_sas_t *sas, const int *comp)
{
    int i;
    fprintf(stdout, "C:");
    for (i = 0; i < sas->fact_size; ++i){
        if (comp[i] != 0)
            fprintf(stdout, " %d:%d", i, comp[i]);
    }
    fprintf(stdout, "\n");
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

static void boundEdgeFromMultiEdge(int fact_id,
                                   const plan_pddl_ground_facts_t *fs,
                                   int *comp, int val)
{
    int i;

    comp[fact_id] += val;
    for (i = 0; i < fs->size; ++i){
        if (fs->fact[i] != fact_id)
            comp[fs->fact[i]] -= val;
    }
}

static int checkSingleEdges(const plan_pddl_ground_facts_t *fs,
                            const int *comp)
{
    int i;

    for (i = 0; i < fs->size; ++i){
        if (comp[fs->fact[i]] < 0)
            return -1;
    }
    return 0;
}

static int checkConflicts(const plan_pddl_sas_t *sas,
                          const plan_pddl_sas_fact_t *fact,
                          const int *comp)
{
    int i;

    for (i = 0; i < sas->fact_size; ++i){
        if (fact->conflict[i] && comp[i] > 0)
            return -1;
    }
    return 0;
}

static void boundConflicts(const plan_pddl_sas_t *sas,
                           const plan_pddl_sas_fact_t *fact,
                           int *comp, int val)
{
    int i;

    for (i = 0; i < sas->fact_size; ++i){
        if (fact->conflict[i])
            comp[i] -= val;
    }
}

static void boundSingleEdges(const plan_pddl_ground_facts_t *fs,
                             int *comp, int val)
{
    int i;

    for (i = 0; i < fs->size; ++i)
        comp[fs->fact[i]] += val;
}

static int processFact(plan_pddl_sas_t *sas,
                       const plan_pddl_sas_fact_t *fact,
                       int *comp, int *close);

static int processNextFact(plan_pddl_sas_t *sas, int *comp, int *close)
{
    int i;

    for (i = 0; i < sas->fact_size; ++i){
        if (comp[i] > 0 && !close[i])
            return processFact(sas, sas->fact + i, comp, close);
    }

    fprintf(stdout, "Invariant:");
    for (i = 0; i < sas->fact_size; ++i){
        if (comp[i] > 0)
            fprintf(stdout, " %d", i);
    }
    fprintf(stdout, "\n");
    return 0;
}

static int processFactMultiEdges(plan_pddl_sas_t *sas,
                                 const plan_pddl_sas_fact_t *fact,
                                 int *comp, int *close, int mi)
{
    const plan_pddl_ground_facts_t *me;
    int ret, i, fact_id;

    // Get next multi edge to split on
    mi = nextMultiEdge(mi, fact, comp);
    if (mi == -1)
        return -1;

    if (mi == fact->multi_edge_size){
        close[fact->id] = 1;
        ret = processNextFact(sas, comp, close);
        close[fact->id] = 0;
        return ret;
    }

    me = fact->multi_edge + mi;
    for (i = 0; i < me->size; ++i){
        fact_id = me->fact[i];
        if (comp[fact_id] != 0)
            continue;

        boundEdgeFromMultiEdge(fact_id, me, comp, 1);
        processFactMultiEdges(sas, fact, comp, close, mi + 1);
        boundEdgeFromMultiEdge(fact_id, me, comp, -1);
    }

    return 0;
}

static int processFact(plan_pddl_sas_t *sas,
                       const plan_pddl_sas_fact_t *fact,
                       int *comp, int *close)
{
    int ret;

    // Check that the single edges can be bound to the current component
    if (checkSingleEdges(&fact->single_edge, comp) != 0)
        return -1;

    // Check that there are no conflicts with the current fact
    if (checkConflicts(sas, fact, comp) != 0)
        return -1;

    // Bound facts on the single edges and conflicts
    boundSingleEdges(&fact->single_edge, comp, 1);
    boundConflicts(sas, fact, comp, 1);

    ret = processFactMultiEdges(sas, fact, comp, close, 0);

    // Unbound facts on the single edges and conflicts
    boundConflicts(sas, fact, comp, -1);
    boundSingleEdges(&fact->single_edge, comp, -1);

    return ret;
}

static void factToSas(plan_pddl_sas_t *sas, int fact_id)
{
    bzero(sas->comp, sizeof(int) * sas->fact_size);
    bzero(sas->close, sizeof(int) * sas->fact_size);
    sas->comp[fact_id] = 1;
    processFact(sas, sas->fact + fact_id, sas->comp, sas->close);
}

void planPDDLSas(plan_pddl_sas_t *sas)
{
    int i;

    for (i = 0; i < sas->fact_size; ++i){
        printf("factToSas(%d)\n", i);
        factToSas(sas, i);
    }
}
