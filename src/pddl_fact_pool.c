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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#include <boruvka/alloc.h>
#include <boruvka/hfunc.h>
#include "plan/pddl_fact_pool.h"
#include "pddl_err.h"

struct _fact_t {
    int id;
    plan_pddl_fact_t fact;
    bor_htable_key_t key;
    bor_list_t htable;
};
typedef struct _fact_t fact_t;

static bor_htable_key_t factComputeKey(const fact_t *g)
{
    int buf[g->fact.arg_size + 2];
    buf[0] = g->fact.pred;
    buf[1] = g->fact.neg;
    memcpy(buf + 2, g->fact.arg, sizeof(int) * g->fact.arg_size);
    return borFastHash_64(buf, sizeof(int) * (g->fact.arg_size + 2), 123);
}

static bor_htable_key_t factKey(const bor_list_t *key, void *userdata)
{
    const fact_t *f = bor_container_of(key, fact_t, htable);
    return f->key;
}

static int factEq(const bor_list_t *key1, const bor_list_t *key2, void *_)
{
    const fact_t *g1 = bor_container_of(key1, fact_t, htable);
    const fact_t *g2 = bor_container_of(key2, fact_t, htable);
    const plan_pddl_fact_t *f1 = &g1->fact;
    const plan_pddl_fact_t *f2 = &g2->fact;
    int cmp, i;

    cmp = f1->pred - f2->pred;
    if (cmp == 0)
        cmp = f1->neg - f2->neg;
    for (i = 0;cmp == 0 && i < f1->arg_size && i < f2->arg_size; ++i)
        cmp = f1->arg[i] - f2->arg[i];
    if (cmp == 0)
        cmp = f1->arg_size - f2->arg_size;

    return cmp == 0;
}

void planPDDLFactPoolInit(plan_pddl_fact_pool_t *pool, int pred_size)
{
    fact_t init_f;
    int i, init_pred_f;

    pool->htable = borHTableNew(factKey, factEq, NULL);
    bzero(&init_f, sizeof(init_f));
    pool->fact = borExtArrNew(sizeof(fact_t), NULL, &init_f);
    pool->size = 0;

    pool->pred_size = pred_size;
    pool->pred_fact_size = BOR_CALLOC_ARR(int, pool->pred_size);
    pool->pred_fact = BOR_ALLOC_ARR(bor_extarr_t *, pool->pred_size);
    init_pred_f = 0;
    for (i = 0; i < pool->pred_size; ++i)
        pool->pred_fact[i] = borExtArrNew(sizeof(int), NULL, &init_pred_f);
}

void planPDDLFactPoolFree(plan_pddl_fact_pool_t *pool)
{
    fact_t *fact;
    int i;

    borHTableDel(pool->htable);
    for (i = 0; i < pool->size; ++i){
        fact = borExtArrGet(pool->fact, i);
        planPDDLFactFree(&fact->fact);
    }
    borExtArrDel(pool->fact);

    for (i = 0; i < pool->pred_size; ++i)
        borExtArrDel(pool->pred_fact[i]);
    if (pool->pred_fact != NULL)
        BOR_FREE(pool->pred_fact);
    if (pool->pred_fact_size != NULL)
        BOR_FREE(pool->pred_fact_size);
}

int planPDDLFactPoolAdd(plan_pddl_fact_pool_t *pool,
                        const plan_pddl_fact_t *f)
{
    fact_t *fact;
    bor_list_t *node;
    int *by_pred;

    // Get element from array
    fact = borExtArrGet(pool->fact, pool->size);
    fact->id = pool->size;
    // and make shallow copy
    fact->fact = *f;
    fact->key = factComputeKey(fact);
    borListInit(&fact->htable);

    // Try to insert it into tree
    node = borHTableInsertUnique(pool->htable, &fact->htable);
    if (node != NULL){
        fact = bor_container_of(node, fact_t, htable);
        return fact->id;
    }

    // Make deep copy and increase size of array
    planPDDLFactCopy(&fact->fact, f);
    by_pred = borExtArrGet(pool->pred_fact[f->pred],
                           pool->pred_fact_size[f->pred]);
    *by_pred = pool->size;
    ++pool->pred_fact_size[f->pred];
    ++pool->size;
    return fact->id;
}

int planPDDLFactPoolFind(plan_pddl_fact_pool_t *pool,
                         const plan_pddl_fact_t *f)
{
    fact_t *fact;
    bor_list_t *node;

    // Get element from array
    fact = borExtArrGet(pool->fact, pool->size);
    // and make shallow copy which is in this case enough
    fact->fact = *f;
    fact->key = factComputeKey(fact);
    borListInit(&fact->htable);

    node = borHTableFind(pool->htable, &fact->htable);
    if (node == NULL)
        return -1;

    fact = bor_container_of(node, fact_t, htable);
    return fact->id;
}

int planPDDLFactPoolExist(plan_pddl_fact_pool_t *pool,
                          const plan_pddl_fact_t *f)
{
    if (planPDDLFactPoolFind(pool, f) >= 0)
        return 1;
    return 0;
}

plan_pddl_fact_t *planPDDLFactPoolGet(const plan_pddl_fact_pool_t *pool, int i)
{
    fact_t *fact;
    fact = borExtArrGet(pool->fact, i);
    return &fact->fact;
}

plan_pddl_fact_t *planPDDLFactPoolGetPred(const plan_pddl_fact_pool_t *pool,
                                          int pred, int i)
{
    int *fact_id;

    fact_id = borExtArrGet(pool->pred_fact[pred], i);
    return planPDDLFactPoolGet(pool, *fact_id);
}

static int cmpFactIds(const void *a, const void *b, void *ud)
{
    int id1 = *(int *)a;
    int id2 = *(int *)b;
    const plan_pddl_fact_pool_t *fact_pool = ud;
    const plan_pddl_fact_t *f1 = planPDDLFactPoolGet(fact_pool, id1);
    const plan_pddl_fact_t *f2 = planPDDLFactPoolGet(fact_pool, id2);
    int cmp;

    cmp = f1->pred - f2->pred;
    if (cmp == 0)
        cmp = memcmp(f1->arg, f2->arg, sizeof(int) * f1->arg_size);
    if (cmp == 0)
        cmp = f1->neg - f2->neg;

    return cmp;
}

void planPDDLFactPoolCleanup(plan_pddl_fact_pool_t *pool, int *map)
{
    plan_pddl_fact_pool_t old_pool;
    fact_t *fact;
    int *fact_ids;
    int id, i, fid;

    old_pool = *pool;
    fact_ids = BOR_ALLOC_ARR(int, old_pool.size);
    for (i = 0; i < old_pool.size; ++i)
        fact_ids[i] = i;
    qsort_r(fact_ids, old_pool.size, sizeof(int), cmpFactIds, &old_pool);

    planPDDLFactPoolInit(pool, old_pool.pred_size);
    for (i = 0; i < old_pool.size; ++i){
        fid = fact_ids[i];
        fact = borExtArrGet(old_pool.fact, fid);
        if (fact->fact.stat || fact->fact.neg){
            map[fid] = -1;
        }else{
            id = planPDDLFactPoolAdd(pool, &fact->fact);
            map[fid] = id;
        }
    }

    planPDDLFactPoolFree(&old_pool);
    BOR_FREE(fact_ids);
}
