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
#include "plan/pddl_ground_action.h"
#include "pddl_err.h"

struct _ground_action_t {
    plan_pddl_ground_action_t action;
    bor_htable_key_t key;
    bor_list_t htable;
};
typedef struct _ground_action_t ground_action_t;

static bor_htable_key_t groundActionComputeKey(const plan_pddl_ground_action_t *a)
{
    return borHashSDBM(a->name);
}

static bor_htable_key_t groundActionKey(const bor_list_t *key, void *userdata)
{
    const ground_action_t *a = bor_container_of(key, ground_action_t, htable);
    return a->key;
}

static int groundActionEq(const bor_list_t *key1, const bor_list_t *key2,
                          void *userdata)
{
    const ground_action_t *g1 = bor_container_of(key1, ground_action_t, htable);
    const ground_action_t *g2 = bor_container_of(key2, ground_action_t, htable);
    const plan_pddl_ground_action_t *a1 = &g1->action;
    const plan_pddl_ground_action_t *a2 = &g2->action;
    return strcmp(a1->name, a2->name) == 0;
}

void planPDDLGroundActionPoolInit(plan_pddl_ground_action_pool_t *ga,
                                  const plan_pddl_objs_t *objs)
{
    ground_action_t ga_init;

    ga->htable = borHTableNew(groundActionKey, groundActionEq, NULL);
    bzero(&ga_init, sizeof(ga_init));
    ga->action = borExtArrNew(sizeof(ground_action_t), NULL, &ga_init);
    ga->size = 0;
    ga->objs = objs;
}

void planPDDLGroundActionPoolFree(plan_pddl_ground_action_pool_t *ga)
{
    ground_action_t *a;
    int i;

    borHTableDel(ga->htable);
    for (i = 0; i < ga->size; ++i){
        a = borExtArrGet(ga->action, i);
        planPDDLGroundActionFree(&a->action);
    }
    borExtArrDel(ga->action);
}

static void setActionName(char *dstname, const plan_pddl_lift_action_t *lift_action,
                          const int *bound_arg, const plan_pddl_objs_t *objs)
{
    const char *s;
    char *c;
    int i;

    c = dstname;
    for (s = lift_action->action->name; *s != 0x0; ++s)
        *(c++) = *s;

    for (i = 0; i < lift_action->param_size; ++i){
        *(c++) = ' ';
        for (s = objs->obj[bound_arg[i]].name; *s != 0x0; ++s)
            *(c++) = *s;
    }
    *c = 0x0;
}

static void groundFact(plan_pddl_fact_t *out,
                       const plan_pddl_fact_t *fact,
                       const plan_pddl_lift_action_t *lift_action,
                       const int *bound_arg)
{
    int i, var, val;

    planPDDLFactCopy(out, fact);
    for (i = 0; i < fact->arg_size; ++i){
        var = fact->arg[i];
        if (var < 0){
            val = var + lift_action->obj_size;
        }else{
            val = bound_arg[var];
        }
        out->arg[i] = val;
    }
}

static void groundFacts(plan_pddl_facts_t *out,
                        const plan_pddl_facts_t *facts,
                        const plan_pddl_lift_action_t *lift_action,
                        const int *bound_arg)
{
    int i;

    out->size = facts->size;
    out->fact = BOR_CALLOC_ARR(plan_pddl_fact_t, out->size);
    for (i = 0; i < facts->size; ++i)
        groundFact(out->fact + i, facts->fact + i, lift_action, bound_arg);
}

static void groundCondEffs(plan_pddl_cond_effs_t *out,
                           const plan_pddl_cond_effs_t *ce,
                           const plan_pddl_lift_action_t *lift_action,
                           const int *bound_arg)
{
    int i;

    out->size = ce->size;
    out->cond_eff = BOR_CALLOC_ARR(plan_pddl_cond_eff_t, out->size);
    for (i = 0; i < ce->size; ++i){
        groundFacts(&out->cond_eff[i].pre, &ce->cond_eff[i].pre,
                    lift_action, bound_arg);
        groundFacts(&out->cond_eff[i].eff, &ce->cond_eff[i].eff,
                    lift_action, bound_arg);
    }
}

