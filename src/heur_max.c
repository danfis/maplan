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
    int cost;           /*!< Cost of the operator */
    int pre_size;       /*!< Number of preconditions */
    int unsat;          /*!< Number of unsatisfied preconditions */
};
typedef struct _op_t op_t;

struct _fact_t {
    plan_arr_int_t pre_op; /*!< Operators having this fact as its
                                precondition */
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

struct _plan_heur_max_t {
    plan_heur_t heur;
    plan_fact_id_t fact_id;

    fact_t *fact;
    int fact_size;
    int fact_goal;
    int fact_nopre;

    op_t *op;
    int op_size;
    int op_goal;
};
typedef struct _plan_heur_max_t plan_heur_max_t;

#define HEUR(parent) bor_container_of((parent), plan_heur_max_t, heur)

static void heurDel(plan_heur_t *_heur);
static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res);

static void opFree(op_t *op);
static void factFree(fact_t *fact);
static void loadOpFact(plan_heur_max_t *h, const plan_problem_t *p);

plan_heur_t *planHeurMaxNew(const plan_problem_t *p, unsigned flags)
{
    plan_heur_max_t *h;

    h = BOR_ALLOC(plan_heur_max_t);
    bzero(h, sizeof(*h));
    _planHeurInit(&h->heur, heurDel, heurVal, NULL);
    planFactIdInit(&h->fact_id, p->var, p->var_size, 0);
    loadOpFact(h, p);

    return &h->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_max_t *h = HEUR(_heur);
    int i;

    _planHeurFree(&h->heur);
    planFactIdFree(&h->fact_id);

    for (i = 0; i < h->fact_size; ++i)
        factFree(h->fact + i);
    if (h->fact)
        BOR_FREE(h->fact);

    for (i = 0; i < h->op_size; ++i)
        opFree(h->op + i);
    if (h->op)
        BOR_FREE(h->op);
    BOR_FREE(h);
}

static void initFacts(plan_heur_max_t *h)
{
    int i;

    for (i = 0; i < h->fact_size; ++i){
        FVALUE_INIT(h->fact + i);
    }
}

static void initOps(plan_heur_max_t *h)
{
    int i;

    for (i = 0; i < h->op_size; ++i){
        h->op[i].unsat = h->op[i].pre_size;
    }
}

static void addInitState(plan_heur_max_t *h,
                         const plan_state_t *state,
                         plan_pq_t *pq)
{
    int fact_id;

    PLAN_FACT_ID_FOR_EACH_STATE(&h->fact_id, state, fact_id)
        FPUSH(pq, 0, h->fact + fact_id);
    FPUSH(pq, 0, h->fact + h->fact_nopre);
}

static void enqueueOpEffects(plan_heur_max_t *h,
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

static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    plan_heur_max_t *h = HEUR(_heur);
    plan_pq_t pq;
    plan_pq_el_t *el;
    fact_t *fact;
    op_t *op;
    int i, fact_id, value;

    planPQInit(&pq);
    initFacts(h);
    initOps(h);
    addInitState(h, state, &pq);
    while (!planPQEmpty(&pq)){
        el = planPQPop(&pq, &value);
        fact = bor_container_of(el, fact_t, heap);

        fact_id = FID(h, fact);
        if (fact_id == h->fact_goal)
            break;

        for (i = 0; i < fact->pre_op.size; ++i){
            op = h->op + fact->pre_op.arr[i];
            if (--op->unsat == 0)
                enqueueOpEffects(h, op, value, &pq);
        }
    }
    planPQFree(&pq);

    res->heur = PLAN_HEUR_DEAD_END;
    if (FVALUE_IS_SET(h->fact + h->fact_goal))
        res->heur = FVALUE(h->fact + h->fact_goal);
}

static void opFree(op_t *op)
{
    planArrIntFree(&op->eff);
}

static void factFree(fact_t *fact)
{
    planArrIntFree(&fact->pre_op);
}

static void loadOpFact(plan_heur_max_t *h, const plan_problem_t *p)
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
        op->cost = p->op[op_id].cost;

        PLAN_FACT_ID_FOR_EACH_PART_STATE(&h->fact_id, p->op[op_id].pre, fid)
            planArrIntAdd(&h->fact[fid].pre_op, op_id);
        op->pre_size = p->op[op_id].pre->vals_size;

        // Record operator with no preconditions
        if (p->op[op_id].pre->vals_size == 0){
            planArrIntAdd(&h->fact[h->fact_nopre].pre_op, op_id);
            op->pre_size = 1;
        }
    }

    // Set up goal operator
    op = h->op + h->op_goal;
    planArrIntAdd(&op->eff, h->fact_goal);
    op->cost = 0;

    PLAN_FACT_ID_FOR_EACH_PART_STATE(&h->fact_id, p->goal, fid)
        planArrIntAdd(&h->fact[fid].pre_op, h->op_goal);
    op->pre_size = p->goal->vals_size;
}

