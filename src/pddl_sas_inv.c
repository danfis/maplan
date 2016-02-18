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
#include "plan/pddl_sas_inv.h"

/** Stores fact IDs from src to dst */
static void storeFactIds(plan_pddl_ground_facts_t *dst,
                         plan_pddl_fact_pool_t *fact_pool,
                         const plan_pddl_facts_t *src);
/** Process all actions and store all needed informations. */
static void processActions(plan_pddl_sas_inv_finder_t *invf,
                           const plan_pddl_ground_action_pool_t *pool);

/** Creates a new invariant from the facts stored in binary array */
static plan_pddl_sas_inv_t *invNew(const int *fact, int fact_size);
/** Frees allocated memory */
static void invDel(plan_pddl_sas_inv_t *inv);
/** Returns true if inv is subset of inv2 */
static int invIsSubset(const plan_pddl_sas_inv_t *inv,
                       const plan_pddl_sas_inv_t *inv2);
/** Returns true if the invariants have at least one common fact */
static int invHasCommonFact(const plan_pddl_sas_inv_t *inv,
                            const plan_pddl_sas_inv_t *inv2);

static int refineNodes(plan_pddl_sas_inv_nodes_t *ns);


static int createInvariant(plan_pddl_sas_inv_finder_t *invf,
                           const plan_pddl_sas_inv_node_t *node);
/** Try to merge invariants to get the biggest possible invariants */
static void mergeInvariants(plan_pddl_sas_inv_finder_t *invf);
/** Remove invariants that are subsets of other invariant. */
static void pruneInvSubsets(plan_pddl_sas_inv_finder_t *invf);


static bor_htable_key_t invTableHashCompute(const plan_pddl_sas_inv_t *inv);
static bor_htable_key_t invTableHash(const bor_list_t *key, void *ud);
static int invTableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud);


void planPDDLSasInvFinderInit(plan_pddl_sas_inv_finder_t *invf,
                              const plan_pddl_ground_t *g)
{
    bzero(invf, sizeof(*invf));

    planPDDLSasInvFactsInit(&invf->facts, g->fact_pool.size);
    borListInit(&invf->inv);
    invf->inv_size = 0;
    invf->inv_table = borHTableNew(invTableHash, invTableEq, NULL);
    invf->g = g;

    storeFactIds(&invf->fact_init, (plan_pddl_fact_pool_t *)&g->fact_pool,
                 &g->pddl->init_fact);
    processActions(invf, &g->action_pool);
    planPDDLSasInvFactsAddConflicts(&invf->facts, &invf->fact_init);
}

void planPDDLSasInvFinderFree(plan_pddl_sas_inv_finder_t *invf)
{
    plan_pddl_sas_inv_t *inv;
    bor_list_t *item;

    planPDDLSasInvFactsFree(&invf->facts);
    if (invf->fact_init.fact)
        BOR_FREE(invf->fact_init.fact);

    if (invf->inv_table)
        borHTableDel(invf->inv_table);

    while (!borListEmpty(&invf->inv)){
        item = borListNext(&invf->inv);
        borListDel(item);
        inv = BOR_LIST_ENTRY(item, plan_pddl_sas_inv_t, list);
        invDel(inv);
    }
}