static int checkConflict(const plan_pddl_fact_t *fact1,
                         const plan_pddl_fact_t *fact2)
{
    int i;

    if (fact1->neg == fact2->neg
            || fact1->pred != fact2->pred)
        return 0;

    for (i = 0; i < fact1->arg_size; ++i){
        if (fact1->arg[i] != fact2->arg[i])
            return 0;
    }

    return 1;
}

static int checkConflictInFacts(const plan_pddl_facts_t *facts)
{
    int i, j;

    for (i = 0; i < facts->size; ++i){
        for (j = i + 1; j < facts->size; ++j){
            if (checkConflict(facts->fact + i, facts->fact + j))
                return 1;
        }
    }
    return 0;
}

static int funcEq(const plan_pddl_fact_t *func1,
                  const plan_pddl_fact_t *func2)
{
    return func1->pred == func2->pred
            && memcmp(func1->arg, func2->arg,
                      sizeof(int) * func1->arg_size) == 0;
}

static int groundCostFunc(const plan_pddl_fact_t *func,
                          const plan_pddl_facts_t *init_func)
{
    int i;

    if (func->pred == -1)
        return func->func_val;

    for (i = 0; i < init_func->size; ++i){
        if (funcEq(func, init_func->fact + i))
            return init_func->fact[i].func_val;
    }

    WARN2("Could not find defined cost function. Returning zero.");
    return 0;
}

static int groundCost(const plan_pddl_lift_action_t *lift_action,
                      const int *bound_arg)
{
    const plan_pddl_action_t *a = lift_action->action;
    const plan_pddl_facts_t *cost_facts = &a->cost;
    plan_pddl_facts_t ground_cost_facts;
    int cost = 0;
    int i;

    bzero(&ground_cost_facts, sizeof(ground_cost_facts));
    groundFacts(&ground_cost_facts, cost_facts, lift_action, bound_arg);
    for (i = 0; i < ground_cost_facts.size; ++i){
        cost += groundCostFunc(ground_cost_facts.fact + i, lift_action->func);
    }

    planPDDLFactsFree(&ground_cost_facts);
    return cost;
}

static void factPoolAddActionEffects(plan_pddl_fact_pool_t *pool,
                                     const plan_pddl_ground_action_t *a)
{
    const plan_pddl_facts_t *eff, *pre;
    int i;

    planPDDLFactPoolAddFacts(pool, &a->eff);
    for (i = 0; i < a->cond_eff.size; ++i){
        pre = &a->cond_eff.cond_eff[i].pre;
        eff = &a->cond_eff.cond_eff[i].eff;
        if (planPDDLFactPoolExistFacts(pool, pre))
            planPDDLFactPoolAddFacts(pool, eff);
    }
}

int planPDDLGroundActionPoolAdd(plan_pddl_ground_action_pool_t *ga,
                                const plan_pddl_lift_action_t *lift_action,
                                const int *bound_arg,
                                plan_pddl_fact_pool_t *fact_pool)
{
    ground_action_t *act;
    plan_pddl_ground_action_t *action;
    bor_list_t *node;
    char name[1024];

    act = borExtArrGet(ga->action, ga->size);
    action = &act->action;

    // Construct name from name of the action and its arguments
    setActionName(name, lift_action, bound_arg, ga->objs);

    // Make only shallow copy and check whether the action isn't already in
    // pool
    action->name = name;
    act->key = groundActionComputeKey(action);
    node = borHTableFind(ga->htable, &act->htable);
    if (node != NULL)
        return -1;

    // Now ground all effects and check for conflicts, i.e., whether there
    // are both positive and negative version of the same fact.
    groundFacts(&action->eff, &lift_action->action->eff, lift_action, bound_arg);
    if (checkConflictInFacts(&action->eff)){
        planPDDLFactsFree(&action->eff);
        bzero(action, sizeof(*action));
        return -1;
    }

    // Everything is ok, so make deep copy of the name, ground
    // preconditions and conditional effects, determine cost of the
    // action and copy arguments.
    action->name = BOR_STRDUP(name);
    groundFacts(&action->pre, &lift_action->action->pre, lift_action, bound_arg);
    groundCondEffs(&action->cond_eff, &lift_action->action->cond_eff,
                   lift_action, bound_arg);
    action->cost = groundCost(lift_action, bound_arg);
    action->arg_size = lift_action->param_size;
    action->arg = BOR_ALLOC_ARR(int, action->arg_size);
    memcpy(action->arg, bound_arg, sizeof(int) * action->arg_size);


    // Insert it into hash table
    borListInit(&act->htable);
    borHTableInsert(ga->htable, &act->htable);
    ++ga->size;

    factPoolAddActionEffects(fact_pool, &act->action);
    return 0;
}


