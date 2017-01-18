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
#include <boruvka/lifo.h>
#include "plan/heur.h"
#include "plan/prio_queue.h"
#include "plan/problem_2.h"
#include "fact_id.h"
#include "fact_op_cross_ref.h"

struct _op_t {
    int pre_unsat;
    int supp_fact;
    plan_cost_t cost;
};
typedef struct _op_t op_t;

struct _fact_t {
    plan_cost_t value;
    int supp_op;
    int goal_zone;
};
typedef struct _fact_t fact_t;

struct _cut_t {
    int *op;
    int size;
    int alloc;
};
typedef struct _cut_t cut_t;

struct _plan_heur_h2_lm_cut_t {
    plan_heur_t heur;
    plan_problem_2_t p2;
    fact_t *fact;
    op_t *op;
    int *goal_zone;
    int *fact_in_queue;
    cut_t cut;
};
typedef struct _plan_heur_h2_lm_cut_t plan_heur_h2_lm_cut_t;

#define HEUR(parent) bor_container_of(parent, plan_heur_h2_lm_cut_t, heur)
#define HEUR_T(heur, parent) plan_heur_h2_lm_cut_t *heur = HEUR(parent)

#define NO_SUPP -2
#define INIT_STATE_SUPP -1

static void heurDel(plan_heur_t *_heur);
static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res);
static void pruneByH2Mutexes(plan_heur_h2_lm_cut_t *h, const plan_problem_t *p);
static void fullMax(plan_heur_h2_lm_cut_t *h, const plan_state_t *state);

plan_heur_t *planHeurH2LMCutNew(const plan_problem_t *p, unsigned flags)
{
    plan_heur_h2_lm_cut_t *heur;

    heur = BOR_ALLOC(plan_heur_h2_lm_cut_t);
    bzero(heur, sizeof(*heur));
    _planHeurInit(&heur->heur, heurDel, heurVal, NULL);

    planProblem2Init(&heur->p2, p);
    heur->fact = BOR_ALLOC_ARR(fact_t, heur->p2.fact_size);
    heur->op = BOR_ALLOC_ARR(op_t, heur->p2.op_size);
    heur->goal_zone = BOR_ALLOC_ARR(int, heur->p2.fact_size);
    heur->fact_in_queue = BOR_ALLOC_ARR(int, heur->p2.fact_size);

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

static void initFacts(plan_heur_h2_lm_cut_t *h,
                      const plan_state_t *state,
                      plan_prio_queue_t *queue)
{
    int i, *fs, size;

    for (i = 0; i < h->p2.fact_size; ++i){
        h->fact[i].value = PLAN_COST_MAX;
        h->fact[i].supp_op = NO_SUPP;
        h->fact[i].goal_zone = 0;
    }

    fs = planFactId2State(&h->p2.fact_id, state, &size);
    for (i = 0; i < size; ++i){
        h->fact[fs[i]].value = 0;
        h->fact[fs[i]].supp_op = INIT_STATE_SUPP;
        planPrioQueuePush(queue, 0, fs[i]);
    }

    if (fs != NULL)
        BOR_FREE(fs);
}

static void initOps(plan_heur_h2_lm_cut_t *h)
{
    int i;

    for (i = 0; i < h->p2.op_size; ++i){
        h->op[i].pre_unsat = h->p2.op[i].pre_size;
        h->op[i].supp_fact = NO_SUPP;
        h->op[i].cost = h->p2.op[i].cost;
    }
}

static void addFact(plan_heur_h2_lm_cut_t *h, int fact_id,
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

static void applyOp(plan_heur_h2_lm_cut_t *h, int op_id,
                    fact_t *fact, int fact_id, plan_prio_queue_t *queue)
{
    plan_op_2_t *op = h->p2.op + op_id;
    plan_cost_t value;
    int i;

    value = h->op[op_id].cost + fact->value;
    for (i = 0; i < op->eff_size; ++i)
        addFact(h, op->eff[i], op_id, value, queue);

    for (i = 0; i < op->child_size; ++i){
        h->op[op->child[i]].pre_unsat -= op->pre_size;
        if (h->op[op->child[i]].pre_unsat < 0){
            fprintf(stderr, "ERROR: pre_unsat < 0, op_id: %d, parent: %d\n",
                    op->child[i], op_id);
            exit(-1);
        }

        if (h->op[op->child[i]].pre_unsat == 0)
            applyOp(h, op->child[i], fact, fact_id, queue);
    }
}

static void fullMax(plan_heur_h2_lm_cut_t *h, const plan_state_t *state)
{
    plan_prio_queue_t queue;
    fact_t *fact;
    plan_fact_2_t *fact2;
    int i, op_id, fact_id, value;

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
                applyOp(h, op_id, fact, fact_id, &queue);
            }
        }
    }

    planPrioQueueFree(&queue);
}