#include <boruvka/timer.h>
void planPDDLSasInvFinder(plan_pddl_sas_inv_finder_t *invf)
{
    plan_pddl_sas_inv_nodes_t nodes;
    int i;

    bor_timer_t timer;
    {
        plan_lp_t *lp;
        int i;

        borTimerStart(&timer);
        fprintf(stderr, "%lx %d\n", (long)invf->g, invf->g->action_pool.size);
        lp = planLPNew(2 * invf->g->action_pool.size, invf->facts.fact_size, PLAN_LP_MAX);
        for (i = 0; i < invf->facts.fact_size; ++i){
            planLPSetObj(lp, i, 1.);
            planLPSetVarRange(lp, i, 0, 1);
            planLPSetVarInt(lp, i);
        }

        for (i = 0; i < invf->g->action_pool.size; ++i){
            const plan_pddl_ground_action_t *a;
            int j;
            a = planPDDLGroundActionPoolGet(&invf->g->action_pool, i);
            for (j = 0; j < a->eff_add.size; ++j)
                planLPSetCoef(lp, 2 * i, a->eff_add.fact[j], 1.);
            for (j = 0; j < a->eff_del.size; ++j)
                planLPSetCoef(lp, 2 * i, a->eff_del.fact[j], -1.);
            planLPSetRHS(lp, 2 * i, 0., 'L');

            for (j = 0; j < a->eff_del.size; ++j)
                planLPSetCoef(lp, 2 * i + 1, a->eff_del.fact[j], 1.);
            planLPSetRHS(lp, 2 * i + 1, 1., 'L');
        }

        int j, add = 2 * i;
        double rhs = 2.;
        char sense = 'G';
        /*
        planLPAddRows(lp, 1, &rhs, &sense);
        for (j = 0; j < invf->facts.fact_size; ++j)
            planLPSetCoef(lp, add, j, 1.);
        ++add;
        */

        rhs = 1.;
        sense = 'L';
        planLPAddRows(lp, 1, &rhs, &sense);
        for (j = 0; j < invf->fact_init.size; ++j){
            planLPSetCoef(lp, add, invf->fact_init.fact[j], 1.);
            /*
            int k;
            for (k = j + 1; k < invf->fact_init.size; ++k){
                planLPAddRows(lp, 1, &rhs, &sense);
                planLPSetCoef(lp, add, invf->fact_init.fact[j], 1.);
                planLPSetCoef(lp, add, invf->fact_init.fact[k], 1.);
                ++add;
            }
            */
        }
        ++add;

        double sol[invf->facts.fact_size];
        double z;
        while (1){
        if (planLPSolve(lp, &z, sol) != 0)
            break;
        if (z < 1.1)
            break;

        fprintf(stderr, "Z: %lf :: ", z);
        for (i = 0; i < invf->facts.fact_size; ++i){
            //fprintf(stderr, "S[%d]: %lf\n", i, sol[i]);
            if (sol[i] > 0.){
                plan_pddl_fact_t *f;
                f = planPDDLFactPoolGet(&invf->g->fact_pool, i);
                fprintf(stderr, " ");
                planPDDLFactPrint(&invf->g->pddl->predicate,
                                  &invf->g->pddl->obj,
                                  f, stderr);
                /*
                fprintf(stderr, "    ");
                planPDDLFactPrint(&invf->g->pddl->predicate,
                                  &invf->g->pddl->obj,
                                  f, stderr);
                fprintf(stderr, "\n");
                */
            }
        }
        fprintf(stderr, "\n");
        //z = z - 0.5;
        sense = 'G';
        double zero = 0.;
        planLPAddRows(lp, 1, &zero, &sense);
        for (i = 0; i < invf->facts.fact_size; ++i){
            if (sol[i] > 0.)
                planLPSetCoef(lp, add, i, -1.);
            else
                planLPSetCoef(lp, add, i, z);
        }
        ++add;
        }

        planLPDel(lp);
        borTimerStop(&timer);
        fprintf(stderr, "Elapsed: %lf\n", borTimerElapsedInSF(&timer));
    }

    borTimerStart(&timer);
    planPDDLSasInvNodesInitFromFacts(&nodes, &invf->facts);

    // TODO: Limited number of iterations
    do {
        while (refineNodes(&nodes))
            planPDDLSasInvNodesReinit(&nodes);
    } while (planPDDLSasInvNodesSplit(&nodes) == 0);

    for (i = 0; i < nodes.node_size; ++i)
        createInvariant(invf, nodes.node + i);
    mergeInvariants(invf);
    pruneInvSubsets(invf);

    planPDDLSasInvNodesFree(&nodes);
    borTimerStop(&timer);
    fprintf(stderr, "Elapsed2: %lf\n", borTimerElapsedInSF(&timer));
}



static void storeFactIds(plan_pddl_ground_facts_t *dst,
                         plan_pddl_fact_pool_t *fact_pool,
                         const plan_pddl_facts_t *src)
{
    int i, fact_id;

    dst->size = 0;
    dst->fact = BOR_ALLOC_ARR(int, src->size);
    for (i = 0; i < src->size; ++i){
        fact_id = planPDDLFactPoolFind(fact_pool, src->fact + i);
        if (fact_id >= 0){
            dst->fact[dst->size++] = fact_id;
        }
    }

    if (dst->size != src->size){
        dst->fact = BOR_REALLOC_ARR(dst->fact, int, dst->size);
    }
}

static void processAction(plan_pddl_sas_inv_finder_t *invf,
                          const plan_pddl_ground_facts_t *pre,
                          const plan_pddl_ground_facts_t *pre_neg,
                          const plan_pddl_ground_facts_t *eff_add,
                          const plan_pddl_ground_facts_t *eff_del)
{
    int i;

    if (eff_del->size == 0){
        for (i = 0; i < eff_add->size; ++i){
            planPDDLSasInvFactsAddConflictsWithAll(&invf->facts,
                                                   eff_add->fact[i]);
        }

    }else{
        planPDDLSasInvFactsAddEdges(&invf->facts, eff_add, eff_del);
        planPDDLSasInvFactsAddConflicts(&invf->facts, eff_add);
    }
}

