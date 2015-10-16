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
#include "plan/causal_graph.h"

#ifndef _GNU_SOURCE
/** Declaration of qsort_r() function that should be available in libc */
void qsort_r(void *base, size_t nmemb, size_t size,
             int (*compar)(const void *, const void *, void *),
             void *arg);
#endif

/** Transforms list of pddl facts to the list of fact IDs */
static void writeFactIds(plan_pddl_ground_facts_t *dst,
                         plan_pddl_fact_pool_t *fact_pool,
                         const plan_pddl_facts_t *src);
/** Transforms invariants into sas variables */
static void invariantToVar(plan_pddl_sas_t *sas);
/** Applies simplifications received from causal graph */
static void causalGraph(plan_pddl_sas_t *sas);

void planPDDLSasInit(plan_pddl_sas_t *sas, const plan_pddl_ground_t *g)
{
    int i;

    sas->ground = g;
    planPDDLSasInvFinderInit(&sas->inv_finder, g);

    sas->fact_size = g->fact_pool.size;
    sas->fact = BOR_CALLOC_ARR(plan_pddl_sas_fact_t, sas->fact_size);
    for (i = 0; i < sas->fact_size; ++i){
        sas->fact[i].var = PLAN_VAR_ID_UNDEFINED;
        sas->fact[i].val = PLAN_VAL_UNDEFINED;
    }

    sas->var_range = NULL;
    sas->var_order = NULL;
    sas->var_size = 0;
    writeFactIds(&sas->goal, (plan_pddl_fact_pool_t *)&g->fact_pool,
                 &g->pddl->goal);
    writeFactIds(&sas->init, (plan_pddl_fact_pool_t *)&g->fact_pool,
                 &g->pddl->init_fact);
}

void planPDDLSasFree(plan_pddl_sas_t *sas)
{
    if (sas->fact != NULL)
        BOR_FREE(sas->fact);

    if (sas->var_range != NULL)
        BOR_FREE(sas->var_range);
    if (sas->var_order != NULL)
        BOR_FREE(sas->var_order);
    if (sas->goal.fact != NULL)
        BOR_FREE(sas->goal.fact);
    if (sas->init.fact != NULL)
        BOR_FREE(sas->init.fact);

    planPDDLSasInvFinderFree(&sas->inv_finder);
}

void planPDDLSas(plan_pddl_sas_t *sas, unsigned flags)
{
    planPDDLSasInvFinder(&sas->inv_finder);
    invariantToVar(sas);

    if (flags & PLAN_PDDL_SAS_USE_CG){
        causalGraph(sas);
    }
}

void planPDDLSasPrintInvariant(const plan_pddl_sas_t *sas,
                               const plan_pddl_ground_t *g,
                               FILE *fout)
{
    const plan_pddl_fact_t *fact;
    bor_list_t *item;
    const plan_pddl_sas_inv_t *inv;
    int i, j;

    i = 0;
    BOR_LIST_FOR_EACH(&sas->inv_finder.inv, item){
        inv = BOR_LIST_ENTRY(item, plan_pddl_sas_inv_t, list);
        fprintf(fout, "Invariant %d:\n", i);
        for (j = 0; j < inv->size; ++j){
            fact = planPDDLFactPoolGet(&g->fact_pool, inv->fact[j]);
            fprintf(fout, "    ");
            planPDDLFactPrint(&g->pddl->predicate, &g->pddl->obj, fact, fout);
            fprintf(fout, " var: %d, val: %d/%d",
                    sas->fact[inv->fact[j]].var,
                    sas->fact[inv->fact[j]].val,
                    sas->var_range[sas->fact[inv->fact[j]].var]);
            fprintf(fout, "\n");
        }

        ++i;
    }
}