static void incMax(plan_heur_h2_lm_cut_t *h, const plan_state_t *state)
{
    plan_prio_queue_t queue;
    fact_t *fact;
    plan_fact_2_t *fact2;
    int i, op_id, fact_id, value;

    planPrioQueueInit(&queue);

    initFacts(h, state, &queue);
    for (i = 0; i < h->p2.op_size; ++i){
        h->op[i].pre_unsat = h->p2.op[i].pre_size;
        h->op[i].supp_fact = NO_SUPP;
    }
    fprintf(stderr, "GGG: %d, supp: %d\n", h->fact[h->p2.goal_fact_id].value,
            h->fact[h->p2.goal_fact_id].supp_op);

    fprintf(stderr, "OP>0:");
    for (i = 0; i < h->p2.op_size; ++i){
        if (h->op[i].cost > 0)
            fprintf(stderr, " %d", i);
    }
    fprintf(stderr, "\n");

    while (!planPrioQueueEmpty(&queue)){
        fact_id = planPrioQueuePop(&queue, &value);
        fact = h->fact + fact_id;
        if (fact->value != value)
            continue;

        fact2 = h->p2.fact + fact_id;
        for (i = 0; i < fact2->pre_op_size; ++i){
            op_id = fact2->pre_op[i];
            if (h->op[op_id].pre_unsat == 0){
                fprintf(stderr, "incERROR!! -- %d (fact: %d)\n", op_id, fact_id);
                exit(-1);
            }

            if (--h->op[op_id].pre_unsat == 0){
                h->op[op_id].supp_fact = fact_id;
                applyOp(h, op_id, fact, fact_id, &queue);
            }
        }
    }

    planPrioQueueFree(&queue);

    int fi = h->p2.goal_fact_id;
    while (fi >= 0 && h->fact[fi].supp_op >= 0){
        fprintf(stderr, "BT: %d, value: %d, supp_op: %d (%d)\n",
                fi, h->fact[fi].value, h->fact[fi].supp_op,
                h->op[h->fact[fi].supp_op].cost);
        fi = h->op[h->fact[fi].supp_op].supp_fact;
    }
    fprintf(stderr, "GGG2: %d, supp: %d, %d(%d) -- %d\n",
            h->fact[h->p2.goal_fact_id].value,
            h->fact[h->p2.goal_fact_id].supp_op,
            h->op[h->fact[h->p2.goal_fact_id].supp_op].supp_fact,
            h->fact[h->op[h->fact[h->p2.goal_fact_id].supp_op].supp_fact].value,
            h->fact[h->op[h->fact[h->p2.goal_fact_id].supp_op].supp_fact].supp_op);
}

static void markGoalZoneRec(plan_heur_h2_lm_cut_t *h, int fact_id)
{
    plan_fact_2_t *fact = h->p2.fact + fact_id;
    op_t *op;
    int i;

    if (h->goal_zone[fact_id])
        return;

    h->goal_zone[fact_id] = 1;

    for (i = 0; i < fact->eff_op_size; ++i){
        op = h->op + fact->eff_op[i];
        if (op->cost == 0 && op->supp_fact >= 0){
            fprintf(stderr, "op: %d (%d), (supp: %d)\n", fact->eff_op[i],
                    h->op[fact->eff_op[i]].cost,
                    h->op[fact->eff_op[i]].supp_fact);
            markGoalZoneRec(h, op->supp_fact);
        }
    }
}

static void markGoalZone(plan_heur_h2_lm_cut_t *h)
{
    bzero(h->goal_zone, sizeof(int) * h->p2.fact_size);
    markGoalZoneRec(h, h->p2.goal_fact_id);

    fprintf(stderr, "GZ:");
    for (int i = 0; i < h->p2.fact_size; ++i){
        if (h->goal_zone[i])
            fprintf(stderr, " %d", i);
    }
    fprintf(stderr, "\n");
}

static void addToCut(plan_heur_h2_lm_cut_t *h, int op_id)
{
    if (h->cut.size == h->cut.alloc){
        if (h->cut.alloc == 0)
            h->cut.alloc = 1;
        h->cut.alloc *= 2;
        h->cut.op = BOR_REALLOC_ARR(h->cut.op, int, h->cut.alloc);
    }
    h->cut.op[h->cut.size++] = op_id;
}

static void findCutAddInit(plan_heur_h2_lm_cut_t *h,
                           bor_lifo_t *queue)
{
    int i;

    fprintf(stderr, "Init:");
    for (i = 0; i < h->p2.fact_size; ++i){
        if (h->fact[i].supp_op == INIT_STATE_SUPP){
            h->fact_in_queue[i] = 1;
            borLifoPush(queue, &i);
            fprintf(stderr, " %d", i);
        }
    }
    fprintf(stderr, "\n");

    // TODO: Empty preconditions
}

