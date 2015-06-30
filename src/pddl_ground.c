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
#include <boruvka/rbtree.h>
#include "plan/pddl_ground.h"
#include "pddl_err.h"

struct _ground_fact_t {
    plan_pddl_fact_t *fact;
    bor_rbtree_node_t node;
};
typedef struct _ground_fact_t ground_fact_t;

struct _ground_fact_pool_t {
    bor_rbtree_t tree;
    plan_pddl_facts_t *fact;
    int size;
    ground_fact_t *next_gf;
};
typedef struct _ground_fact_pool_t ground_fact_pool_t;

static int groundFactCmp(const bor_rbtree_node_t *n1,
                         const bor_rbtree_node_t *n2,
                         void *data)
{
    const ground_fact_t *g1 = bor_container_of(n1, ground_fact_t, node);
    const ground_fact_t *g2 = bor_container_of(n2, ground_fact_t, node);
    const plan_pddl_fact_t *f1 = g1->fact;
    const plan_pddl_fact_t *f2 = g2->fact;
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
    borRBTreeInit(&gf->tree, groundFactCmp, NULL);
    gf->fact = BOR_CALLOC_ARR(plan_pddl_facts_t, size);
    gf->size = size;
    gf->next_gf = BOR_CALLOC_ARR(ground_fact_t, 1);
}

static void groundFactPoolFree(ground_fact_pool_t *gf)
{
    bor_rbtree_node_t *node;
    ground_fact_t *f;
    int i;

    for (i = 0; i < gf->size; ++i)
        planPDDLFactsFree(gf->fact + i);
    if (gf->fact != NULL)
        BOR_FREE(gf->fact);
    if (gf->next_gf)
        BOR_FREE(gf->next_gf);

    while (!borRBTreeEmpty(&gf->tree)){
        node = borRBTreeExtractMin(&gf->tree);
        f = bor_container_of(node, ground_fact_t, node);
        BOR_FREE(f);
    }
    borRBTreeFree(&gf->tree);
}

static int groundFactsAdd(ground_fact_pool_t *gf, const plan_pddl_fact_t *f)
{
    bor_rbtree_node_t *node;
    plan_pddl_fact_t *new_f;

    gf->next_gf->fact = (plan_pddl_fact_t *)f;
    node = borRBTreeInsert(&gf->tree, &gf->next_gf->node);
    if (node != NULL)
        return -1;

    new_f = planPDDLFactsAdd(gf->fact + f->pred);
    planPDDLFactCopy(new_f, f);
    gf->next_gf->fact = new_f;
    gf->next_gf = BOR_CALLOC_ARR(ground_fact_t, 1);
    return 0;
}


void planPDDLGround(const plan_pddl_t *pddl, plan_pddl_ground_t *g)
{
    bzero(g, sizeof(*g));
    g->pddl = pddl;
}

void planPDDLGroundFree(plan_pddl_ground_t *g)
{
    planPDDLFactsFree(&g->fact);
    planPDDLActionsFree(&g->action);
}