static void processActions(plan_pddl_sas_inv_finder_t *invf,
                           const plan_pddl_ground_action_pool_t *pool)
{
    const plan_pddl_ground_action_t *a;
    int i, j;

    for (i = 0; i < pool->size; ++i){
        a = planPDDLGroundActionPoolGet(pool, i);
        processAction(invf, &a->pre, &a->pre_neg,
                      &a->eff_add, &a->eff_del);
        for (j = 0; j < a->cond_eff.size; ++j){
            processAction(invf, &a->cond_eff.cond_eff[j].pre,
                          &a->cond_eff.cond_eff[j].pre_neg,
                          &a->cond_eff.cond_eff[j].eff_add,
                          &a->cond_eff.cond_eff[j].eff_del);
        }
    }
}


/*** INV ***/
static plan_pddl_sas_inv_t *invNew(const int *fact, int fact_size)
{
    plan_pddl_sas_inv_t *inv;
    int i, size;

    for (size = 0, i = 0; i < fact_size; ++i){
        if (fact[i])
            ++size;
    }

    if (size == 0)
        return NULL;

    inv = BOR_ALLOC(plan_pddl_sas_inv_t);
    inv->fact = BOR_ALLOC_ARR(int, size);
    inv->size = 0;
    borListInit(&inv->list);
    for (i = 0; i < fact_size; ++i){
        if (fact[i])
            inv->fact[inv->size++] = i;
    }

    borListInit(&inv->table);
    inv->table_key = invTableHashCompute(inv);

    return inv;
}

static void invDel(plan_pddl_sas_inv_t *inv)
{
    if (inv->fact)
        BOR_FREE(inv->fact);
    BOR_FREE(inv);
}

static int invIsSubset(const plan_pddl_sas_inv_t *inv,
                       const plan_pddl_sas_inv_t *inv2)
{
    int i, j;

    for (i = 0, j = 0; i < inv->size && j < inv2->size;){
        if (inv->fact[i] == inv2->fact[j]){
            ++i;
            ++j;

        }else if (inv->fact[i] < inv2->fact[j]){
            return 0;
        }else{
            ++j;
        }
    }

    return i == inv->size;
}

static int invHasCommonFact(const plan_pddl_sas_inv_t *inv,
                            const plan_pddl_sas_inv_t *inv2)
{
    int i, j;

    for (i = 0, j = 0; i < inv->size && j < inv2->size;){
        if (inv->fact[i] == inv2->fact[j]){
            return 1;
        }else if (inv->fact[i] < inv2->fact[j]){
            ++i;
        }else{
            ++j;
        }
    }

    return 0;
}
/*** INV END ***/



/*** REFINE ***/
static int refineNode(plan_pddl_sas_inv_node_t *node,
                      plan_pddl_sas_inv_nodes_t *ns)
{
    int change, anychange = 0;

    do {
        change = 0;

        planPDDLSasInvNodeUniqueEdges(node);

        change |= planPDDLSasInvNodeRemoveCoveredEdges(node);
        change |= planPDDLSasInvNodeRemoveConflictNodesFromEdges(node);
        change |= planPDDLSasInvNodePruneSuperEdges(node, ns);
        change |= planPDDLSasInvNodeAddMustFromSingleEdges(node);
        change |= planPDDLSasInvNodeAddMustsOfMusts(node, ns);
        change |= planPDDLSasInvNodeAddConflictsOfMusts(node, ns);

        anychange |= change;
    } while (change);

    return anychange;
}

static int refineNodes(plan_pddl_sas_inv_nodes_t *ns)
{
    int i, change, anychange = 0;

    do {
        change = 0;
        for (i = 0; i < ns->node_size; ++i)
            change |= refineNode(ns->node + i, ns);
        anychange |= change;
    } while (change);

    return anychange;
}
/*** REFINE END ***/


/*** CREATE/MERGE-INVARIANT ***/
static int checkMergedInvariant(const plan_pddl_sas_inv_finder_t *invf,
                                const plan_pddl_sas_inv_t *inv1,
                                const plan_pddl_sas_inv_t *inv2)
{
    int i, *I, res;

    I = BOR_CALLOC_ARR(int, invf->facts.fact_size);
    for (i = 0; i < inv1->size; ++i)
        I[inv1->fact[i]] = 1;
    for (i = 0; i < inv2->size; ++i)
        I[inv2->fact[i]] = 1;

    res = planPDDLSasInvFactsCheckInvariant(&invf->facts, I);

    BOR_FREE(I);
    return res;
}

static int createUniqueInvariantFromFacts(plan_pddl_sas_inv_finder_t *invf,
                                          const int *fact)
{
    plan_pddl_sas_inv_t *inv;
    bor_list_t *item;

    // Create a unique invariant and append it to the list
    inv = invNew(fact, invf->facts.fact_size);
    item = borHTableInsertUnique(invf->inv_table, &inv->table);
    if (item == NULL){
        ++invf->inv_size;
        borListAppend(&invf->inv, &inv->list);
        return 1;

    }else{
        invDel(inv);
        return 0;
    }
}

