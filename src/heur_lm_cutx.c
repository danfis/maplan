/***
 * maplan
 * -------
 * Copyright (c)2017 Daniel Fiser <danfis@danfis.cz>,
 * AIC, Department of Computer Science,
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
#include "plan/arr.h"
#include "plan/pq.h"
#include "plan/fact_id.h"
#include "plan/heur.h"

struct _op_t {
    plan_arr_int_t eff; /*!< Facts in its effect */
    int op_cost;        /*!< Original operator's cost */
    int pre_size;       /*!< Number of preconditions */

    int cost;           /*!< Current cost of the operator */
    int unsat;          /*!< Number of unsatisfied preconditions */
    int supp;           /*!< Supporter fact (that maximizes h^max) */
};
typedef struct _op_t op_t;

struct _fact_t {
    plan_arr_int_t pre_op; /*!< Operators having this fact as its precond */
    plan_arr_int_t eff_op; /*!< Operators having this fact as its effect */
    plan_pq_el_t heap;     /*!< Connection to priority heap */
};
typedef struct _fact_t fact_t;

#define FID(heur, f) ((f) - (heur)->fact)
#define FVALUE(fact) (fact)->heap.key
#define FVALUE_SET(fact, val) do { (fact)->heap.key = val; } while(0)
#define FVALUE_INIT(fact) FVALUE_SET((fact), INT_MAX)
#define FVALUE_IS_SET(fact) (FVALUE(fact) != INT_MAX)
#define FPUSH(pq, value, fact) \
    do { \
    if (FVALUE_IS_SET(fact)){ \
        planPQUpdate((pq), (value), &(fact)->heap); \
    }else{ \
        planPQPush((pq), (value), &(fact)->heap); \
    } \
    } while (0)

#define SET_OP_SUPP(h, op, fact_id) \
    do { \
        (op)->supp = (fact_id); \
        (h)->fact_supp[fact_id] = 1; \
    } while (0)

struct _plan_heur_lm_cut_t {
    plan_heur_t heur;
    plan_fact_id_t fact_id;

    fact_t *fact;
    int fact_size;
    int fact_goal;
    int fact_nopre;
    int *fact_supp;

    op_t *op;
    int op_size;
    int op_goal;

    plan_arr_int_t state; /*!< Current state from which heur is computed */
    plan_arr_int_t cut;   /*!< Current cut */

    /** Auxiliary structures to avoid re-allocation */
    int *fact_state;
    plan_arr_int_t queue;
    plan_pq_t pq;
};
typedef struct _plan_heur_lm_cut_t plan_heur_lm_cut_t;

#define HEUR(parent) bor_container_of((parent), plan_heur_lm_cut_t, heur)

static void heurDel(plan_heur_t *_heur);
static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res);

static void opFree(op_t *op);
static void factFree(fact_t *fact);
static void loadOpFact(plan_heur_lm_cut_t *h, const plan_problem_t *p);

plan_heur_t *planHeurLMCutXNew(const plan_problem_t *p, unsigned flags)
{
    plan_heur_lm_cut_t *h;

    h = BOR_ALLOC(plan_heur_lm_cut_t);
    bzero(h, sizeof(*h));
    _planHeurInit(&h->heur, heurDel, heurVal, NULL);
    planFactIdInit(&h->fact_id, p->var, p->var_size, 0);
    loadOpFact(h, p);
    h->fact_supp = BOR_ALLOC_ARR(int, h->fact_size);

    h->fact_state = BOR_ALLOC_ARR(int, h->fact_size);
    planArrIntInit(&h->queue, h->fact_size / 2);
    planPQInit(&h->pq);

    return &h->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_lm_cut_t *h = HEUR(_heur);
    int i;

    _planHeurFree(&h->heur);

    for (i = 0; i < h->fact_size; ++i)
        factFree(h->fact + i);
    if (h->fact)
        BOR_FREE(h->fact);
    if (h->fact_supp)
        BOR_FREE(h->fact_supp);

    for (i = 0; i < h->op_size; ++i)
        opFree(h->op + i);
    if (h->op)
        BOR_FREE(h->op);

    if (h->fact_state)
        BOR_FREE(h->fact_state);
    planArrIntFree(&h->queue);
    planPQFree(&h->pq);

    planFactIdFree(&h->fact_id);
    BOR_FREE(h);
}

static void initFacts(plan_heur_lm_cut_t *h)
{
    int i;

    for (i = 0; i < h->fact_size; ++i){
        FVALUE_INIT(h->fact + i);
    }
    bzero(h->fact_supp, sizeof(int) * h->fact_size);
}

