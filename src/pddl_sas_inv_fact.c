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
#include "plan/pddl_sas_inv_fact.h"

/** Initializes fact structure. */
static void factInit(plan_pddl_sas_inv_fact_t *f, int id, int fact_size);
/** Frees allocated memory. */
static void factFree(plan_pddl_sas_inv_fact_t *f);
/** Adds edges between fact_id fact and all facts listed in the {facts}. */
static void factAddEdges(plan_pddl_sas_inv_fact_t *f,
                         const plan_pddl_ground_facts_t *facts);
/** Adds conflict between f1 and f2 */
static void factsAddConflict(plan_pddl_sas_inv_facts_t *fs, int f1, int f2);
/** Returns true if all facts has bound edges, considering I as invariant
 *  bool array. */
static int factAllEdgesBound(const plan_pddl_sas_inv_fact_t *fact,
                             const int *I);
/** Returns true if any of facts from I is in conflict in any other fact
 *  from I. */
static int factsInConflict(const plan_pddl_sas_inv_facts_t *fs,
                           int fact_id, const int *I);

void planPDDLSasInvFactsInit(plan_pddl_sas_inv_facts_t *fs, int size)
{
    int i;

    fs->fact_size = size;
    fs->fact = BOR_ALLOC_ARR(plan_pddl_sas_inv_fact_t, size);
    for (i = 0; i < size; ++i)
        factInit(fs->fact + i, i, size);
}

void planPDDLSasInvFactsFree(plan_pddl_sas_inv_facts_t *fs)
{
    int i;

    for (i = 0; i < fs->fact_size; ++i)
        factFree(fs->fact + i);
    BOR_FREE(fs->fact);
}

void planPDDLSasInvFactsAddEdges(plan_pddl_sas_inv_facts_t *fs,
                                 const plan_pddl_ground_facts_t *from,
                                 const plan_pddl_ground_facts_t *to)
{
    int i;
    plan_pddl_sas_inv_fact_t *fact;

    for (i = 0; i < from->size; ++i){
        fact = fs->fact + from->fact[i];
        factAddEdges(fact, to);
    }
}

void planPDDLSasInvFactsAddConflicts(plan_pddl_sas_inv_facts_t *fs,
                                     const plan_pddl_ground_facts_t *facts)
{
    int i, j, f1, f2;

    for (i = 0; i < facts->size; ++i){
        f1 = facts->fact[i];
        for (j = i + 1; j < facts->size; ++j){
            f2 = facts->fact[j];
            factsAddConflict(fs, f1, f2);
        }
    }
}

void planPDDLSasInvFactsAddConflictsWithAll(plan_pddl_sas_inv_facts_t *fs,
                                            int fact_id)
{
    int i;

    for (i = 0; i < fs->fact_size; ++i){
        if (i != fact_id)
            factsAddConflict(fs, fact_id, i);
    }
}

int planPDDLSasInvFactsCheckInvariant(const plan_pddl_sas_inv_facts_t *fs,
                                      const int *facts_bool)
{
    int i;

    for (i = 0; i < fs->fact_size; ++i){
        if (!facts_bool[i])
            continue;

        if (factsInConflict(fs, i, facts_bool))
            return 0;
        if (!factAllEdgesBound(fs->fact + i, facts_bool))
            return 0;
    }

    return 1;
}

int planPDDLSasInvFactsCheckInvariant2(const plan_pddl_sas_inv_facts_t *fs,
                                       const int *facts_bool1,
                                       const int *facts_bool2)
{
    int i, *facts, res;

    facts = BOR_ALLOC_ARR(int, fs->fact_size);
    for (i = 0; i < fs->fact_size; ++i)
        facts[i] = facts_bool1[i] | facts_bool2[i];
    res = planPDDLSasInvFactsCheckInvariant(fs, facts);
    BOR_FREE(facts);
    return res;
}

void planPDDLSasInvFactPrint(const plan_pddl_sas_inv_fact_t *fact, FILE *fout)
{
    int i, j;

    fprintf(fout, "Fact %d:", fact->id);

    fprintf(fout, "conflict:");
    for (i = 0; i < fact->fact_size; ++i){
        if (fact->conflict[i])
            fprintf(fout, " %d", i);
    }

    fprintf(fout, ", edge[%d]:", fact->edge_size);
    for (i = 0; i < fact->edge_size; ++i){
        fprintf(fout, " [");
        for (j = 0; j < fact->edge[i].size; ++j){
            if (j != 0)
                fprintf(fout, " ");
            fprintf(fout, "%d", fact->edge[i].fact[j]);
        }
        fprintf(fout, "]");
    }

    fprintf(fout, "\n");
}


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

static void factAddEdges(plan_pddl_sas_inv_fact_t *f,
                         const plan_pddl_ground_facts_t *facts)
{
    plan_pddl_ground_facts_t *dst;

    if (facts->size == 0)
        return;

    ++f->edge_size;
    f->edge = BOR_REALLOC_ARR(f->edge, plan_pddl_ground_facts_t,
                              f->edge_size);
    dst = f->edge + f->edge_size - 1;
    dst->size = facts->size;
    dst->fact = BOR_ALLOC_ARR(int, facts->size);
    memcpy(dst->fact, facts->fact, sizeof(int) * facts->size);
}

static void factsAddConflict(plan_pddl_sas_inv_facts_t *fs, int f1, int f2)
{
    fs->fact[f1].conflict[f2] = 1;
    fs->fact[f2].conflict[f1] = 1;
}

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

static int factsInConflict(const plan_pddl_sas_inv_facts_t *fs,
                           int fact_id, const int *I)
{
    const plan_pddl_sas_inv_fact_t *fact = fs->fact + fact_id;
    int i;

    for (i = 0; i < fs->fact_size; ++i){
        if (I[i] && fact->conflict[i])
            return 1;
    }

    return 0;
}