static int createMergedInvariant(plan_pddl_sas_inv_finder_t *invf,
                                 const plan_pddl_sas_inv_t *inv1,
                                 const plan_pddl_sas_inv_t *inv2)
{
    int i, *fact, change = 0;

    fact = BOR_CALLOC_ARR(int, invf->facts.fact_size);
    for (i = 0; i < inv1->size; ++i)
        fact[inv1->fact[i]] = 1;
    for (i = 0; i < inv2->size; ++i)
        fact[inv2->fact[i]] = 1;
    change = createUniqueInvariantFromFacts(invf, fact);

    BOR_FREE(fact);
    return change;
}

static int createInvariant(plan_pddl_sas_inv_finder_t *invf,
                           const plan_pddl_sas_inv_node_t *node)
{
    // Check that the invariant is really invariant
    if (node->edges.edge_size > 0 || node->conflict[node->id]){
        if (planPDDLSasInvFactsCheckInvariant(&invf->facts, node->inv)){
            fprintf(stderr, "ERROR: Something is really wrong!\n");
        }
        return 0;

    }else{
        if (!planPDDLSasInvFactsCheckInvariant(&invf->facts, node->inv)){
            fprintf(stderr, "ERROR: Something is really really wrong!\n");
            int i;
            for (i = 0; i < node->fact_size; ++i){
                if (node->inv[i]){
                    planPDDLSasInvFactPrint(invf->facts.fact + i, stderr);
                }
            }
            planPDDLSasInvNodePrint(node, stderr);
            fprintf(stderr, "\n");
        }
    }

    // Skip invariants counting only one fact
    if (planPDDLSasInvNodeInvSize(node) <= 1)
        return 0;

    // Create a unique invariant and append it to the list
    return createUniqueInvariantFromFacts(invf, node->inv);
}

static int mergeInvariantsTry(plan_pddl_sas_inv_finder_t *invf)
{
    bor_list_t *item1, *item2;
    plan_pddl_sas_inv_t *inv1, *inv2;
    int change = 0;

    BOR_LIST_FOR_EACH(&invf->inv, item1){
        inv1 = BOR_LIST_ENTRY(item1, plan_pddl_sas_inv_t, list);
        BOR_LIST_FOR_EACH(&invf->inv, item2){
            inv2 = BOR_LIST_ENTRY(item2, plan_pddl_sas_inv_t, list);
            if (inv1 != inv2
                    && invHasCommonFact(inv1, inv2)
                    && !invIsSubset(inv1, inv2)
                    && checkMergedInvariant(invf, inv1, inv2)){
                change |= createMergedInvariant(invf, inv1, inv2);
            }
        }
    }

    return change;
}

static void mergeInvariants(plan_pddl_sas_inv_finder_t *invf)
{
    while (mergeInvariantsTry(invf));
}

static void pruneInvSubsets(plan_pddl_sas_inv_finder_t *invf)
{
    bor_list_t *item, *item2, *tmp;
    plan_pddl_sas_inv_t *inv1, *inv2;

    BOR_LIST_FOR_EACH(&invf->inv, item){
        inv1 = bor_container_of(item, plan_pddl_sas_inv_t, list);
        BOR_LIST_FOR_EACH_SAFE(&invf->inv, item2, tmp){
            inv2 = bor_container_of(item2, plan_pddl_sas_inv_t, list);
            if (inv1 != inv2 && invIsSubset(inv2, inv1)){
                borListDel(&inv2->list);
                borHTableErase(invf->inv_table, &inv2->table);
                invDel(inv2);
                --invf->inv_size;
            }
        }
    }
}
/*** CREATE/MERGE-INVARIANT END ***/


static bor_htable_key_t invTableHashCompute(const plan_pddl_sas_inv_t *inv)
{
    return borCityHash_64(inv->fact, inv->size * sizeof(int));
}

static bor_htable_key_t invTableHash(const bor_list_t *key, void *ud)
{
    const plan_pddl_sas_inv_t *inv;

    inv = bor_container_of(key, const plan_pddl_sas_inv_t, table);
    return inv->table_key;
}

static int invTableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud)
{
    const plan_pddl_sas_inv_t *inv1, *inv2;

    inv1 = bor_container_of(k1, const plan_pddl_sas_inv_t, table);
    inv2 = bor_container_of(k2, const plan_pddl_sas_inv_t, table);

    if (inv1->size != inv2->size)
        return 0;
    return memcmp(inv1->fact, inv2->fact, sizeof(int) * inv1->size) == 0;
}