void planPDDLSasPrintInvariantFD(const plan_pddl_sas_t *sas,
                                 const plan_pddl_ground_t *g,
                                 FILE *fout)
{
    const plan_pddl_fact_t *fact;
    bor_list_t *item;
    const plan_pddl_sas_inv_t *inv;
    int i, j, k;

    i = 0;
    BOR_LIST_FOR_EACH(&sas->inv_finder.inv, item){
        inv = BOR_LIST_ENTRY(item, plan_pddl_sas_inv_t, list);
        fprintf(fout, "I: ");
        for (j = 0; j < inv->size; ++j){
            fact = planPDDLFactPoolGet(&g->fact_pool, inv->fact[j]);
            if (j > 0)
                fprintf(fout, ";");
            fprintf(fout, "Atom ");
            fprintf(fout, "%s(", g->pddl->predicate.pred[fact->pred].name);

            for (k = 0; k < fact->arg_size; ++k){
                if (k > 0)
                    fprintf(fout, ", ");
                fprintf(fout, "%s", g->pddl->obj.obj[fact->arg[k]].name);
            }
            fprintf(fout, ")");
        }
        fprintf(fout, "\n");

        ++i;
    }
}

void planPDDLSasPrintFacts(const plan_pddl_sas_t *sas,
                           const plan_pddl_ground_t *g,
                           FILE *fout)
{
    const plan_pddl_fact_t *fact;
    const plan_pddl_sas_fact_t *f;
    int i;

    fprintf(fout, "SAS Facts[%d]:\n", sas->fact_size);
    for (i = 0; i < sas->fact_size; ++i){
        f = sas->fact + i;
        fact = planPDDLFactPoolGet(&g->fact_pool, i);
        fprintf(fout, "    [%d] ", i);
        //fprintf(fout, "    ");
        planPDDLFactPrint(&g->pddl->predicate, &g->pddl->obj, fact, fout);
        if (f->var != PLAN_VAR_ID_UNDEFINED){
            fprintf(fout, " var: %d, val: %d/%d", (int)f->var, (int)f->val,
                    (int)sas->var_range[f->var]);
        }else{
            fprintf(fout, " var: %d", (int)PLAN_VAR_ID_UNDEFINED);
        }

        /*
        fprintf(fout, ", conflict:");
        for (int j = 0; j < f->conflict.size; ++j){
            fprintf(fout, " %d", f->conflict.fact[j]);
        }
        */

        fprintf(fout, "\n");
    }
}


