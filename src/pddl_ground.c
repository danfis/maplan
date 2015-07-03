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
#include "plan/pddl_ground.h"
#include "pddl_err.h"

struct _plan_pddl_ground_action_t {
    char *name;
    int *arg;
    int arg_size;
    int cost;
    plan_pddl_facts_t pre;
    plan_pddl_facts_t eff;
    plan_pddl_cond_effs_t cond_eff;
};
typedef struct _plan_pddl_ground_action_t plan_pddl_ground_action_t;

struct _plan_pddl_ground_actions_t {
    plan_pddl_ground_action_t *action;
    int size;
};
typedef struct _plan_pddl_ground_actions_t plan_pddl_ground_actions_t;

struct _ground_action_t {
    plan_pddl_ground_action_t action;
    bor_htable_key_t key;
    bor_list_t htable;
};
typedef struct _ground_action_t ground_action_t;


/** Initializes and frees action pool */
static void groundActionPoolInit(plan_pddl_ground_action_pool_t *ga,
                                 const plan_pddl_objs_t *objs);
static void groundActionPoolFree(plan_pddl_ground_action_pool_t *ga);
/** Adds grounded action into pool if not already there.
 *  If an action was inserted, pointer is returned. Otherwise NULL is
 *  returned. */
static plan_pddl_ground_action_t *groundActionPoolAdd(plan_pddl_ground_action_pool_t *ga,
                                                      const plan_pddl_lift_action_t *lift_action,
                                                      const int *bound_arg);
static void groundActionPoolToArr(const plan_pddl_ground_action_pool_t *ga,
                                  plan_pddl_ground_actions_t *as);
/** Returns i'th action from the pool */
static plan_pddl_ground_action_t *groundActionPoolGetAction(
            const plan_pddl_ground_action_pool_t *pool, int i);


/** Instantiates actions using facts in fact pool. */
static void instActions(const plan_pddl_lift_actions_t *actions,
                        plan_pddl_fact_pool_t *fact_pool,
                        plan_pddl_ground_action_pool_t *action_pool,
                        const plan_pddl_t *pddl);
/** Instantiates negative preconditions of actions and adds them to fact
 *  pool. */
static void instActionsNegativePreconditions(const plan_pddl_lift_actions_t *as,
                                             plan_pddl_fact_pool_t *fact_pool);

void planPDDLGroundActionPrint(const plan_pddl_ground_action_t *a,
                               const plan_pddl_predicates_t *preds,
                               const plan_pddl_objs_t *objs,
                               FILE *fout);

void planPDDLGroundInit(plan_pddl_ground_t *g, const plan_pddl_t *pddl)
{
    bzero(g, sizeof(*g));
    g->pddl = pddl;
    planPDDLFactPoolInit(&g->fact_pool, pddl->predicate.size);
    groundActionPoolInit(&g->action_pool, &pddl->obj);
}

void planPDDLGroundFree(plan_pddl_ground_t *g)
{
    planPDDLFactPoolFree(&g->fact_pool);
    groundActionPoolFree(&g->action_pool);
    planPDDLLiftActionsFree(&g->lift_action);
}

void planPDDLGround(plan_pddl_ground_t *g)
{
    planPDDLLiftActionsInit(&g->lift_action, &g->pddl->action,
                            &g->pddl->type_obj, &g->pddl->init_func,
                            g->pddl->obj.size, g->pddl->predicate.eq_pred);
    planPDDLLiftActionsPrint(&g->lift_action, &g->pddl->predicate, &g->pddl->obj, stdout);

    planPDDLFactPoolAddFacts(&g->fact_pool, &g->pddl->init_fact);
    instActionsNegativePreconditions(&g->lift_action, &g->fact_pool);
    instActions(&g->lift_action, &g->fact_pool, &g->action_pool, g->pddl);
}

