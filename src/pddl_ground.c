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
#include <boruvka/htable.h>
#include <boruvka/hfunc.h>
#include "plan/pddl_ground.h"
#include "pddl_err.h"

struct _ground_fact_t {
    plan_pddl_fact_t fact;
    uint64_t key;
    bor_list_t htable;
};
typedef struct _ground_fact_t ground_fact_t;

struct _ground_fact_pool_t {
    bor_htable_t *htable;
    bor_extarr_t **fact;
    int *fact_size;
    int size;
};
typedef struct _ground_fact_pool_t ground_fact_pool_t;

struct _lift_action_t {
    int action;
    plan_pddl_facts_t pre;
    plan_pddl_facts_t pre_neg;
    plan_pddl_facts_t eq;
};
typedef struct _lift_action_t lift_action_t;

struct _lift_actions_t {
    lift_action_t *action;
    int size;
};
typedef struct _lift_actions_t lift_actions_t;

uint64_t groundFactComputeKey(const ground_fact_t *g)
{
    uint64_t key;
    uint32_t *k = (uint32_t *)&key;
    k[0] = (g->fact.pred << 1) | g->fact.neg;
    k[1] = borFastHash_32(g->fact.arg, sizeof(int) * g->fact.arg_size, 123);
    return key;
}
uint64_t groundFactKey(const bor_list_t *key, void *userdata)
{
    const ground_fact_t *f = bor_container_of(key, ground_fact_t, htable);
    return f->key;
}

int groundFactEq(const bor_list_t *key1, const bor_list_t *key2,
                 void *userdata)
{
    const ground_fact_t *g1 = bor_container_of(key1, ground_fact_t, htable);
    const ground_fact_t *g2 = bor_container_of(key2, ground_fact_t, htable);
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
    gf->htable = borHTableNew(groundFactKey, groundFactEq, NULL);
}

static void groundFactPoolFree(ground_fact_pool_t *gf)
{
    int i, j;

    borHTableDel(gf->htable);
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
    bor_list_t *node;

    // Get element from array
    fact = borExtArrGet(gf->fact[f->pred], gf->fact_size[f->pred]);
    // and make shallow copy
    fact->fact = *f;
    fact->key = groundFactComputeKey(fact);
    borListInit(&fact->htable);

    // Try to insert it into tree
    node = borHTableInsertUnique(gf->htable, &fact->htable);
    if (node != NULL)
        return -1;

    // Make deep copy and increase size of array
    planPDDLFactCopy(&fact->fact, f);
    ++gf->fact_size[f->pred];
    return 0;
}

static int liftActionPreCmp(const void *a, const void *b)
{
    const plan_pddl_fact_t *f1 = a;
    const plan_pddl_fact_t *f2 = b;
    int cmp, i;

    cmp = f1->arg_size - f2->arg_size;
    if (cmp == 0)
        cmp = f1->pred - f2->pred;
    for (i = 0; cmp == 0 && i < f1->arg_size; ++i)
        cmp = f1->arg[i] - f2->arg[i];
    return cmp;
}

static void liftActionPrepare(lift_action_t *a)
{
    qsort(a->pre.fact, a->pre.size, sizeof(plan_pddl_fact_t),
          liftActionPreCmp);
}

static void liftActionsInit(lift_actions_t *action,
                            const plan_pddl_actions_t *pddl_action,
                            int eq_fact_id)
{
    lift_action_t *a;
    const plan_pddl_action_t *pddl_a;
    plan_pddl_fact_t *f;
    const plan_pddl_fact_t *pddl_f;
    int i, prei;

    action->size = pddl_action->size;
    action->action = BOR_CALLOC_ARR(lift_action_t, action->size);

    for (i = 0; i < action->size; ++i){
        a = action->action + i;
        a->action = i;

        pddl_a = pddl_action->action + i;
        for (prei = 0; prei < pddl_a->pre.size; ++prei){
            pddl_f = pddl_a->pre.fact + prei;
            if (pddl_f->pred == eq_fact_id){
                f = planPDDLFactsAdd(&a->eq);
            }else if (pddl_f->neg){
                f = planPDDLFactsAdd(&a->pre_neg);
            }else{
                f = planPDDLFactsAdd(&a->pre);
            }
            planPDDLFactCopy(f, pddl_f);
        }

        liftActionPrepare(a);
    }
}

static void liftActionsFree(lift_actions_t *as)
{
    int i;

    for (i = 0; i < as->size; ++i){
        planPDDLFactsFree(&as->action[i].pre);
        planPDDLFactsFree(&as->action[i].pre_neg);
        planPDDLFactsFree(&as->action[i].eq);
    }

    if (as->action != NULL)
        BOR_FREE(as->action);
}

static void _liftActionFactsPrint(const lift_action_t *a,
                                  const plan_pddl_facts_t *fs,
                                  const plan_pddl_actions_t *action,
                                  const plan_pddl_predicates_t *pred,
                                  const plan_pddl_objs_t *obj,
                                  const char *header,
                                  FILE *fout)
{
    int i, j;

    fprintf(fout, "    %s[%d]:\n", header, fs->size);
    for (i = 0; i < fs->size; ++i){
        fprintf(fout, "      ");
        if (fs->fact[i].neg)
            fprintf(fout, "N:");
        fprintf(fout, "%s:", pred->pred[fs->fact[i].pred].name);
        for (j = 0; j < fs->fact[i].arg_size; ++j){
            if (fs->fact[i].arg[j] < 0){
                fprintf(fout, " %s", obj->obj[fs->fact[i].arg[j] + obj->size].name);
            }else{
                fprintf(fout, " %s",
                        action->action[a->action].param.obj[fs->fact[i].arg[j]].name);
            }
        }
        fprintf(fout, "\n");
    }
}

static void liftActionsPrint(const lift_actions_t *a,
                             const plan_pddl_actions_t *action,
                             const plan_pddl_predicates_t *pred,
                             const plan_pddl_objs_t *obj,
                             FILE *fout)
{
    const lift_action_t *lift;
    int i;

    fprintf(fout, "Lift Actions[%d]:\n", a->size);
    for (i = 0; i < a->size; ++i){
        lift = a->action + i;
        fprintf(fout, "  %s:\n", action->action[lift->action].name);
        _liftActionFactsPrint(lift, &lift->pre, action, pred, obj, "pre", fout);
        _liftActionFactsPrint(lift, &lift->pre_neg, action, pred, obj, "pre-neg", fout);
        _liftActionFactsPrint(lift, &lift->eq, action, pred, obj, "eq", fout);
    }
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
    lift_actions_t lift_actions;

    bzero(g, sizeof(*g));
    groundFactPoolInit(&fact_pool, pddl->predicate.size);
    liftActionsInit(&lift_actions, &pddl->action, pddl->predicate.eq_pred);
    liftActionsPrint(&lift_actions, &pddl->action,
                     &pddl->predicate, &pddl->obj, stdout);

    addInitFacts(&fact_pool, &pddl->init_fact);

    g->pddl = pddl;
    gatherFactsFromPool(&g->fact, &fact_pool);
    liftActionsFree(&lift_actions);
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