static void initOps(plan_heur_lm_cut_t *h, int init_cost)
{
    int i;

    for (i = 0; i < h->op_size; ++i){
        h->op[i].unsat = h->op[i].pre_size;
        h->op[i].supp = -1;
        if (init_cost)
            h->op[i].cost = h->op[i].op_cost;
    }
}

static void addInitState(plan_heur_lm_cut_t *h,
                         const plan_state_t *state,
                         plan_pq_t *pq)
{
    int fact_id;

    h->state.size = 0;
    PLAN_FACT_ID_FOR_EACH_STATE(&h->fact_id, state, fact_id){
        FPUSH(pq, 0, h->fact + fact_id);
        planArrIntAdd(&h->state, fact_id);
    }
    FPUSH(pq, 0, h->fact + h->fact_nopre);
    planArrIntAdd(&h->state, h->fact_nopre);
}

static void enqueueOpEffects(plan_heur_lm_cut_t *h,
                             op_t *op, int fact_value,
                             plan_pq_t *pq)
{
    fact_t *fact;
    int value = op->cost + fact_value;
    int i;

    for (i = 0; i < op->eff.size; ++i){
        fact = h->fact + op->eff.arr[i];
        if (FVALUE(fact) > value)
            FPUSH(pq, value, fact);
    }
}

static void hMaxFull(plan_heur_lm_cut_t *h, const plan_state_t *state,
                     int init_cost)
{
    plan_pq_t pq;
    plan_pq_el_t *el;
    fact_t *fact;
    op_t *op;
    int i, value;

    planPQInit(&pq);
    initFacts(h);
    initOps(h, init_cost);
    addInitState(h, state, &pq);
    while (!planPQEmpty(&pq)){
        el = planPQPop(&pq, &value);
        fact = bor_container_of(el, fact_t, heap);

        for (i = 0; i < fact->pre_op.size; ++i){
            op = h->op + fact->pre_op.arr[i];
            if (--op->unsat == 0){
                // Set as supporter the last fact that enabled this
                // operator (it must be one of those that have maximum
                // value
                SET_OP_SUPP(h, op, fact - h->fact);
                enqueueOpEffects(h, op, value, &pq);
            }
        }
    }
    planPQFree(&pq);
}

#define CUT_UNDEF 0
#define CUT_INIT 1
#define CUT_GOAL 2

/** Mark facts connected with the goal with zero cost paths */
static void markGoalZone(plan_heur_lm_cut_t *h)
{
    fact_t *fact;
    op_t *op;
    int fact_id, op_id;

    h->queue.size = 0;
    planArrIntAdd(&h->queue, h->fact_goal);
    h->fact_state[h->fact_goal] = CUT_GOAL;
    while (h->queue.size > 0){
        fact_id = h->queue.arr[--h->queue.size];
        fact = h->fact + fact_id;
        PLAN_ARR_INT_FOR_EACH(&fact->eff_op, op_id){
            op = h->op + op_id;
            if (op->supp >= 0
                    && op->cost == 0
                    && h->fact_state[op->supp] == CUT_UNDEF){
                planArrIntAdd(&h->queue, op->supp);
                h->fact_state[op->supp] = CUT_GOAL;
            }
        }
    }

    /*
    fprintf(stderr, "gz:");
    for (fact_id = 0; fact_id < h->fact_size; ++fact_id){
        if (h->fact_state[fact_id] == CUT_GOAL)
            fprintf(stderr, " %d", fact_id);
    }
    fprintf(stderr, "\n");
    */
}

/** Finds cut (and fills h->cut) and returns cost of the cut.
 *  Requires marked goal zone. */
