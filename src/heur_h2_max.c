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
#include "plan/problem_2.h"
#include "fact_id.h"
#include "fact_op_cross_ref.h"

struct _op_t {
    int pre_unsat;
    int supp_fact;
};
typedef struct _op_t op_t;

struct _fact_t {
    plan_cost_t value;
    int supp_op;
};
typedef struct _fact_t fact_t;

struct _plan_heur_h2_max_t {
    plan_heur_t heur;
    plan_problem_2_t p2;
    fact_t *fact;
    op_t *op;
};
typedef struct _plan_heur_h2_max_t plan_heur_h2_max_t;

#define HEUR(parent) bor_container_of(parent, plan_heur_h2_max_t, heur)
#define HEUR_T(heur, parent) plan_heur_h2_max_t *heur = HEUR(parent)

static void heurDel(plan_heur_t *_heur);
static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res);
static void pruneByH2Mutexes(plan_heur_h2_max_t *h, const plan_problem_t *p);

plan_heur_t *planHeurH2MaxNew(const plan_problem_t *p, unsigned flags)
{
    plan_heur_h2_max_t *heur;

    heur = BOR_ALLOC(plan_heur_h2_max_t);
    bzero(heur, sizeof(*heur));
    _planHeurInit(&heur->heur, heurDel, heurVal, NULL);

    planProblem2Init(&heur->p2, p);
    heur->fact = BOR_ALLOC_ARR(fact_t, heur->p2.fact_id.fact_size);
    heur->op = BOR_ALLOC_ARR(op_t, heur->p2.op_size);

    pruneByH2Mutexes(heur, p);

    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    HEUR_T(heur, _heur);
    BOR_FREE(heur->op);
    BOR_FREE(heur->fact);
    planProblem2Free(&heur->p2);
    _planHeurFree(&heur->heur);
    BOR_FREE(heur);
}

static void initFacts(plan_heur_h2_max_t *h,
                      const plan_state_t *state,
                      plan_prio_queue_t *queue)
{
    int i, *fs, size;

    for (i = 0; i < h->p2.fact_id.fact_size; ++i){
        h->fact[i].value = PLAN_COST_MAX;
        h->fact[i].supp_op = -2;
    }

    fs = planFactId2State(&h->p2.fact_id, state, &size);
    for (i = 0; i < size; ++i){
        h->fact[fs[i]].value = 0;
        h->fact[fs[i]].supp_op = -1;
        planPrioQueuePush(queue, 0, fs[i]);
    }

    if (fs != NULL)
        BOR_FREE(fs);
}

static void initOps(plan_heur_h2_max_t *h)
{
    int i;

    for (i = 0; i < h->p2.op_size; ++i){
        h->op[i].pre_unsat = h->p2.op[i].pre_size;
        h->op[i].supp_fact = -1;
    }
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
}

static void applyOp(plan_heur_h2_max_t *h, int op_id,
                    fact_t *fact, int fact_id, plan_prio_queue_t *queue)
{
    plan_op_2_t *op = h->p2.op + op_id;
    plan_cost_t value;
    int i;

    value = op->cost + fact->value;
    for (i = 0; i < op->eff_size; ++i)
        addFact(h, op->eff[i], op_id, value, queue);
}

static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    HEUR_T(h, _heur);
    plan_prio_queue_t queue;
    fact_t *fact;
    plan_fact_2_t *fact2;
    int i, op_id, fact_id, value;

    res->heur = PLAN_HEUR_DEAD_END;

    planPrioQueueInit(&queue);

    initFacts(h, state, &queue);
    initOps(h);
    while (!planPrioQueueEmpty(&queue)){
        fact_id = planPrioQueuePop(&queue, &value);
        fact = h->fact + fact_id;
        if (fact->value != value)
            continue;

        fact2 = h->p2.fact + fact_id;
        for (i = 0; i < fact2->pre_op_size; ++i){
            op_id = fact2->pre_op[i];
            if (h->op[op_id].pre_unsat == 0){
                fprintf(stderr, "ERROR!! -- %d (fact: %d)\n", op_id, fact_id);
                exit(-1);
            }

            if (--h->op[op_id].pre_unsat == 0){
                h->op[op_id].supp_fact = fact_id;

                if (op_id == h->p2.goal_op_id){
                    res->heur = fact->value;
                    planPrioQueueFree(&queue);
                    return;
                }

                applyOp(h, op_id, fact, fact_id, &queue);
            }
        }
    }

    planPrioQueueFree(&queue);
}

static void pruneByH2Mutexes(plan_heur_h2_max_t *h, const plan_problem_t *p)
{
    PLAN_STATE_STACK(state, p->var_size);
    plan_heur_res_t res;

    planStatePoolGetState(p->state_pool, p->initial_state, &state);
    planHeurResInit(&res);
    heurVal(&h->heur, &state, &res);
    for (int i = 0; i < h->p2.fact_id.fact_size; ++i){
        if (h->fact[i].value == PLAN_COST_MAX)
            planProblem2PruneByMutex(&h->p2, i);
    }
    planProblem2PruneEmptyOps(&h->p2);
}
