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

static void addEdges(plan_pddl_sas_t *sas,
                     const plan_pddl_ground_action_pool_t *pool)
{
    const plan_pddl_ground_action_t *a;
    int i;

    for (i = 0; i < pool->size; ++i){
        a = planPDDLGroundActionPoolGet(pool, i);
        addActionEdges(sas, &a->eff_del, &a->eff_add);
    }
}

void planPDDLSasInit(plan_pddl_sas_t *sas, const plan_pddl_ground_t *g)
{
    sas->fact_size = g->fact_pool.size;
    sas->comp = BOR_ALLOC_ARR(int, sas->fact_size);
    sas->close = BOR_ALLOC_ARR(int, sas->fact_size);
    sas->fact = BOR_CALLOC_ARR(plan_pddl_sas_fact_t, sas->fact_size);

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
