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
#include <boruvka/scc.h>
#include "plan/pddl_ground.h"
#include "pddl_err.h"


/** Instantiates actions using facts in fact pool. */
static void instActions(const plan_pddl_lift_actions_t *actions,
                        plan_pddl_fact_pool_t *fact_pool,
                        plan_pddl_ground_action_pool_t *action_pool,
                        const plan_pddl_t *pddl);
/** Instantiates negative preconditions of actions and adds them to fact
 *  pool. */
static void instActionsNegativePreconditions(const plan_pddl_t *pddl,
                                             const plan_pddl_lift_actions_t *as,
                                             plan_pddl_fact_pool_t *fact_pool);

/** Flip flag of the facts that are static facts.
 *  Static facts are those that are in the init state and are never changed
 *  to negative form. */
static void markStaticFacts(plan_pddl_fact_pool_t *fact_pool,
                            const plan_pddl_facts_t *init_facts)
{
    plan_pddl_fact_t fact, *init_fact, *f;
    int i, id;

    for (i = 0; i < init_facts->size; ++i){
        init_fact = init_facts->fact + i;
        if (init_fact->neg)
            continue;

        planPDDLFactCopy(&fact, init_fact);
        fact.neg = 1;
        if (!planPDDLFactPoolExist(fact_pool, &fact)){
            fact.neg = 0;
            id = planPDDLFactPoolFind(fact_pool, &fact);
            f = planPDDLFactPoolGet(fact_pool, id);
            f->stat = 1;
        }
        planPDDLFactFree(&fact);
    }
}

static void removeStatAndNegFacts(plan_pddl_fact_pool_t *fact_pool,
                                  plan_pddl_ground_action_pool_t *action_pool)
{
    int *map;

    map = BOR_CALLOC_ARR(int, fact_pool->size);
    planPDDLFactPoolCleanup(fact_pool, map);
    planPDDLGroundActionPoolRemap(action_pool, map);
    BOR_FREE(map);
}

void planPDDLGroundInit(plan_pddl_ground_t *g, const plan_pddl_t *pddl)
{
    bzero(g, sizeof(*g));
    g->pddl = pddl;
    planPDDLFactPoolInit(&g->fact_pool, pddl->predicate.size);
    planPDDLGroundActionPoolInit(&g->action_pool, &pddl->predicate, &pddl->obj);
}

void planPDDLGroundFree(plan_pddl_ground_t *g)
{
    planPDDLFactPoolFree(&g->fact_pool);
    planPDDLGroundActionPoolFree(&g->action_pool);
    planPDDLLiftActionsFree(&g->lift_action);
}

void planPDDLGround(plan_pddl_ground_t *g)
{
    planPDDLLiftActionsInit(&g->lift_action, &g->pddl->action,
                            &g->pddl->type_obj, &g->pddl->init_func,
                            g->pddl->obj.size, g->pddl->predicate.eq_pred);

    planPDDLFactPoolAddFacts(&g->fact_pool, &g->pddl->init_fact);
    instActionsNegativePreconditions(g->pddl, &g->lift_action, &g->fact_pool);
    instActions(&g->lift_action, &g->fact_pool, &g->action_pool, g->pddl);
    markStaticFacts(&g->fact_pool, &g->pddl->init_fact);
    planPDDLGroundActionPoolInst(&g->action_pool, &g->fact_pool);
    removeStatAndNegFacts(&g->fact_pool, &g->action_pool);
}

static int printCmpActions(const void *a, const void *b, void *ud)
{
    int id1 = *(int *)a;
    int id2 = *(int *)b;
    const plan_pddl_ground_action_pool_t *ap = ud;
    const plan_pddl_ground_action_t *a1 = planPDDLGroundActionPoolGet(ap, id1);
    const plan_pddl_ground_action_t *a2 = planPDDLGroundActionPoolGet(ap, id2);
    return strcmp(a1->name, a2->name);
}

void planPDDLGroundPrint(const plan_pddl_ground_t *g, FILE *fout)
{
    const plan_pddl_fact_t *fact;
    const plan_pddl_ground_action_t *action;
    int *action_ids;
    int i;

    action_ids = BOR_ALLOC_ARR(int, g->action_pool.size);
    for (i = 0; i < g->action_pool.size; ++i)
        action_ids[i] = i;

    qsort_r(action_ids, g->action_pool.size, sizeof(int), printCmpActions,
            (void *)&g->action_pool);

    fprintf(fout, "Facts[%d]:\n", g->fact_pool.size);
    for (i = 0; i < g->fact_pool.size; ++i){
        fact = planPDDLFactPoolGet(&g->fact_pool, i);
        fprintf(fout, "    ");
        planPDDLFactPrint(&g->pddl->predicate, &g->pddl->obj, fact, fout);
        fprintf(fout, "\n");
    }

    fprintf(fout, "Actions[%d]:\n", g->action_pool.size);
    for (i = 0; i < g->action_pool.size; ++i){
        action = planPDDLGroundActionPoolGet(&g->action_pool, action_ids[i]);
        planPDDLGroundActionPrint(action, &g->fact_pool, &g->pddl->predicate,
                                  &g->pddl->obj, fout);
    }

    BOR_FREE(action_ids);
}



