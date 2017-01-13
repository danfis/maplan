/***
 * maplan
 * -------
 * Copyright (c)2017 Daniel Fiser <danfis@danfis.cz>,
 * Artificial Intelligence Center, Department of Computer Science,
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
#include "plan/heur.h"
#include "plan/prio_queue.h"
#include "fact_id.h"
#include "fact_op_cross_ref.h"

struct _op_t {
    int op_id;
    int pre_size;
    int pre_unsat;
    plan_cost_t cost;

    int *eff;
    int eff_size;
    int *ext;
    int ext_size;
};
typedef struct _op_t op_t;

struct _fact_t {
    int fact_id[2];
    plan_cost_t value;
    int supp_op;

    int *pre_op;
    int pre_op_size;
};
typedef struct _fact_t fact_t;

struct _plan_heur_h2_max_t {
    plan_heur_t heur;
    plan_fact_id_t fact_id;
    fact_t *fact;
    op_t *op;
    int op_size;
};
typedef struct _plan_heur_h2_max_t plan_heur_h2_max_t;

#define HEUR(parent) bor_container_of(parent, plan_heur_h2_max_t, heur)
#define HEUR_T(heur, parent) plan_heur_h2_max_t *heur = HEUR(parent)

#define FID(h, i, j) ((i) * (h)->fact_id.fact_size + (j))
#define FACT(h, i, j) ((h)->fact[FID(h, i, j)])

static void heurDel(plan_heur_t *_heur);
static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res);

static void addPreOp(plan_heur_h2_max_t *h, int fid1, int fid2, int op_id)
{
    fact_t *fact;

    fact = &FACT(h, fid1, fid2);
    ++fact->pre_op_size;
    fact->pre_op = BOR_REALLOC_ARR(fact->pre_op, int, fact->pre_op_size);
    fact->pre_op[fact->pre_op_size - 1] = op_id;

    if (fid1 != fid2){
        fact = &FACT(h, fid2, fid1);
        ++fact->pre_op_size;
        fact->pre_op = BOR_REALLOC_ARR(fact->pre_op, int, fact->pre_op_size);
        fact->pre_op[fact->pre_op_size - 1] = op_id;
    }

    ++h->op[op_id].pre_size;
}

static void addEffOp(plan_heur_h2_max_t *h, int fid1, int fid2, int op_id)
{
    op_t *op;

    op = h->op + op_id;
    ++op->eff_size;
    op->eff = BOR_REALLOC_ARR(op->eff, int, op->eff_size);
    op->eff[op->eff_size - 1] = FID(h, fid1, fid2);
}

static void addOp(plan_heur_h2_max_t *h, int op_id, const plan_op_t *op,
                  const plan_var_t *var, int var_size)
{
    plan_part_state_pair_t pair1, pair2;
    int i, j, fid1, fid2;

    for (i = 0; i < op->pre->vals_size; ++i){
        pair1 = op->pre->vals[i];
        fid1 = planFactId(&h->fact_id, pair1.var, pair1.val);
        for (j = i; j < op->pre->vals_size; ++j){
            pair2 = op->pre->vals[j];
            fid2 = planFactId(&h->fact_id, pair2.var, pair2.val);
            addPreOp(h, fid1, fid2, op_id);
        }
    }

    for (i = 0; i < op->eff->vals_size; ++i){
        pair1 = op->eff->vals[i];
        fid1 = planFactId(&h->fact_id, pair1.var, pair1.val);
        for (j = i; j < op->eff->vals_size; ++j){
            pair2 = op->eff->vals[j];
            fid2 = planFactId(&h->fact_id, pair2.var, pair2.val);
            addEffOp(h, fid1, fid2, op_id);
        }
    }

    for (i = 0; i < var_size; ++i){
        if (planPartStateGet(op->eff, i) != PLAN_VAL_UNDEFINED)
            continue;
        for (j = 0; j < var[i].range; ++j){
            fid1 = planFactId(&h->fact_id, i, j);
            ++h->op[op_id].ext_size;
            h->op[op_id].ext = BOR_REALLOC_ARR(h->op[op_id].ext, int,
                                               h->op[op_id].ext_size);
            h->op[op_id].ext[h->op[op_id].ext_size - 1] = fid1;
        }
    }

    h->op[op_id].op_id = op_id;
    h->op[op_id].cost = op->cost;
}

plan_heur_t *planHeurH2MaxNew(const plan_var_t *var, int var_size,
                              const plan_part_state_t *goal,
                              const plan_op_t *op, int op_size,
                              unsigned flags)
{
    plan_heur_h2_max_t *heur;
    int i, j, size;

    heur = BOR_ALLOC(plan_heur_h2_max_t);
    bzero(heur, sizeof(*heur));
    _planHeurInit(&heur->heur, heurDel, heurVal, NULL);

    planFactIdInit(&heur->fact_id, var, var_size);

    size = heur->fact_id.fact_size * heur->fact_id.fact_size;
    heur->fact = BOR_CALLOC_ARR(fact_t, size);
    for (i = 0; i < heur->fact_id.fact_size; ++i){
        for (j = 0; j < heur->fact_id.fact_size; ++j){
            FACT(heur, i, j).fact_id[0] = i;
            FACT(heur, i, j).fact_id[1] = j;
        }
    }

    heur->op_size = op_size;
    heur->op = BOR_CALLOC_ARR(op_t, op_size);
    for (i = 0; i < op_size; ++i){
        addOp(heur, i, op + i, var, var_size);
    }

    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    HEUR_T(heur, _heur);
    _planHeurFree(&heur->heur);
    planFactIdFree(&heur->fact_id);
    BOR_FREE(heur->fact);
    BOR_FREE(heur);
}

static void initFacts(plan_heur_h2_max_t *h,
                      const plan_state_t *state,
                      plan_prio_queue_t *queue)
{
    int i, j, fid1, fid2, fid;

    for (i = 0; i < h->fact_id.fact_size; ++i){
        for (j = 0; j < h->fact_id.fact_size; ++j){
            FACT(h, i, j).value = INT_MAX;
            FACT(h, i, j).supp_op = -2;
        }
    }

    for (i = 0; i < state->size; ++i){
        fid1 = planFactId(&h->fact_id, i, planStateGet(state, i));
        for (j = i; j < state->size; ++j){
            fid2 = planFactId(&h->fact_id, j, planStateGet(state, j));
            fid = FID(h, fid1, fid2);
            fprintf(stderr, "Init: %d\n", fid);

            FACT(h, fid1, fid2).value = FACT(h, fid2, fid1).value = 0;
            FACT(h, fid1, fid2).supp_op = FACT(h, fid2, fid1).supp_op = -1;

            planPrioQueuePush(queue, 0, fid);
        }
    }
}

static void initOps(plan_heur_h2_max_t *h)
{
    int i;

    for (i = 0; i < h->op_size; ++i)
        h->op[i].pre_unsat = h->op[i].pre_size;
}

static void addFact(plan_heur_h2_max_t *h, int fact_id,
                    int op_id, plan_cost_t value,
                    plan_prio_queue_t *queue)
{
    fact_t *fact = h->fact + fact_id;
    if (fact->value <= value)
        return;

    fact->value = value;
    fact->supp_op = op_id;
    planPrioQueuePush(queue, value, fact_id);

    if (fact->fact_id[0] != fact->fact_id[1]){
        FACT(h, fact->fact_id[1], fact->fact_id[0]).value = fact->value;
        FACT(h, fact->fact_id[1], fact->fact_id[0]).supp_op = fact->supp_op;
    }
}

static void applyOp(plan_heur_h2_max_t *h,
                    op_t *op, fact_t *src_fact, plan_prio_queue_t *queue)
{
    plan_cost_t value;
    int i, j;

    fprintf(stderr, "applyOp: %d (fact: %d)\n",
            op->op_id, (int)(src_fact - h->fact));
    value = op->cost + src_fact->value;
    for (i = 0; i < op->eff_size; ++i){
        addFact(h, op->eff[i], op->op_id, value, queue);
    }

    for (i = 0; i < op->ext_size; ++i){
        if (FACT(h, op->ext[i], op->ext[i]).supp_op < -1)
            continue;
        for (j = 0; j < op->eff_size; ++j){
            if (h->fact[op->eff[j]].fact_id[0] !=
                    h->fact[op->eff[j]].fact_id[1])
                continue;
            addFact(h, FID(h, op->ext[i], op->eff[j]), op->op_id, value, queue);
        }
    }
}

static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    HEUR_T(h, _heur);
    plan_prio_queue_t queue;
    fact_t *fact;
    int i, op_id, fid;
    plan_cost_t value;

    fprintf(stderr, "heurVal\n");
    planPrioQueueInit(&queue);

    initFacts(h, state, &queue);
    initOps(h);
    while (!planPrioQueueEmpty(&queue)){
        fid = planPrioQueuePop(&queue, &value);
        fact = h->fact + fid;
        if (fact->value != value)
            continue;

        fprintf(stderr, "fid: %d, %d\n", fid, value);
        for (i = 0; i < fact->pre_op_size; ++i){
            op_id = fact->pre_op[i];
            if (h->op[op_id].pre_unsat == 0){
                fprintf(stderr, "ERROR!! -- %d (fact: %d)\n", op_id, fid);
                exit(-1);
            }

            if (--h->op[op_id].pre_unsat == 0){
                applyOp(h, h->op + op_id, fact, &queue);
            }
        }

        if (fact->fact_id[0] == fact->fact_id[1]){
            for (i = 0; i < h->op_size; ++i){
                if (h->op[i].pre_unsat == 0)
                    applyOp(h, h->op + i, fact, &queue);
            }
        }
    }

    planPrioQueueFree(&queue);
}