void planPDDLGroundPrint(const plan_pddl_ground_t *g, FILE *fout)
{
    const plan_pddl_fact_t *fact;
    const plan_pddl_ground_action_t *action;
    int i;

    fprintf(fout, "Facts[%d]:\n", g->fact_pool.size);
    for (i = 0; i < g->fact_pool.size; ++i){
        fact = planPDDLFactPoolGet(&g->fact_pool, i);
        fprintf(fout, "    ");
        planPDDLFactPrint(&g->pddl->predicate, &g->pddl->obj, fact, fout);
        fprintf(fout, "\n");
    }

    fprintf(fout, "Actions[%d]:\n", g->action_pool.size);
    for (i = 0; i < g->action_pool.size; ++i){
        action = groundActionPoolGetAction(&g->action_pool, i);
        planPDDLGroundActionPrint(action, &g->pddl->predicate,
                                  &g->pddl->obj, fout);
    }
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


/**** GROUND ACTION POOL ****/
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

static void groundActionPoolInit(plan_pddl_ground_action_pool_t *ga,
                                 const plan_pddl_objs_t *objs)
{
    ground_action_t ga_init;

    ga->htable = borHTableNew(groundActionKey, groundActionEq, NULL);
    bzero(&ga_init, sizeof(ga_init));
    ga->action = borExtArrNew(sizeof(ground_action_t), NULL, &ga_init);
    ga->size = 0;
    ga->objs = objs;
}

static void groundActionPoolFree(plan_pddl_ground_action_pool_t *ga)
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

static plan_pddl_ground_action_t *groundActionPoolAdd(plan_pddl_ground_action_pool_t *ga,
                                                      const plan_pddl_lift_action_t *lift_action,
                                                      const int *bound_arg)
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
        return NULL;

    // Now ground all effects and check for conflicts, i.e., whether there
    // are both positive and negative version of the same fact.
    groundFacts(&action->eff, &lift_action->action->eff, lift_action, bound_arg);
    if (checkConflictInFacts(&action->eff)){
        planPDDLFactsFree(&action->eff);
        bzero(action, sizeof(*action));
        return NULL;
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

    return action;
}

static void groundActionPoolToArr(const plan_pddl_ground_action_pool_t *ga,
                                  plan_pddl_ground_actions_t *as)
{
    const ground_action_t *a;
    int i;

    as->size = ga->size;
    as->action = BOR_CALLOC_ARR(plan_pddl_ground_action_t, as->size);
    for (i = 0; i < as->size; ++i){
        a = borExtArrGet(ga->action, i);
        planPDDLGroundActionCopy(as->action + i, &a->action);
    }
}

static plan_pddl_ground_action_t *groundActionPoolGetAction(
            const plan_pddl_ground_action_pool_t *pool, int i)
{
    ground_action_t *a;

    a = borExtArrGet(pool->action, i);
    return &a->action;
}
/**** GROUND ACTION POOL END ****/

/**** INSTANTIATE ****/
static void initBoundArg(int *bound_arg, const int *bound_arg_init, int size)
{
    int i;

    if (bound_arg_init == NULL){
        for (i = 0; i < size; ++i)
            bound_arg[i] = -1;
    }else{
        memcpy(bound_arg, bound_arg_init, sizeof(int) * size);
    }
}

static int _paramIsUsedByFacts(const plan_pddl_facts_t *fs, int param)
{
    int i, j;

    for (i = 0; i < fs->size; ++i){
        for (j = 0; j < fs->fact[i].arg_size; ++j){
            if (fs->fact[i].arg[j] == param)
                return 1;
        }
    }

    return 0;
}

static int paramIsUsedInEff(const plan_pddl_action_t *a, int param)
{
    const plan_pddl_cond_eff_t *cond_eff;
    int k;

    if (_paramIsUsedByFacts(&a->eff, param)
            || _paramIsUsedByFacts(&a->cost, param))
        return 1;

    for (k = 0; k < a->cond_eff.size; ++k){
        cond_eff = a->cond_eff.cond_eff + k;

        if (_paramIsUsedByFacts(&cond_eff->pre, param)
                || _paramIsUsedByFacts(&cond_eff->eff, param))
            return 1;
    }

    return 0;
}

static int checkBoundArgEqFact(const plan_pddl_fact_t *eq,
                               int obj_size,
                               const int *bound_arg)
{
    int i, val[2];

    for (i = 0; i < 2; ++i){
        val[i] = eq->arg[i];
        if (val[i] < 0){
            val[i] = val[i] + obj_size;
        }else{
            val[i] = bound_arg[val[i]];
        }
    }

    return val[0] == val[1];
}

static int checkBoundArgEq(const plan_pddl_lift_action_t *lift_action,
                           const int *bound_arg)
{
    const plan_pddl_fact_t *eq;
    int i, size, obj_size, check;

    obj_size = lift_action->obj_size;
    size = lift_action->eq.size;
    for (i = 0; i < size; ++i){
        eq = lift_action->eq.fact + i;
        check = checkBoundArgEqFact(eq, obj_size, bound_arg);
        if (eq->neg && check){
            return 0;
        }else if (!eq->neg && !check){
            return 0;
        }
    }

    return 1;
}

static int unboundArg(const int *bound_arg, int arg_size)
{
    int arg_i;

    for (arg_i = 0; arg_i < arg_size; ++arg_i){
        if (bound_arg[arg_i] == -1)
            return arg_i;
    }
    return -1;
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
static void instActionBound(const plan_pddl_lift_action_t *lift_action,
                            plan_pddl_fact_pool_t *fact_pool,
                            plan_pddl_ground_action_pool_t *action_pool,
                            const int *bound_arg)
{
    const plan_pddl_ground_action_t *ga;

    // Check (= ...) equality predicate if it holds for the current binding
    // of arguments.
    if (!checkBoundArgEq(lift_action, bound_arg))
        return;

    // Ground action and if successful instatiate grounded
    ga = groundActionPoolAdd(action_pool, lift_action, bound_arg);
    if (ga != NULL)
        factPoolAddActionEffects(fact_pool, ga);
}

static void instActionBoundPre(const plan_pddl_lift_action_t *lift_action,
                               plan_pddl_fact_pool_t *fact_pool,
                               plan_pddl_ground_action_pool_t *action_pool,
                               const int *bound_arg_init)
{
    const plan_pddl_action_t *action = lift_action->action;
    int arg_size = lift_action->param_size;
    int obj_size = lift_action->obj_size;
    int bound_arg[arg_size];
    int i, unbound;

    initBoundArg(bound_arg, bound_arg_init, arg_size);
    unbound = unboundArg(bound_arg, arg_size);
    if (unbound >= 0){
        if (!paramIsUsedInEff(action, unbound)){
            // If parameter is not used anywhere just bound by a first
            // available object
            bound_arg[unbound] = planPDDLLiftActionFirstAllowedObj(lift_action, unbound);
            instActionBoundPre(lift_action, fact_pool, action_pool, bound_arg);

        }else{
            // Bound by all possible objects
            for (i = 0; i < obj_size; ++i){
                if (!lift_action->allowed_arg[unbound][i])
                    continue;
                bound_arg[unbound] = i;
                instActionBoundPre(lift_action, fact_pool, action_pool, bound_arg);
            }
        }

    }else{
        instActionBound(lift_action, fact_pool, action_pool, bound_arg);
    }
}

static int instBoundArg(const plan_pddl_lift_action_t *lift_action,
                        const plan_pddl_fact_t *pre,
                        const plan_pddl_fact_t *fact,
                        const int *bound_arg_init,
                        int *bound_arg)
{
    int arg_size = lift_action->param_size;
    int objs_size = lift_action->obj_size;
    int i, var, val;

    initBoundArg(bound_arg, bound_arg_init, arg_size);
    for (i = 0; i < pre->arg_size; ++i){
        // Get variable ID (or constant ID)
        var = pre->arg[i];
        // and resolve ID to object ID
        if (var < 0){
            val = var + objs_size;
        }else{
            val = bound_arg[var];
        }

        // Check if values in the current argument equals
        if (val >= 0 && val != fact->arg[i])
            return -1;

        if (val == -1){
            if (!lift_action->allowed_arg[var][fact->arg[i]])
                return -1;
            bound_arg[var] = fact->arg[i];
        }
    }

    return 0;
}

static void instAction(const plan_pddl_lift_action_t *lift_action,
                              plan_pddl_fact_pool_t *fact_pool,
                              plan_pddl_ground_action_pool_t *action_pool,
                              const int *bound_arg_init,
                              int pre_i)
{
    const plan_pddl_fact_t *fact, *pre;
    int facts_size, i, bound_arg[lift_action->param_size];

    pre = lift_action->pre.fact + pre_i;
    facts_size = planPDDLFactPoolPredSize(fact_pool, pre->pred);
    for (i = 0; i < facts_size; ++i){
        fact = planPDDLFactPoolGetPred(fact_pool, pre->pred, i);
        // Precondition and the fact must really match
        if (fact->neg != pre->neg)
            continue;

        // Bound action arguments using the current fact
        if (instBoundArg(lift_action, pre, fact, bound_arg_init, bound_arg) != 0)
            continue;

        if (pre_i == lift_action->pre.size - 1){
            // If this was the last precondition then try to instantiate
            // action using bounded arguments.
            instActionBoundPre(lift_action, fact_pool, action_pool, bound_arg);

        }else{
            // Recursion -- bound next precondition
            instAction(lift_action, fact_pool, action_pool,
                       bound_arg, pre_i + 1);
        }
    }
}

static void instActions(const plan_pddl_lift_actions_t *actions,
                        plan_pddl_fact_pool_t *fact_pool,
                        plan_pddl_ground_action_pool_t *action_pool,
                        const plan_pddl_t *pddl)
{
    const plan_pddl_lift_action_t *lift_action;
    int i, num_facts;

    do {
        num_facts = fact_pool->size;
        for (i = 0; i < actions->size; ++i){
            lift_action = actions->action + i;
            instAction(lift_action, fact_pool, action_pool, NULL, 0);
        }
    } while (num_facts != fact_pool->size);
}
/**** INSTANTIATE END ****/

/**** INSTANTIATE NEGATIVE PRECONDITIONS ****/
static void instActionNegPreWrite(const plan_pddl_lift_action_t *lift_action,
                                  const plan_pddl_fact_t *pre,
                                  int *bound_arg,
                                  plan_pddl_fact_pool_t *fact_pool)
{
    plan_pddl_fact_t f;

    // Termination of recursion -- record the grounded fact
    planPDDLFactCopy(&f, pre);
    memcpy(f.arg, bound_arg, sizeof(int) * f.arg_size);

    // Write only those negative facts that are not present in positive
    // form
    f.neg = 0;
    if (!planPDDLFactPoolExist(fact_pool, &f)){
        f.neg = 1;
        planPDDLFactPoolAdd(fact_pool, &f);
    }
    planPDDLFactFree(&f);
}

static void instActionNegPre(const plan_pddl_lift_action_t *lift_action,
                             const plan_pddl_fact_t *pre,
                             int *bound_arg, int arg_i,
                             plan_pddl_fact_pool_t *fact_pool)
{
    int i, var, *allowed;

    if (arg_i == pre->arg_size){
        instActionNegPreWrite(lift_action, pre, bound_arg, fact_pool);
        return;
    }

    var = pre->arg[arg_i];
    if (var < 0){
        bound_arg[arg_i] = var + lift_action->obj_size;
        instActionNegPre(lift_action, pre, bound_arg,
                         arg_i + 1, fact_pool);

    }else{
        allowed = lift_action->allowed_arg[var];
        for (i = 0; i < lift_action->obj_size; ++i){
            if (!allowed[i])
                continue;
            bound_arg[arg_i] = i;
            instActionNegPre(lift_action, pre, bound_arg,
                             arg_i + 1, fact_pool);
        }
    }
}

static void instActionNegPres(const plan_pddl_lift_action_t *lift_action,
                              plan_pddl_fact_pool_t *fact_pool)
{
    const plan_pddl_fact_t *fact;
    int i, *bound_arg;

    for (i = 0; i < lift_action->pre_neg.size; ++i){
        fact = lift_action->pre_neg.fact + i;
        bound_arg = BOR_ALLOC_ARR(int, fact->arg_size);
        instActionNegPre(lift_action, fact, bound_arg, 0, fact_pool);
        BOR_FREE(bound_arg);
    }
}

static void instActionsNegativePreconditions(const plan_pddl_lift_actions_t *as,
                                             plan_pddl_fact_pool_t *fact_pool)
{
    int i;
    for (i = 0; i < as->size; ++i)
        instActionNegPres(as->action + i, fact_pool);
}
/**** INSTANTIATE NEGATIVE PRECONDITIONS END ****/
