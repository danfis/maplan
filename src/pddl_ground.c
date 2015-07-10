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
static void instActionsNegativePreconditions(const plan_pddl_lift_actions_t *as,
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

struct _sas_fact_edge_t {
    int action;
    int fact;
};
typedef struct _sas_fact_edge_t sas_fact_edge_t;

typedef struct _sas_split_node_t sas_split_node_t;
struct _sas_split_node_t {
    int *fact;
    sas_split_node_t *child;
    int size;
};

struct _sas_fact_t {
    int *conn;
    sas_fact_edge_t *edge;
    int edge_size;
    sas_split_node_t split;
    plan_pddl_ground_facts_t must_fact;
    int is_init;
};
typedef struct _sas_fact_t sas_fact_t;

struct _sas_t {
    sas_fact_t *fact;
    int fact_size;

    int cur_root;
    int *cur_comp;
};
typedef struct _sas_t sas_t;


static void sasInit(sas_t *sas, int fact_size)
{
    sas_fact_t *f;
    int i;

    bzero(sas, sizeof(*sas));
    sas->fact_size = fact_size;
    sas->fact = BOR_CALLOC_ARR(sas_fact_t, fact_size);
    for (i = 0; i < fact_size; ++i){
        f = sas->fact + i;
        f->conn = BOR_CALLOC_ARR(int, fact_size);
    }

    sas->cur_root = 0;
    sas->cur_comp = BOR_ALLOC_ARR(int, fact_size);
}

static void sasSplitNodeFree(sas_split_node_t *split)
{
    int i;

    for (i = 0; i < split->size; ++i)
        sasSplitNodeFree(split->child + i);

    if (split->child != NULL)
        BOR_FREE(split->child);
    if (split->fact != NULL)
        BOR_FREE(split->fact);
}

static void sasFactFree(sas_fact_t *sas_fact)
{
    if (sas_fact->conn)
        BOR_FREE(sas_fact->conn);
    if (sas_fact->edge)
        BOR_FREE(sas_fact->edge);
    if (sas_fact->must_fact.fact != NULL)
        BOR_FREE(sas_fact->must_fact.fact);
    sasSplitNodeFree(&sas_fact->split);
}

static void sasFree(sas_t *sas)
{
    sas_fact_t *f;
    int i;

    for (i = 0; i < sas->fact_size; ++i){
        f = sas->fact + i;
        sasFactFree(f);
    }
    BOR_FREE(sas->fact);
    if (sas->cur_comp != NULL)
        BOR_FREE(sas->cur_comp);
}

static void sasSetInitFacts(sas_t *sas, const plan_pddl_facts_t *fs,
                            plan_pddl_fact_pool_t *fact_pool)
{
    const plan_pddl_fact_t *f;
    int i, j, id;
    int size, fact_ids[fs->size];

    size = 0;
    for (i = 0; i < fs->size; ++i){
        f = fs->fact + i;
        id = planPDDLFactPoolFind(fact_pool, f);
        if (id >= 0){
            sas->fact[id].is_init = 1;
            fact_ids[size++] = id;
        }
    }

    for (i = 0; i < size; ++i){
        fprintf(stdout, "Init: %d\n", fact_ids[i]);
        for (j = i + 1; j < size; ++j){
            sas->fact[fact_ids[i]].conn[fact_ids[j]] = -1;
            sas->fact[fact_ids[j]].conn[fact_ids[i]] = -1;
        }
    }
}

static void sasMarkExclusiveFacts2(sas_t *sas,
                                   const plan_pddl_ground_facts_t *fs)
{
    int i, j, f1, f2;

    for (i = 0; i < fs->size; ++i){
        f1 = fs->fact[i];
        for (j = i + 1; j < fs->size; ++j){
            f2 = fs->fact[j];
            sas->fact[f1].conn[f2] = -1;
            sas->fact[f2].conn[f1] = -1;
        }
    }
}

static void sasMarkExclusiveFacts(sas_t *sas,
                                  const plan_pddl_ground_action_pool_t *pool)
{
    const plan_pddl_ground_action_t *a;
    int i;

    for (i = 0; i < pool->size; ++i){
        a = planPDDLGroundActionPoolGet(pool, i);
        sasMarkExclusiveFacts2(sas, &a->pre);
        sasMarkExclusiveFacts2(sas, &a->eff_add);
        sasMarkExclusiveFacts2(sas, &a->eff_del);
    }
}

static void sasFactAddEdge(sas_fact_t *fact, int action_id, int fact_id)
{
    sas_fact_edge_t *edge;

    ++fact->edge_size;
    fact->edge = BOR_REALLOC_ARR(fact->edge, sas_fact_edge_t, fact->edge_size);
    edge = fact->edge + fact->edge_size - 1;
    edge->action = action_id;
    edge->fact   = fact_id;
}

static void sasConnectFacts2(sas_t *sas, int action_id,
                             const plan_pddl_ground_facts_t *del,
                             const plan_pddl_ground_facts_t *add)
{
    sas_fact_t *fact;
    int i, j, f1, f2;

    for (i = 0; i < del->size; ++i){
        f1 = del->fact[i];
        fact = sas->fact + f1;

        for (j = 0; j < add->size; ++j){
            f2 = add->fact[j];
            if (fact->conn[f2] >= 0){
                ++fact->conn[f2];
                sasFactAddEdge(fact, action_id, f2);
            }
        }
    }

    if (del->size == 0){
        for (j = 0; j < add->size; ++j){
            f1 = add->fact[j];
            sas->fact[f1].conn[f1]++;
        }
    }
}

static void sasPrintConn(const sas_t *sas, FILE *fout)
{
    int i, j;
    int conn;

    for (i = 0; i < sas->fact_size; ++i){
        for (j = 0; j < sas->fact_size; ++j){
            conn = sas->fact[i].conn[j];
            if (conn == -1){
                fprintf(fout, "X");
            }else if (conn == 0){
                fprintf(fout, "-");
            }else if (conn == 1){
                fprintf(fout, "I");
            }else if (conn > 1){
                fprintf(fout, "C");
            }
        }
        fprintf(fout, "\n");
    }
}

static void sasConnectFacts(sas_t *sas,
                            const plan_pddl_ground_action_pool_t *pool)
{
    const plan_pddl_ground_action_t *a;
    int i;

    for (i = 0; i < pool->size; ++i){
        a = planPDDLGroundActionPoolGet(pool, i);
        sasConnectFacts2(sas, i, &a->eff_del, &a->eff_add);
    }
}

static void _sasSetMustFacts(sas_fact_t *sas_fact)
{
    plan_pddl_ground_facts_t *w;
    int i, j, action;

    w = &sas_fact->must_fact;
    w->size = 0;
    w->fact = BOR_ALLOC_ARR(int, sas_fact->edge_size);

    for (i = 0; i < sas_fact->edge_size;){
        action = sas_fact->edge[i].action;
        for (j = i + 1; j < sas_fact->edge_size
                && sas_fact->edge[j].action == action; ++j);

        if (j == i + 1)
            w->fact[w->size++] = sas_fact->edge[i].fact;

        i = j;
    }
}

static void sasSetMustFacts(sas_t *sas)
{
    int i;
    for (i = 0; i < sas->fact_size; ++i)
        _sasSetMustFacts(sas->fact + i);
}

static int _sasBuildSplitTreeNext(const sas_fact_t *sas_fact, int from,
                                  int *start, int *end)
{
    int i, j, action;

    *start = *end = -1;
    for (i = from; i < sas_fact->edge_size;){
        action = sas_fact->edge[i].action;
        for (j = i + 1; j < sas_fact->edge_size
                && sas_fact->edge[j].action == action; ++j);

        if (j != i + 1){
            *start = i;
            *end = j;
            return 0;
        }

        i = j;
    }

    return -1;
}

static void _sasBuildSplitTree(sas_t *sas,
                               sas_fact_t *sas_fact,
                               sas_split_node_t *node,
                               const int *allowed,
                               int from, int to)
{
    int *next_allowed;
    int forced, split;
    int i, j, fi, fact_id, next_from, next_to;

    if (from == -1){
        node->size = 0;
        return;
    }

    forced = -1;
    split = 0;
    for (i = from; i < to; ++i){
        fact_id = sas_fact->edge[i].fact;
        if (allowed[fact_id] == 0){
            ++split;

        }else if (allowed[fact_id] == 1){
            if (forced != -1){
                // Two forced edges -- this branch cannot be satisfied.
                node->size = -1;
                return;
            }
            forced = i;
        }
    }

    if (forced >= 0){
        next_allowed = BOR_ALLOC_ARR(int, sas->fact_size);
        memcpy(next_allowed, allowed, sizeof(int) * sas->fact_size);
        next_allowed[forced] = 1;

        for (j = from; j < to; ++j){
            if (forced != j)
                next_allowed[sas_fact->edge[j].fact] = -1;
        }
        _sasBuildSplitTreeNext(sas_fact, to, &next_from, &next_to);
        _sasBuildSplitTree(sas, sas_fact, node, next_allowed, next_from, next_to);
        BOR_FREE(next_allowed);
        return;
    }

    if (split == 0){
        // No branching possible
        node->size = -1;
        return;
    }

    node->size = split;
    node->fact = BOR_ALLOC_ARR(int, node->size);
    node->child = BOR_CALLOC_ARR(sas_split_node_t, node->size);
    next_allowed = BOR_ALLOC_ARR(int, sas->fact_size);
    for (fi = 0, i = from; i < to; ++i){
        fact_id = sas_fact->edge[i].fact;
        if (allowed[fact_id] == -1)
            continue;

        memcpy(next_allowed, allowed, sizeof(int) * sas->fact_size);
        next_allowed[fact_id] = 1;

        for (j = from; j < to; ++j){
            if (i != j)
                next_allowed[sas_fact->edge[j].fact] = -1;
        }

        _sasBuildSplitTreeNext(sas_fact, to, &next_from, &next_to);
        _sasBuildSplitTree(sas, sas_fact, &node->child[fi], next_allowed,
                           next_from, next_to);
        node->fact[fi] = fact_id;
        ++fi;
    }

    BOR_FREE(next_allowed);

    for (i = 0, fi = 0; i < node->size; ++i){
        if (node->child[i].size != -1){
            if (i != fi){
                node->child[fi] = node->child[i];
                node->fact[fi] = node->fact[i];
            }
            ++fi;
        }
    }
    node->size = fi;

    if (node->size == 0){
        BOR_FREE(node->child);
        BOR_FREE(node->fact);
        node->size = -1;
    }
}


static void sasBuildSplitTree(sas_t *sas, sas_fact_t *sas_fact)
{
    int from, to, *allowed;

    allowed = BOR_CALLOC_ARR(int, sas->fact_size);
    _sasBuildSplitTreeNext(sas_fact, 0, &from, &to);
    _sasBuildSplitTree(sas, sas_fact, &sas_fact->split, allowed, from, to);
    BOR_FREE(allowed);
}

static void sasBuildSplitTrees(sas_t *sas)
{
    int i;
    for (i = 0; i < sas->fact_size; ++i)
        sasBuildSplitTree(sas, sas->fact + i);
}

static void _sasPrintSplitTree(sas_split_node_t *root, int fact_id, int prefix, FILE *fout)
{
    int i;
    for (i = 0; i < prefix; ++i)
        fprintf(fout, " ");
    fprintf(fout, "%d: size: %d\n", fact_id, root->size);

    for (i = 0; i < root->size; ++i)
        _sasPrintSplitTree(root->child + i, root->fact[i], prefix + 2, fout);
}

static void sasPrintSplitTree(sas_split_node_t *root, FILE *fout)
{
    _sasPrintSplitTree(root, -1, 0, fout);
}

static void sasPrintSplitTrees(sas_t *sas, FILE *fout)
{
    int i;
    for (i = 0; i < sas->fact_size; ++i){
        fprintf(fout, "fact-split-tree[%d]:\n", i);
        sasPrintSplitTree(&sas->fact[i].split, fout);
    }
}

static long sasSCCIter(int node_id, void *ud)
{
    return 0;
}

static int sasSCCNext(int node_id, long *it, void *ud)
{
    sas_t *sas = ud;
    sas_fact_t *fact = sas->fact + node_id;
    const int *allowed = sas->cur_comp;
    const int *conn = fact->conn;
    int i, id;

    /*
    for (i = *it; i < sas->fact_size; ++i){
        if (conn[i] > 0 && root_conn[i] >= 0){
            fprintf(stderr, "%d -> %d\n", node_id, i);
            *it = i + 1;
            return i;
        }
    }
    */

    for (i = *it; i < fact->edge_size; ++i){
        id = fact->edge[i].fact;
        if (allowed[id]){
            *it = i + 1;
            return id;
        }
    }

    return -1;
}

static void sccPrint(const bor_scc_t *scc, FILE *fout)
{
    int i, j;

    fprintf(stderr, "XXX\n");
    for (i = 0; i < scc->comp_size; ++i){
        fprintf(fout, "Component %d:", i);
        for (j = 0; j < scc->comp[i].node_size; ++j){
            fprintf(fout, " %d", scc->comp[i].node[j]);
        }
        fprintf(fout, "\n");
    }
}

static void sasPrintComp(const sas_t *sas, FILE *fout)
{
    int i;

    fprintf(fout, "SAS comp [%d]:", sas->cur_root);
    for (i = 0; i < sas->fact_size; ++i){
        if (sas->cur_comp[i] == 1)
            fprintf(fout, " %d", i);
        if (sas->cur_comp[i] == 2)
            fprintf(fout, " F%d", i);
    }
    fprintf(fout, "\n");
}

static int _sasCompFixFacts(sas_t *sas, int fact_id)
{
    const sas_fact_t *fact;
    int i, j, action, fid;

    sas->cur_comp[fact_id] = 2;
    fact = sas->fact + fact_id;

    if (fact->edge_size == 1)
        return _sasCompFixFacts(sas, fact->edge[0].fact);

    for (i = 0; i < fact->edge_size;){
        action = fact->edge[i].action;
        for (j = i + 1; j < fact->edge_size
                && fact->edge[j].action == action; ++j);

        fprintf(stderr, "i: %d / %d -- %d\n", i, fact->edge_size, j);
        if (j == i + 1){
            fid = fact->edge[i].fact;
            fprintf(stderr, "%d -- %d --> %d\n", fact_id, fact->edge[i].action,
                    fact->edge[i].fact);
            if (sas->cur_comp[fid] <= 0)
                return -1;
            if (sas->cur_comp[fid] == 1){
                if (_sasCompFixFacts(sas, fid) != 0)
                    return -1;
            }
        }

        i = j;
    }
    return 0;
}

static int sasCompFixFacts(sas_t *sas)
{
    return _sasCompFixFacts(sas, sas->cur_root);
}

static int _sasGenComponentsEdge(sas_t *sas, const sas_fact_t *fact,
                                 const int *comp, int edge_from, int edge_to)
{
    int i, fid, found2, found1;

    found2 = -1;
    found1 = 0;
    for (i = edge_from; i < edge_to; ++i){
        fid = fact->edge[i].fact;
        if (comp[fid]){
            found1 = 1;
            if (comp[fid] == 2){
                if (found2 != -1) // Two '2'!!
                    return -1;
                found2 = fid;
            }
        }
    }

    if (found2 >= 0)
        return 0;

    return 0;
}

static int _sasGenComponents(sas_t *sas, int fact_id, const int *_comp)
{
    const sas_fact_t *fact = sas->fact + fact_id;
    int *comp;
    int i, j, action;

    comp = BOR_ALLOC_ARR(int, sas->fact_size);
    memcpy(comp, _comp, sizeof(int) * sas->fact_size);

    comp[fact_id] = 2;
    for (i = 0; i < fact->edge_size;){
        action = fact->edge[i].action;
        for (j = i + 1; j < fact->edge_size
                && fact->edge[j].action == action; ++j);

        fprintf(stderr, "i: %d / %d -- %d\n", i, fact->edge_size, j);
        if (j == i + 1){
            // The only possible edge -- it must be in the component
            if (comp[fact->edge[i].fact] == 0)
                return -1;

            comp[fact->edge[i].fact] = 2;

        }else{
            if (_sasGenComponentsEdge(sas, fact, comp, i, j) != 0)
                return -1;
        }

        i = j;
    }

    return 0;
}

static void sasGenComponents(sas_t *sas)
{
    _sasGenComponents(sas, sas->cur_root, sas->cur_comp);
}

static void _sasSplitGroups(sas_t *sas, int fact_id)
{
    const sas_fact_t *fact = sas->fact + fact_id;
    int i, j, action, k;

    fprintf(stdout, "SPLIT %d\n", fact_id);
    for (i = 0; i < fact->edge_size;){
        action = fact->edge[i].action;
        for (j = i + 1; j < fact->edge_size
                && fact->edge[j].action == action; ++j);

        if (j != i + 1){
            fprintf(stdout, "split-group:");
            for (k = i; k < j; ++k)
                fprintf(stdout, " %d", fact->edge[k].fact);
            fprintf(stdout, "\n");
        }

        i = j;
    }
}

static void sasSplitGroups(sas_t *sas)
{
    int i;

    for (i = 0; i < sas->fact_size; ++i){
        if (sas->cur_comp[i])
            _sasSplitGroups(sas, i);
    }
}

static void sasSCC(sas_t *sas)
{
    bor_scc_t scc;
    const int *conn;
    int i, ret;

    sas->cur_root = 20;

    conn = sas->fact[sas->cur_root].conn;
    for (i = 0; i < sas->fact_size; ++i){
        sas->cur_comp[i] = 0;
        if (conn[i] >= 0)
            sas->cur_comp[i] = 1;
    }

    borSCCInit(&scc, sas->fact_size, sasSCCIter, sasSCCNext, sas);
    borSCC1(&scc, sas->cur_root);
    sccPrint(&scc, stdout);

    bzero(sas->cur_comp, sizeof(int) * sas->fact_size);
    for (i = 0; i < scc.comp[0].node_size; ++i)
        sas->cur_comp[scc.comp[0].node[i]] = 1;

    borSCCFree(&scc);

    sasSplitGroups(sas);
    ret = sasCompFixFacts(sas);
    fprintf(stdout, "Fix: %d\n", ret);
    sasPrintComp(sas, stdout);
}

static void sas(plan_pddl_ground_t *g)
{
    sas_t sas;

    sasInit(&sas, g->fact_pool.size);
    sasSetInitFacts(&sas, &g->pddl->init_fact, &g->fact_pool);
    sasMarkExclusiveFacts(&sas, &g->action_pool);
    sasConnectFacts(&sas, &g->action_pool);
    sasSetMustFacts(&sas);
    sasBuildSplitTrees(&sas);
    sasPrintConn(&sas, stdout);
    sasPrintSplitTrees(&sas, stdout);
    sasSCC(&sas);
    sasFree(&sas);
    /*
    fact_size = g->fact_pool.size;
    table_size = fact_size * fact_size;
    sas_table = BOR_CALLOC_ARR(sas_fact_t, table_size);
    sasFillTable(sas_table, fact_size, g);
    sasPrintTable(sas_table, fact_size, stdout);

    sas.table = sas_table;
    sas.size = fact_size;
    sas.root_node = 0;
    borSCCInit(&scc, fact_size, sccIter, sccNext, &sas);
    borSCC1(&scc, 0);
    sccPrint(&scc, stdout);
    sasTight(&sas, scc.comp[0].node, scc.comp[0].node_size);
    borSCCFree(&scc);

    sas.root_node = 6;
    borSCCInit(&scc, fact_size, sccIter, sccNext, &sas);
    borSCC1(&scc, 6);
    sccPrint(&scc, stdout);
    sasTight(&sas, scc.comp[0].node, scc.comp[0].node_size);
    borSCCFree(&scc);

    sasPrintTable(sas_table, fact_size, stdout);
    BOR_FREE(sas_table);
    */
}

#if 0

#define SAS_TABLE(STABLE, FS, X, Y) \
    (STABLE)[(Y) * (FS) + (X)]

static void sasDisableFacts(sas_fact_t *table, int fact_size,
                            const plan_pddl_ground_facts_t *fs)
{
    int i, j;

    for (i = 0; i < fs->size; ++i){
        for (j = i + 1; j < fs->size; ++j){
            SAS_TABLE(table, fact_size, fs->fact[i], fs->fact[j]).conn = -1;
            SAS_TABLE(table, fact_size, fs->fact[j], fs->fact[i]).conn = -1;
        }
    }
}

static void sasDisableInitFacts(sas_fact_t *table, int fact_size,
                                plan_pddl_ground_t *g)
{
    const plan_pddl_fact_t *f;
    int i, j, id;
    int size, fact_ids[g->pddl->init_fact.size];

    size = 0;
    for (i = 0; i < g->pddl->init_fact.size; ++i){
        f = g->pddl->init_fact.fact + i;
        id = planPDDLFactPoolFind(&g->fact_pool, f);
        if (id >= 0)
            fact_ids[size++] = id;
    }

    for (i = 0; i < size; ++i){
        fprintf(stdout, "Init: %d\n", fact_ids[i]);
        for (j = i + 1; j < size; ++j){
            SAS_TABLE(table, fact_size, fact_ids[i], fact_ids[j]).conn = -1;
            SAS_TABLE(table, fact_size, fact_ids[j], fact_ids[i]).conn = -1;
        }
    }
}

static void sasConnectFacts(sas_fact_t *table, int fact_size,
                            const plan_pddl_ground_facts_t *del,
                            const plan_pddl_ground_facts_t *add)
{
    int i, j;

    for (i = 0; i < del->size; ++i){
        for (j = 0; j < add->size; ++j){
            if (SAS_TABLE(table, fact_size, del->fact[i], add->fact[j]).conn == 0){
                SAS_TABLE(table, fact_size, del->fact[i], add->fact[j]).conn = 1;
            }
        }
    }

    if (del->size == 0){
        for (j = 0; j < add->size; ++j){
            SAS_TABLE(table, fact_size, add->fact[j], add->fact[j]).conn = 1;
        }
    }
}

static void sasFillTable(sas_fact_t *table, int fact_size,
                         plan_pddl_ground_t *g)
{
    const plan_pddl_ground_action_pool_t *pool = &g->action_pool;
    const plan_pddl_ground_action_t *a;
    int i;

    sasDisableInitFacts(table, fact_size, g);
    for (i = 0; i < pool->size; ++i){
        a = planPDDLGroundActionPoolGet(pool, i);
        sasDisableFacts(table, fact_size, &a->pre);
        sasDisableFacts(table, fact_size, &a->eff_add);
        sasDisableFacts(table, fact_size, &a->eff_del);
        sasConnectFacts(table, fact_size, &a->eff_del, &a->eff_add);
    }
}

static void sasPrintTable(const sas_fact_t *table, int fact_size,
                          FILE *fout)
{
    int i, j;
    int conn;

    for (i = 0; i < fact_size; ++i){
        for (j = 0; j < fact_size; ++j){
            conn = SAS_TABLE(table, fact_size, i, j).conn;
            if (conn == -1){
                fprintf(fout, "X");
            }else if (conn == 0){
                fprintf(fout, "-");
            }else if (conn == 1){
                fprintf(fout, "C");
            }
        }
        fprintf(fout, "\n");
    }
}

struct _sas_t {
    sas_fact_t *table;
    int size;
    int root_node;
};
typedef struct _sas_t sas_t;

static long sccIter(int node_id, void *ud)
{
    return 0;
}

static int sccNext(int node_id, long *it, void *ud)
{
    sas_t *sas = ud;
    sas_fact_t *root_row = sas->table + (sas->size * sas->root_node);
    sas_fact_t *row = sas->table + (sas->size * node_id);
    int i;

    for (i = *it; i < sas->size; ++i){
        if (row[i].conn && root_row[i].conn != -1){
            *it = i + 1;
            return i;
        }
    }

    return -1;
}

static void sccPrint(const bor_scc_t *scc, FILE *fout)
{
    int i, j;

    for (i = 0; i < scc->comp_size; ++i){
        fprintf(fout, "Component %d:", i);
        for (j = 0; j < scc->comp[i].node_size; ++j){
            fprintf(fout, " %d", scc->comp[i].node[j]);
        }
        fprintf(fout, "\n");
    }
}

static void sasTight(sas_t *sas, const int *comp, int comp_size)
{
    int change, i, *facts;
    sas_fact_t *row;

    facts = BOR_CALLOC_ARR(int, sas->size);
    for (i = 0; i < comp_size; ++i)
        facts[comp[i]] = 1;

    change = 1;
    while (change){
        change = 0;

        for (i = 0; i < sas->size; ++i){
            if (!facts[i])
                continue;

            row = sas->table + (i * sas->size);
            for (int j = 0; j < sas->size; ++j){
                if (row[j].conn == 1 && !facts[j]){
                    fprintf(stdout, "Disable %d (->%d)\n", i, j);
                    facts[i] = 0;
                    change = 1;
                    break;
                }
            }
        }
    }

    for (i = 0; i < sas->size; ++i){
        if (!facts[i])
            continue;

        row = sas->table + (i * sas->size);
        for (int j = 0; j < sas->size; ++j){
            if (!facts[j]){
                row[j].conn = -1;
                sas->table[j * sas->size + i].conn = -1;
            }
        }
    }

    fprintf(stdout, "Tight:");
    for (i = 0; i < sas->size; ++i){
        if (facts[i])
            fprintf(stdout, " %d", i);
    }
    fprintf(stdout, "\n");

    BOR_FREE(facts);
}

static void sas(plan_pddl_ground_t *g)
{
    sas_fact_t *sas_table;
    int fact_size, table_size;
    sas_t sas;
    bor_scc_t scc;

    fact_size = g->fact_pool.size;
    table_size = fact_size * fact_size;
    sas_table = BOR_CALLOC_ARR(sas_fact_t, table_size);
    sasFillTable(sas_table, fact_size, g);
    sasPrintTable(sas_table, fact_size, stdout);

    sas.table = sas_table;
    sas.size = fact_size;
    sas.root_node = 0;
    borSCCInit(&scc, fact_size, sccIter, sccNext, &sas);
    borSCC1(&scc, 0);
    sccPrint(&scc, stdout);
    sasTight(&sas, scc.comp[0].node, scc.comp[0].node_size);
    borSCCFree(&scc);

    sas.root_node = 6;
    borSCCInit(&scc, fact_size, sccIter, sccNext, &sas);
    borSCC1(&scc, 6);
    sccPrint(&scc, stdout);
    sasTight(&sas, scc.comp[0].node, scc.comp[0].node_size);
    borSCCFree(&scc);

    sasPrintTable(sas_table, fact_size, stdout);
    BOR_FREE(sas_table);
}
#endif

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
    planPDDLGroundActionPoolInit(&g->action_pool, &pddl->obj);
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
    planPDDLLiftActionsPrint(&g->lift_action, &g->pddl->predicate, &g->pddl->obj, stdout);

    planPDDLFactPoolAddFacts(&g->fact_pool, &g->pddl->init_fact);
    instActionsNegativePreconditions(&g->lift_action, &g->fact_pool);
    instActions(&g->lift_action, &g->fact_pool, &g->action_pool, g->pddl);
    markStaticFacts(&g->fact_pool, &g->pddl->init_fact);
    planPDDLGroundActionPoolInst(&g->action_pool, &g->fact_pool);
    removeStatAndNegFacts(&g->fact_pool, &g->action_pool);
    sas(g);
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

static void instActionFromFact(const plan_pddl_lift_action_t *lift_action,
                               plan_pddl_fact_pool_t *fact_pool,
                               plan_pddl_ground_action_pool_t *action_pool,
                               int fact_id)
{
    const plan_pddl_fact_t *fact, *pre;
    int i, pre_size = lift_action->pre.size;
    int bound_arg[lift_action->param_size];

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