static void writeFactIds(plan_pddl_ground_facts_t *dst,
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






/*** INVARIANT-TO-VAR ***/
static int invariantCmp(const void *a, const void *b)
{
    const plan_pddl_ground_facts_t *f1 = a;
    const plan_pddl_ground_facts_t *f2 = b;
    return f2->size - f1->size;
}

static void invariantsToGroundFacts(plan_pddl_sas_t *sas,
                                    plan_pddl_ground_facts_t *fs)
{
    bor_list_t *item;
    plan_pddl_sas_inv_t *inv;
    plan_pddl_ground_facts_t *f;
    int size;

    size = 0;
    BOR_LIST_FOR_EACH(&sas->inv_finder.inv, item){
        inv = BOR_LIST_ENTRY(item, plan_pddl_sas_inv_t, list);
        f = fs + size;
        f->size = inv->size;
        f->fact = BOR_ALLOC_ARR(int, inv->size);
        memcpy(f->fact, inv->fact, sizeof(int) * inv->size);
        ++size;
    }

    qsort(fs, size, sizeof(plan_pddl_ground_facts_t), invariantCmp);
}

static void invariantGroundFactsFree(plan_pddl_sas_t *sas,
                                     plan_pddl_ground_facts_t *inv)
{
    int i;
    for (i = 0; i < sas->inv_finder.inv_size; ++i){
        if (inv[i].fact != NULL)
            BOR_FREE(inv[i].fact);
    }
}

static void invariantAddVar(plan_pddl_sas_t *sas,
                            const plan_pddl_ground_facts_t *inv)
{
    int size, var_range, var, val;

    size = var_range = inv->size;
    var = sas->var_size;
    ++sas->var_size;
    sas->var_range = BOR_REALLOC_ARR(sas->var_range, plan_val_t, sas->var_size);
    sas->var_range[var] = var_range;

    for (val = 0; val < size; ++val){
        sas->fact[inv->fact[val]].var = var;
        sas->fact[inv->fact[val]].val = val;
    }
}

static void invariantRemoveFacts(const plan_pddl_sas_t *sas,
                                 const plan_pddl_ground_facts_t *src,
                                 plan_pddl_ground_facts_t *dst)
{
    int si, di, ins, ssize, dsize;
    int sfact, dfact;

    ssize = src->size;
    dsize = dst->size;
    for (si = 0, di = 0, ins = 0; si < ssize && di < dsize;){
        sfact = src->fact[si];
        dfact = dst->fact[di];

        if (sfact == dfact){
            ++si;
            ++di;
        }else if (sfact < dfact){
            ++si;
        }else{ // dfact < sfact
            dst->fact[ins++] = dst->fact[di];
            ++di;
        }
    }

    for (; di < dsize; ++di)
        dst->fact[ins++] = dst->fact[di];

    dst->size = ins;
}

static void invariantRemoveTop(const plan_pddl_sas_t *sas,
                               plan_pddl_ground_facts_t *inv)
{
    int i;

    for (i = 1; i < sas->inv_finder.inv_size; ++i)
        invariantRemoveFacts(sas, inv, inv + i);
    inv[0].size = 0;
    qsort(inv, sas->inv_finder.inv_size, sizeof(plan_pddl_ground_facts_t),
          invariantCmp);
}

static int numVarEdge(const plan_pddl_sas_t *sas,
                      const plan_pddl_ground_facts_t *fs,
                      int var)
{
    int i, num = 0;

    for (i = 0; i < fs->size; ++i){
        if (sas->fact[fs->fact[i]].var == var)
            ++num;
    }
    return num;
}

static void setVarNeg(plan_pddl_sas_t *sas)
{
    const plan_pddl_sas_fact_t *fact;
    const plan_pddl_sas_inv_fact_t *inv_fact;
    int i, j, add, *var_done;

    var_done = BOR_CALLOC_ARR(int, sas->var_size);
    for (i = 0; i < sas->fact_size; ++i){
        fact = sas->fact + i;
        inv_fact = sas->inv_finder.facts.fact + i;
        if (var_done[fact->var])
            continue;

        add = (sas->var_range[fact->var] == 1);
        for (j = 0; !add && j < inv_fact->edge_size; ++j){
            if (numVarEdge(sas, inv_fact->edge + j, fact->var) == 0)
                add = 1;
        }

        if (add){
            var_done[fact->var] = 1;
            ++sas->var_range[fact->var];
        }
    }
    BOR_FREE(var_done);
}

static void invariantToVar(plan_pddl_sas_t *sas)
{
    plan_pddl_ground_facts_t *inv = NULL;
    plan_pddl_ground_facts_t one;
    int i;

    if (sas->inv_finder.inv_size > 0){
        inv = BOR_CALLOC_ARR(plan_pddl_ground_facts_t, sas->inv_finder.inv_size);
        invariantsToGroundFacts(sas, inv);

        while (inv[0].size > 1){
            invariantAddVar(sas, inv);
            invariantRemoveTop(sas, inv);
        }
    }

    one.fact = alloca(sizeof(int));
    one.size = 1;
    for (i = 0; i < sas->fact_size; ++i){
        if (sas->fact[i].var == PLAN_VAR_ID_UNDEFINED){
            one.fact[0] = i;
            invariantAddVar(sas, &one);
        }
    }

    if (inv != NULL){
        invariantGroundFactsFree(sas, inv);
        BOR_FREE(inv);
    }

    setVarNeg(sas);
}
/*** INVARIANT-TO-VAR END ***/



/*** CAUSAL GRAPH ***/
static void causalGraphBuildAddEdges(const plan_pddl_sas_t *sas,
                                     const plan_pddl_ground_facts_t *pre,
                                     const plan_pddl_ground_facts_t *eff,
                                     plan_causal_graph_build_t *build)
{
    int i, j, pre_var, eff_var;

    for (i = 0; i < pre->size; ++i){
        for (j = 0; j < eff->size; ++j){
            pre_var = sas->fact[pre->fact[i]].var;
            eff_var = sas->fact[eff->fact[j]].var;
            planCausalGraphBuildAdd(build, pre_var, eff_var);
        }
    }
}

static void causalGraphBuildAddAction(const plan_pddl_sas_t *sas,
                                      const plan_pddl_ground_action_t *action,
                                      plan_causal_graph_build_t *build)
{
    const plan_pddl_ground_cond_eff_t *ce;
    int i;

    causalGraphBuildAddEdges(sas, &action->pre, &action->eff_add, build);
    causalGraphBuildAddEdges(sas, &action->pre, &action->eff_del, build);
    causalGraphBuildAddEdges(sas, &action->pre_neg, &action->eff_add, build);
    causalGraphBuildAddEdges(sas, &action->pre_neg, &action->eff_del, build);

    for (i = 0; i < action->cond_eff.size; ++i){
        ce = action->cond_eff.cond_eff + i;
        causalGraphBuildAddEdges(sas, &action->pre, &ce->eff_add, build);
        causalGraphBuildAddEdges(sas, &action->pre, &ce->eff_del, build);
        causalGraphBuildAddEdges(sas, &action->pre_neg, &ce->eff_add, build);
        causalGraphBuildAddEdges(sas, &action->pre_neg, &ce->eff_del, build);
        causalGraphBuildAddEdges(sas, &ce->pre, &ce->eff_add, build);
        causalGraphBuildAddEdges(sas, &ce->pre_neg, &ce->eff_del, build);
    }
}

static plan_causal_graph_t *causalGraphBuild(const plan_pddl_sas_t *sas)
{
    plan_causal_graph_t *cg;
    plan_causal_graph_build_t cg_build;
    const plan_pddl_ground_action_t *action;
    plan_part_state_t *goal;
    int i;

    planCausalGraphBuildInit(&cg_build);
    for (i = 0; i < sas->ground->action_pool.size; ++i){
        action = planPDDLGroundActionPoolGet(&sas->ground->action_pool, i);
        causalGraphBuildAddAction(sas, action, &cg_build);
    }

    cg = planCausalGraphNew(sas->var_size);
    planCausalGraphBuild(cg, &cg_build);
    planCausalGraphBuildFree(&cg_build);

    goal = planPartStateNew(sas->var_size);
    for (i = 0; i < sas->goal.size; ++i){
        planPartStateSet(goal, sas->fact[sas->goal.fact[i]].var,
                               sas->fact[sas->goal.fact[i]].val);
    }
    planCausalGraph(cg, goal);
    planPartStateDel(goal);

    return cg;
}

static void causalGraph(plan_pddl_sas_t *sas)
{
    plan_causal_graph_t *cg;
    plan_var_id_t *var_map;
    int i, var_size;
    plan_val_t *var_range;

    cg = causalGraphBuild(sas);

    // Create mapping from old var ID to the new ID
    var_map = (plan_var_id_t *)alloca(sizeof(plan_var_id_t) * sas->var_size);
    var_size = cg->var_order_size;
    for (i = 0; i < sas->var_size; ++i)
        var_map[i] = PLAN_VAR_ID_UNDEFINED;
    for (i = 0; i < cg->var_order_size; ++i)
        var_map[cg->var_order[i]] = i;

    // Fix fact variables
    for (i = 0; i < sas->fact_size; ++i)
        sas->fact[i].var = var_map[sas->fact[i].var];

    // Fix ranges of variables
    var_range = BOR_ALLOC_ARR(plan_val_t, var_size);
    for (i = 0; i < sas->var_size; ++i){
        if (var_map[i] != PLAN_VAR_ID_UNDEFINED){
            var_range[var_map[i]] = sas->var_range[i];
        }
    }
    BOR_FREE(sas->var_range);
    sas->var_range = var_range;
    sas->var_size = var_size;

    // Copy var-order array
    sas->var_order = BOR_ALLOC_ARR(plan_var_id_t, cg->var_order_size + 1);
    memcpy(sas->var_order, cg->var_order,
           sizeof(plan_var_id_t) * cg->var_order_size);
    sas->var_order[cg->var_order_size] = PLAN_VAR_ID_UNDEFINED;
    for (i = 0; i < cg->var_order_size; ++i)
        sas->var_order[i] = var_map[sas->var_order[i]];

    planCausalGraphDel(cg);
}
/*** CAUSAL GRAPH END ***/