plan_pddl_ground_action_t *planPDDLGroundActionPoolGet(
            const plan_pddl_ground_action_pool_t *pool, int i)
{
    ground_action_t *a;

    a = borExtArrGet(pool->action, i);
    return &a->action;
}

void planPDDLGroundActionFree(plan_pddl_ground_action_t *ga)
{
    if (ga->name)
        BOR_FREE(ga->name);
    if (ga->arg)
        BOR_FREE(ga->arg);
    planPDDLFactsFree(&ga->pre);
    planPDDLFactsFree(&ga->eff);
    planPDDLCondEffsFree(&ga->cond_eff);
}

void planPDDLGroundActionsFree(plan_pddl_ground_actions_t *ga)
{
    int i;

    for (i = 0; i < ga->size; ++i)
        planPDDLGroundActionFree(ga->action + i);
    if (ga->action != NULL)
        BOR_FREE(ga->action);
}

void planPDDLGroundActionCopy(plan_pddl_ground_action_t *dst,
                              const plan_pddl_ground_action_t *src)
{
    *dst = *src;
    if (src->name != NULL)
        dst->name = BOR_STRDUP(src->name);
    if (src->arg != NULL){
        dst->arg = BOR_ALLOC_ARR(int, src->arg_size);
        memcpy(dst->arg, src->arg, sizeof(int) * src->arg_size);
    }
    planPDDLFactsCopy(&dst->pre, &src->pre);
    planPDDLFactsCopy(&dst->eff, &src->eff);
    planPDDLCondEffsCopy(&dst->cond_eff, &src->cond_eff);
}

static void printFact(const plan_pddl_fact_t *fact,
                      const plan_pddl_predicates_t *preds,
                      const plan_pddl_objs_t *objs,
                      FILE *fout)
{
    int i;

    fprintf(fout, "        ");
    if (fact->neg)
        fprintf(fout, "N:");
    fprintf(fout, "%s", preds->pred[fact->pred].name);
    for (i = 0; i < fact->arg_size; ++i)
        fprintf(fout, " %s", objs->obj[fact->arg[i]].name);
    fprintf(fout, "\n");
}

static void printFacts(const plan_pddl_facts_t *facts,
                       const plan_pddl_predicates_t *preds,
                       const plan_pddl_objs_t *objs,
                       FILE *fout)
{
    int i;

    for (i = 0; i < facts->size; ++i)
        printFact(facts->fact + i, preds, objs, fout);
}

static void printCondEffs(const plan_pddl_cond_effs_t *ce,
                          const plan_pddl_predicates_t *preds,
                          const plan_pddl_objs_t *objs,
                          FILE *fout)
{
    int i;

    for (i = 0; i < ce->size; ++i){
        fprintf(fout, "      pre[%d]:\n", ce->cond_eff[i].pre.size);
        printFacts(&ce->cond_eff[i].pre, preds, objs, fout);
        fprintf(fout, "      eff[%d]:\n", ce->cond_eff[i].eff.size);
        printFacts(&ce->cond_eff[i].eff, preds, objs, fout);
    }
}

void planPDDLGroundActionPrint(const plan_pddl_ground_action_t *a,
                               const plan_pddl_predicates_t *preds,
                               const plan_pddl_objs_t *objs,
                               FILE *fout)
{
    fprintf(fout, "%s --> %d\n", a->name, a->cost);
    fprintf(fout, "    pre[%d]:\n", a->pre.size);
    printFacts(&a->pre, preds, objs, fout);
    fprintf(fout, "    eff[%d]:\n", a->eff.size);
    printFacts(&a->eff, preds, objs, fout);
    fprintf(fout, "    cond-eff[%d]:\n", a->cond_eff.size);
    printCondEffs(&a->cond_eff, preds, objs, fout);
}