static void findCutEnqueueEff(plan_heur_h2_lm_cut_t *h,
                              int op_id, bor_lifo_t *queue)
{
    const plan_op_2_t *op = h->p2.op + op_id;
    int i, fact_id;
    int in_cut = 0;

    for (i = 0; i < op->eff_size; ++i){
        fact_id = op->eff[i];

        // Determine whether the operator belongs to cut
        if (!in_cut && h->goal_zone[fact_id]){
            addToCut(h, op_id);
            in_cut = 1;
        }

        if (!h->fact_in_queue[fact_id] && !h->goal_zone[fact_id]){
            h->fact_in_queue[fact_id] = 1;
            borLifoPush(queue, &fact_id);
        }
    }
}

static void findCut(plan_heur_h2_lm_cut_t *h)
{
    int i, fact_id;
    plan_fact_2_t *fact;
    bor_lifo_t queue;

    // Zeroize in-queue flags
    bzero(h->fact_in_queue, sizeof(int) * h->p2.fact_size);

    // Reset output structure
    h->cut.size = 0;

    // Initialize queue and adds initial state
    borLifoInit(&queue, sizeof(int));
    findCutAddInit(h, &queue);

    while (!borLifoEmpty(&queue)){
        // Pop next fact from queue
        fact_id = *(int *)borLifoBack(&queue);
        borLifoPop(&queue);

        fact = h->p2.fact + fact_id;
        for (i = 0; i < fact->pre_op_size; ++i){
            if (h->op[fact->pre_op[i]].supp_fact == fact_id)
                findCutEnqueueEff(h, fact->pre_op[i], &queue);
        }
    }

    borLifoFree(&queue);
}

static void substractCutCost(plan_heur_h2_lm_cut_t *h, int op_id, int cost)
{
    int i;

    if (h->p2.op[op_id].parent != -1)
        op_id = h->p2.op[op_id].parent;

    fprintf(stderr, "substract: %d %d\n", op_id, cost);
    h->op[op_id].cost -= cost;
    for (i = 0; i < h->p2.op[op_id].child_size; ++i){
        h->op[h->p2.op[op_id].child[i]].cost -= cost;
        fprintf(stderr, "substract: %d %d\n", h->p2.op[op_id].child[i], cost);
    }
}

static plan_cost_t updateCutCost(plan_heur_h2_lm_cut_t *h)
{
    const cut_t *cut = &h->cut;
    op_t *op = h->op;
    int cut_cost = INT_MAX;
    int i, len, op_id;
    const int *ops;

    len = cut->size;
    ops = cut->op;

    // Find minimal cost from the cut operators
    cut_cost = op[ops[0]].cost;
    for (i = 1; i < len; ++i){
        op_id = ops[i];
        cut_cost = BOR_MIN(cut_cost, op[op_id].cost);
    }

    fprintf(stderr, "CCC\n");
    // Substract the minimal cost from all cut operators
    for (i = 0; i < len; ++i){
        h->op[ops[i]].cost -= cut_cost;
        //substractCutCost(h, ops[i], cut_cost);
    }

    return cut_cost;
}

static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    HEUR_T(h, _heur);
    plan_cost_t hval = 0;

    // Compute initial h^2
    fullMax(h, state);

    // Check whether the goal is reachable.
    if (h->fact[h->p2.goal_fact_id].value == PLAN_COST_MAX){
        // Goal is not reachable, return dead-end
        res->heur = PLAN_HEUR_DEAD_END;
        return;
    }

    while (h->fact[h->p2.goal_fact_id].value > 0){
        fprintf(stderr, "goal value: %d\n", h->fact[h->p2.goal_fact_id].value);
        // Mark facts connected with the goal by zero-cost operators in
        // justification graph.
        markGoalZone(h);

        // Find operators that are reachable from the initial state and are
        // connected with the goal-zone facts in justification graph.
        findCut(h);

        // If no cut was found we have reached dead-end
        if (h->cut.size == 0){
            fprintf(stderr, "Error: LM-Cut: Empty cut! Something is"
                            " seriously wrong!\n");
            exit(-1);
        }
        fprintf(stderr, "Cut:");
        for (int i = 0; i < h->cut.size; ++i)
            fprintf(stderr, " %d", h->cut.op[i]);
        fprintf(stderr, "\n");

        /*
        // Store landmarks into output structure if requested.
        if (res->save_landmarks)
            storeLandmarks(heur, heur->cut.op, heur->cut.size, res);
        */

        // Determine the minimal cost from all cut-operators. Substract
        // this cost from their cost and add it to the final heuristic
        // value.
        hval += updateCutCost(h);
        fprintf(stderr, "hval: %d\n", hval);

        // Performat incremental h^max computation using changed operator
        // costs
        incMax(h, state);
    }

    res->heur = hval;
}

static void pruneByH2Mutexes(plan_heur_h2_lm_cut_t *h, const plan_problem_t *p)
{
    PLAN_STATE_STACK(state, p->var_size);

    planStatePoolGetState(p->state_pool, p->initial_state, &state);
    fullMax(h, &state);
    for (int i = 0; i < h->p2.fact_size; ++i){
        if (h->fact[i].value == PLAN_COST_MAX)
            planProblem2PruneByMutex(&h->p2, i);
    }
    planProblem2PruneEmptyOps(&h->p2);
}