static int findCut(plan_heur_lm_cut_t *h)
{
    op_t *op;
    int fact_id, op_id, next;
    int min_cost = INT_MAX;

    h->queue.size = 0;
    PLAN_ARR_INT_FOR_EACH(&h->state, fact_id){
        if (h->fact_state[fact_id] == CUT_UNDEF){
            planArrIntAdd(&h->queue, fact_id);
            h->fact_state[fact_id] = CUT_INIT;
        }
    }

    h->cut.size = 0;
    while (h->queue.size > 0){
        fact_id = h->queue.arr[--h->queue.size];
        PLAN_ARR_INT_FOR_EACH(&h->fact[fact_id].pre_op, op_id){
            op = h->op + op_id;
            if (op->supp != fact_id)
                continue;
            PLAN_ARR_INT_FOR_EACH(&op->eff, next){
                if (h->fact_state[next] == CUT_UNDEF){
                    if (h->fact_supp[next]){
                        h->fact_state[next] = CUT_INIT;
                        planArrIntAdd(&h->queue, next);
                    }

                }else if (h->fact_state[next] == CUT_GOAL){
                    planArrIntAdd(&h->cut, op_id);
                    min_cost = BOR_MIN(min_cost, op->cost);
                }
            }
        }
    }
    planArrIntSort(&h->cut);
    planArrIntUniq(&h->cut);

    /*
    fprintf(stderr, "Cut(%d):", min_cost);
    PLAN_ARR_INT_FOR_EACH(&h->cut, op_id)
        fprintf(stderr, " %d", op_id);
    fprintf(stderr, "\n");
    */

    if (h->cut.size == 0){
        fprintf(stderr, "ERROR: Empty cut!\n");
        exit(-1);
    }else if (min_cost <= 0){
        fprintf(stderr, "ERROR: Invalid cut cost: %d!\n", min_cost);
        exit(-1);
    }

    return min_cost;
}

/** Decrease cost of the operators in the cut */
static void applyCutCost(plan_heur_lm_cut_t *h, int min_cost)
{
    int op_id;

    PLAN_ARR_INT_FOR_EACH(&h->cut, op_id){
        h->op[op_id].cost -= min_cost;
    }
}

/** Perform cut */
static int cut(plan_heur_lm_cut_t *h)
{
    int cost;

    bzero(h->fact_state, sizeof(int) * h->fact_size);
    markGoalZone(h);
    cost = findCut(h);
    applyCutCost(h, cost);
    return cost;
}

static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    plan_heur_lm_cut_t *h = HEUR(_heur);

    res->heur = 0;

    hMaxFull(h, state, 1);
    if (!FVALUE_IS_SET(h->fact + h->fact_goal)){
        res->heur = PLAN_HEUR_DEAD_END;
        return;
    }

    while (FVALUE(h->fact + h->fact_goal) > 0){
        res->heur += cut(h);
        hMaxFull(h, state, 0);
    }
}

static void opFree(op_t *op)
{
    planArrIntFree(&op->eff);
}

static void factFree(fact_t *fact)
{
    planArrIntFree(&fact->pre_op);
    planArrIntFree(&fact->eff_op);
}

static void loadOpFact(plan_heur_lm_cut_t *h, const plan_problem_t *p)
{
    op_t *op;
    int op_id, fid, size;

    // Allocate facts and add one for empty-precondition fact and one for
    // goal fact
    h->fact_size = h->fact_id.fact_size + 2;
    h->fact = BOR_CALLOC_ARR(fact_t, h->fact_size);
    h->fact_goal = h->fact_size - 2;
    h->fact_nopre = h->fact_size - 1;

    // Allocate operators and add one artificial for goal
    h->op_size = p->op_size + 1;
    h->op = BOR_CALLOC_ARR(op_t, h->op_size);
    h->op_goal = h->op_size - 1;

    for (op_id = 0; op_id < p->op_size; ++op_id){
        // TODO: Conditional effects
        op = h->op + op_id;
        op->eff.arr = planFactIdPartState2(&h->fact_id, p->op[op_id].eff,
                                           &size);
        op->eff.alloc = op->eff.size = size;
        op->op_cost = p->op[op_id].cost;

        PLAN_FACT_ID_FOR_EACH_PART_STATE(&h->fact_id, p->op[op_id].pre, fid)
            planArrIntAdd(&h->fact[fid].pre_op, op_id);
        op->pre_size = p->op[op_id].pre->vals_size;

        // Record operator with no preconditions
        if (p->op[op_id].pre->vals_size == 0){
            planArrIntAdd(&h->fact[h->fact_nopre].pre_op, op_id);
            op->pre_size = 1;
        }

        PLAN_ARR_INT_FOR_EACH(&op->eff, fid){
            planArrIntAdd(&h->fact[fid].eff_op, op_id);
        }
    }

    // Set up goal operator
    op = h->op + h->op_goal;
    planArrIntAdd(&op->eff, h->fact_goal);
    op->op_cost = 0;

    PLAN_FACT_ID_FOR_EACH_PART_STATE(&h->fact_id, p->goal, fid)
        planArrIntAdd(&h->fact[fid].pre_op, h->op_goal);
    op->pre_size = p->goal->vals_size;
    planArrIntAdd(&h->fact[h->fact_goal].eff_op, h->op_goal);
}