/**** INSTANTIATE ****/
static void initBoundArg(int *bound_arg, const int *bound_arg_init, int size)
{
    int i;

    if (bound_arg_init == NULL){
        for (i = 0; i < size + 1; ++i)
            bound_arg[i] = -1;
    }else{
        memcpy(bound_arg, bound_arg_init, sizeof(int) * (size + 1));
    }
}

_bor_inline int boundOwner(const int *bound_arg, int arg_size)
{
    return bound_arg[arg_size];
}

_bor_inline void boundSetOwner(int *bound_arg, int arg_size, int owner)
{
    bound_arg[arg_size] = owner;
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

static void instActionBound(const plan_pddl_lift_action_t *lift_action,
                            plan_pddl_fact_pool_t *fact_pool,
                            plan_pddl_ground_action_pool_t *action_pool,
                            const int *bound_arg)
{
    // Check (= ...) equality predicate if it holds for the current binding
    // of arguments.
    if (!checkBoundArgEq(lift_action, bound_arg))
        return;

    // Ground action and if successful instatiate grounded
    planPDDLGroundActionPoolAdd(action_pool, lift_action, bound_arg, fact_pool);
}

static int instBoundOwner(const plan_pddl_lift_action_t *lift_action,
                          const plan_pddl_fact_t *fact,
                          int *bound_arg)
{
    int arg_size = lift_action->param_size;
    int owner, bound_owner;

    // Determine the owner object that is forced through action bound
    // argument.
    owner = -1;
    if (lift_action->owner_param >= 0
            && bound_arg[lift_action->owner_param] >= 0)
        owner = bound_arg[lift_action->owner_param];

    // Check that the owner corresponds to the owner of the binding fact
    if (fact != NULL && fact->owner >= 0){
        if (owner >= 0 && fact->owner != owner)
            return -1;
        owner = fact->owner;
    }

    // Check that the owner corresponds to the owner bound in previous
    // steps.
    bound_owner = boundOwner(bound_arg, arg_size);
    if (bound_owner >= 0){
        if (owner >= 0 && owner != bound_owner)
            return -1;
        owner = bound_owner;
    }

    // Bound the owner ID
    boundSetOwner(bound_arg, arg_size, owner);
    return 0;
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

    return instBoundOwner(lift_action, fact, bound_arg);
}

static void instActionBoundPre(const plan_pddl_lift_action_t *lift_action,
                               plan_pddl_fact_pool_t *fact_pool,
                               plan_pddl_ground_action_pool_t *action_pool,
                               const int *bound_arg_init)
{
    const plan_pddl_action_t *action = lift_action->action;
    int arg_size = lift_action->param_size;
    int obj_size = lift_action->obj_size;
    int bound_arg[arg_size + 1];
    int i, unbound, bind_only_first;

    initBoundArg(bound_arg, bound_arg_init, arg_size);
    unbound = unboundArg(bound_arg, arg_size);
    if (unbound >= 0){
        // If parameter is not used anywhere just bound by a first
        // available object
        bind_only_first = !paramIsUsedInEff(action, unbound);

        // Bound by all possible objects if bind_first is not set
        for (i = 0; i < obj_size; ++i){
            if (!lift_action->allowed_arg[unbound][i])
                continue;

            // Try an allowed object ID but first check whether it causes
            // conflict in owners.
            bound_arg[unbound] = i;
            if (instBoundOwner(lift_action, NULL, bound_arg) != 0)
                continue;

            instActionBoundPre(lift_action, fact_pool, action_pool, bound_arg);
            if (bind_only_first)
                break;
        }

    }else{
        instActionBound(lift_action, fact_pool, action_pool, bound_arg);
    }
}

static void instAction(const plan_pddl_lift_action_t *lift_action,
                       plan_pddl_fact_pool_t *fact_pool,
                       plan_pddl_ground_action_pool_t *action_pool,
                       const int *bound_arg_init,
                       int pre_i)
{
    const plan_pddl_fact_t *fact, *pre;
    int facts_size, i, bound_arg[lift_action->param_size + 1];

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

static void instActionFromFact(const plan_pddl_lift_action_t *lift_action,
                               plan_pddl_fact_pool_t *fact_pool,
                               plan_pddl_ground_action_pool_t *action_pool,
                               int fact_id)
{
    const plan_pddl_fact_t *fact, *pre;
    int i, pre_size = lift_action->pre.size;
    int bound_arg[lift_action->param_size + 1];

    fact = planPDDLFactPoolGet(fact_pool, fact_id);
    for (i = 0; i < pre_size; ++i){
        pre = lift_action->pre.fact + i;
        if (pre->pred != fact->pred || pre->neg != fact->neg)
            continue;

        if (instBoundArg(lift_action, pre, fact, NULL, bound_arg) != 0)
            continue;

        instAction(lift_action, fact_pool, action_pool, bound_arg, 0);
    }
}

static void instActions(const plan_pddl_lift_actions_t *actions,
                        plan_pddl_fact_pool_t *fact_pool,
                        plan_pddl_ground_action_pool_t *action_pool,
                        const plan_pddl_t *pddl)
{
    const plan_pddl_lift_action_t *lift_action;
    int i, fact_id, size, num_facts;

    num_facts = fact_pool->size;
    for (i = 0; i < actions->size; ++i){
        lift_action = actions->action + i;
        instAction(lift_action, fact_pool, action_pool, NULL, 0);
    }

    while (num_facts != fact_pool->size){
        fact_id = num_facts;
        size = fact_pool->size;
        num_facts = fact_pool->size;
        for (; fact_id < size; ++fact_id){
            for (i = 0; i < actions->size; ++i){
                lift_action = actions->action + i;
                instActionFromFact(lift_action, fact_pool, action_pool, fact_id);
            }
        }
    }
}
/**** INSTANTIATE END ****/

/**** INSTANTIATE NEGATIVE PRECONDITIONS ****/
static void instActionNegPreWrite(const plan_pddl_t *pddl,
                                  const plan_pddl_lift_action_t *lift_action,
                                  const plan_pddl_fact_t *pre,
                                  int *bound_arg,
                                  plan_pddl_fact_pool_t *fact_pool)
{
    plan_pddl_fact_t f;

    // Termination of recursion -- record the grounded fact
    planPDDLFactCopy(&f, pre);
    memcpy(f.arg, bound_arg, sizeof(int) * f.arg_size);

    // Check if the fact is correct from point of privateness
    if (planPDDLFactSetPrivate(&f, &pddl->predicate, &pddl->obj) < 0){
        planPDDLFactFree(&f);
        return;
    }

    // Write only those negative facts that are not present in positive
    // form
    f.neg = 0;
    if (!planPDDLFactPoolExist(fact_pool, &f)){
        f.neg = 1;
        planPDDLFactPoolAdd(fact_pool, &f);
    }
    planPDDLFactFree(&f);
}

static void instActionNegPre(const plan_pddl_t *pddl,
                             const plan_pddl_lift_action_t *lift_action,
                             const plan_pddl_fact_t *pre,
                             int *bound_arg, int arg_i,
                             plan_pddl_fact_pool_t *fact_pool)
{
    int i, var, *allowed;

    if (arg_i == pre->arg_size){
        instActionNegPreWrite(pddl, lift_action, pre, bound_arg, fact_pool);
        return;
    }

    var = pre->arg[arg_i];
    if (var < 0){
        bound_arg[arg_i] = var + lift_action->obj_size;
        instActionNegPre(pddl, lift_action, pre, bound_arg,
                         arg_i + 1, fact_pool);

    }else{
        allowed = lift_action->allowed_arg[var];
        for (i = 0; i < lift_action->obj_size; ++i){
            if (!allowed[i])
                continue;
            bound_arg[arg_i] = i;
            instActionNegPre(pddl, lift_action, pre, bound_arg,
                             arg_i + 1, fact_pool);
        }
    }
}

static void instActionNegPres(const plan_pddl_t *pddl,
                              const plan_pddl_lift_action_t *lift_action,
                              plan_pddl_fact_pool_t *fact_pool)
{
    const plan_pddl_fact_t *fact;
    int i, *bound_arg;

    for (i = 0; i < lift_action->pre_neg.size; ++i){
        fact = lift_action->pre_neg.fact + i;
        bound_arg = BOR_ALLOC_ARR(int, fact->arg_size);
        instActionNegPre(pddl, lift_action, fact, bound_arg, 0, fact_pool);
        BOR_FREE(bound_arg);
    }
}

static void instActionsNegativePreconditions(const plan_pddl_t *pddl,
                                             const plan_pddl_lift_actions_t *as,
                                             plan_pddl_fact_pool_t *fact_pool)
{
    int i;
    for (i = 0; i < as->size; ++i)
        instActionNegPres(pddl, as->action + i, fact_pool);
}
/**** INSTANTIATE NEGATIVE PRECONDITIONS END ****/
