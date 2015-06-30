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
#include <boruvka/extarr.h>
#include <boruvka/rbtree.h>
#include "plan/pddl_ground.h"
#include "pddl_err.h"

struct _ground_fact_t {
    plan_pddl_fact_t fact;
    bor_rbtree_node_t node;
};
typedef struct _ground_fact_t ground_fact_t;

struct _ground_fact_pool_t {
    bor_rbtree_t tree;
    bor_extarr_t **fact;
    int *fact_size;
    int size;
};
typedef struct _ground_fact_pool_t ground_fact_pool_t;

static int groundFactCmp(const bor_rbtree_node_t *n1,
                         const bor_rbtree_node_t *n2,
                         void *data)
{
    const ground_fact_t *g1 = bor_container_of(n1, ground_fact_t, node);
    const ground_fact_t *g2 = bor_container_of(n2, ground_fact_t, node);
    const plan_pddl_fact_t *f1 = &g1->fact;
    const plan_pddl_fact_t *f2 = &g2->fact;
    int cmp, i = 0;

    cmp = f1->pred - f2->pred;
    while (cmp == 0 && i < f1->arg_size && i < f2->arg_size)
        cmp = f1->arg[i] - f2->arg[i];
    if (cmp == 0)
        cmp = f1->arg_size - f2->arg_size;

    return cmp;
}

static void groundFactPoolInit(ground_fact_pool_t *gf, int size)
{
    ground_fact_t init_gf;
    int i;

    gf->size = size;
    gf->fact = BOR_ALLOC_ARR(bor_extarr_t *, size);
    bzero(&init_gf, sizeof(init_gf));
    for (i = 0; i < size; ++i)
        gf->fact[i] = borExtArrNew(sizeof(ground_fact_t), NULL, &init_gf);
    gf->fact_size = BOR_CALLOC_ARR(int, size);
    borRBTreeInit(&gf->tree, groundFactCmp, NULL);
}

static void groundFactPoolFree(ground_fact_pool_t *gf)
{
    int i, j;

    borRBTreeFree(&gf->tree);
    for (i = 0; i < gf->size; ++i){
        for (j = 0; j < gf->fact_size[i]; ++j)
            planPDDLFactFree(borExtArrGet(gf->fact[i], j));
        borExtArrDel(gf->fact[i]);
    }
    if (gf->fact != NULL)
        BOR_FREE(gf->fact);
    if (gf->fact_size != NULL)
        BOR_FREE(gf->fact_size);
}

static int groundFactPoolAdd(ground_fact_pool_t *gf, const plan_pddl_fact_t *f)
{
    ground_fact_t *fact;
    bor_rbtree_node_t *node;

    // Get element from array
    fact = borExtArrGet(gf->fact[f->pred], gf->fact_size[f->pred]);
    // and make shallow copy
    fact->fact = *f;

    // Try to insert it into tree
    node = borRBTreeInsert(&gf->tree, &fact->node);
    if (node != NULL)
        return -1;

    // Make deep copy and increase size of array
    planPDDLFactCopy(&fact->fact, f);
    ++gf->fact_size[f->pred];
    return 0;
}

static void addInitFacts(ground_fact_pool_t *pool,
                         const plan_pddl_facts_t *facts)
{
    int i;

    for (i = 0; i < facts->size; ++i)
        groundFactPoolAdd(pool, facts->fact + i);
}

static void gatherFactsFromPool(plan_pddl_facts_t *fs,
                                const ground_fact_pool_t *pool)
{
    const plan_pddl_fact_t *f;
    int pred, i, ins, size;

    size = 0;
    for (pred = 0; pred < pool->size; ++pred)
        size += pool->fact_size[pred];

    fs->size = size;
    fs->fact = BOR_CALLOC_ARR(plan_pddl_fact_t, fs->size);
    ins = 0;
    for (pred = 0; pred < pool->size; ++pred){
        for (i = 0; i < pool->fact_size[pred]; ++i){
            f = borExtArrGet(pool->fact[pred], i);
            planPDDLFactCopy(fs->fact + ins++, f);
        }
    }
}

void planPDDLGround(const plan_pddl_t *pddl, plan_pddl_ground_t *g)
{
    ground_fact_pool_t fact_pool;

    bzero(g, sizeof(*g));
    groundFactPoolInit(&fact_pool, pddl->predicate.size);

    addInitFacts(&fact_pool, &pddl->init_fact);

    g->pddl = pddl;
    gatherFactsFromPool(&g->fact, &fact_pool);
    groundFactPoolFree(&fact_pool);
}

void planPDDLGroundFree(plan_pddl_ground_t *g)
{
    planPDDLFactsFree(&g->fact);
    planPDDLActionsFree(&g->action);
}

void planPDDLGroundPrint(const plan_pddl_ground_t *g, FILE *fout)
{
    int i;

    fprintf(fout, "Facts[%d]:\n", g->fact.size);
    for (i = 0; i < g->fact.size; ++i){
        fprintf(fout, "    ");
        planPDDLFactPrint(&g->pddl->predicate, &g->pddl->obj,
                          g->fact.fact + i, fout);
        fprintf(fout, "\n");
    }
}
