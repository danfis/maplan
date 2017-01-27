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
    int *pre2_size;     /*!< Number of preconditions of the extended ops */

    int unsat;          /*!< Number of unsatisfied preconditions */
    int *unsat2;        /*!< As .unsat but from .pre2_size */
};
typedef struct _op_t op_t;

struct _fact_t {
    plan_arr_int_t pre_op; /*!< Operators having this fact as its
                                precondition */
    plan_arr_int_t pre2_op; /*!< Operators that can be extended by this fact */
    plan_arr_int_t pre2_op_fact; /*!< Extension fact corresponding to
                                      .pre2_op */
    plan_pq_el_t heap;     /*!< Connection to priority heap */
};
typedef struct _fact_t fact_t;

/** Returns fact ID from fact object */
#define FID(heur, f) ((f) - (heur)->fact)
/** Returns current value of the fact */
#define FVALUE(fact) (fact)->heap.key
/** Set value of the fact */
#define FVALUE_SET(fact, val) do { (fact)->heap.key = val; } while(0)
/** Initialize fact value */
#define FVALUE_INIT(fact) FVALUE_SET((fact), INT_MAX)
/** Returns true if fact value was set */
#define FVALUE_IS_SET(fact) (FVALUE(fact) != INT_MAX)
/** Set value of the fact and push the fact to the priority queue (or
 *  update its value int the queue). */
#define FPUSH(pq, value, fact) \
    do { \
    if (FVALUE_IS_SET(fact)){ \
        planPQUpdate((pq), (value), &(fact)->heap); \
    }else{ \
        planPQPush((pq), (value), &(fact)->heap); \
    } \
    } while (0)

struct _plan_heur_max2_t {
    plan_heur_t heur;
    plan_fact_id_t fact_id;

    fact_t *fact;
    int fact_size;
    int fact_goal; /*!< ID of the artificial goal fact */
    int fact_nopre; /*!< ID of the fact representing empty preconditions */

    op_t *op;
    int op_size;
    int op_goal;
};
typedef struct _plan_heur_max2_t plan_heur_max2_t;

#define HEUR(parent) bor_container_of((parent), plan_heur_max2_t, heur)

static void heurDel(plan_heur_t *_heur);
static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res);

static void opFree(op_t *op);
static void factFree(fact_t *fact);
static void loadOpFact(plan_heur_max2_t *h, const plan_problem_t *p);

plan_heur_t *planHeurMax2New(const plan_problem_t *p, unsigned flags)
{
    plan_heur_max2_t *h;

    h = BOR_ALLOC(plan_heur_max2_t);
    bzero(h, sizeof(*h));
    _planHeurInit(&h->heur, heurDel, heurVal, NULL);
    planFactIdInit(&h->fact_id, p->var, p->var_size, PLAN_FACT_ID_H2);
    loadOpFact(h, p);

    return &h->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_max2_t *h = HEUR(_heur);
    int i;

    _planHeurFree(&h->heur);

    for (i = 0; i < h->fact_size; ++i)
        factFree(h->fact + i);
    if (h->fact)
        BOR_FREE(h->fact);

    for (i = 0; i < h->op_size; ++i)
        opFree(h->op + i);
    if (h->op)
        BOR_FREE(h->op);

    planFactIdFree(&h->fact_id);
    BOR_FREE(h);
}

static void initFacts(plan_heur_max2_t *h)
{
    int i;

    for (i = 0; i < h->fact_size; ++i)
        FVALUE_INIT(h->fact + i);
}

static void initOps(plan_heur_max2_t *h)
{
    int i;

    for (i = 0; i < h->op_size; ++i){
        h->op[i].unsat = h->op[i].pre_size;
        if (h->op[i].pre2_size != NULL){
            memcpy(h->op[i].unsat2, h->op[i].pre2_size,
                   sizeof(int) * h->fact_id.fact1_size);
        }
    }
}

static void addInitState(plan_heur_max2_t *h,
                         const plan_state_t *state,
                         plan_pq_t *pq)
{
    int fact_id;

    PLAN_FACT_ID_FOR_EACH_STATE(&h->fact_id, state, fact_id)
        FPUSH(pq, 0, h->fact + fact_id);
    FPUSH(pq, 0, h->fact + h->fact_nopre);
}

static void enqueueOpExtEffects(plan_heur_max2_t *h, op_t *op,
                                int fact_id, int fact_value, plan_pq_t *pq)
{
    fact_t *fact;
    int value = op->cost + fact_value;
    int fact_id2, next_fact_id, i;

    // Add combinations of fact_id and all unary facts in op's effect
    for (i = 0; i < op->eff.size; ++i){
        fact_id2 = op->eff.arr[i];
        if (fact_id2 >= h->fact_id.fact1_size)
            break;

        next_fact_id = planFactIdFact2(&h->fact_id, fact_id, fact_id2);
        fact = h->fact + next_fact_id;
        if (FVALUE(fact) > value)
            FPUSH(pq, value, fact);
    }
}

static void enqueueOpEffects(plan_heur_max2_t *h,
                             op_t *op, int fact_value,
                             plan_pq_t *pq)
{
    fact_t *fact;
    int value = op->cost + fact_value;
    int i;

    // Add effects if the value is lowered
    for (i = 0; i < op->eff.size; ++i){
        fact = h->fact + op->eff.arr[i];
        if (FVALUE(fact) > value)
            FPUSH(pq, value, fact);
    }

    if (op->unsat2 != NULL){
        // Also check if some extension preconditions were already
        // satisfied
        for (i = 0; i < h->fact_id.fact1_size; ++i){
            if (op->unsat2[i] == 0)
                enqueueOpExtEffects(h, op, i, fact_value, pq);
        }
    }
}

static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    plan_heur_max2_t *h = HEUR(_heur);
    plan_pq_t pq;
    plan_pq_el_t *el;
    fact_t *fact;
    op_t *op;
    int i, fact_id, fact_id2, value;

    planPQInit(&pq);
    initFacts(h);
    initOps(h);

    addInitState(h, state, &pq);
    while (!planPQEmpty(&pq)){
        el = planPQPop(&pq, &value);
        fact = bor_container_of(el, fact_t, heap);
        fact_id = FID(h, fact);

        // Terminate prematurely if we have reached goal
        if (fact_id == h->fact_goal)
            break;

        // Check operators of which the current fact is in their
        // preconditions
        for (i = 0; i < fact->pre_op.size; ++i){
            op = h->op + fact->pre_op.arr[i];
            if (--op->unsat == 0)
                enqueueOpEffects(h, op, value, &pq);
        }

        // Check operators of which the current fact is an extension fact
        for (i = 0; i < fact->pre2_op.size; ++i){
            op = h->op + fact->pre2_op.arr[i];
            fact_id2 = fact->pre2_op_fact.arr[i];
            if (--op->unsat2[fact_id2] == 0 && op->unsat == 0){
                enqueueOpExtEffects(h, op, fact_id2, value, &pq);
            }
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
    if (op->pre2_size != NULL)
        BOR_FREE(op->pre2_size);
    if (op->unsat2 != NULL)
        BOR_FREE(op->unsat2);
}

static void factFree(fact_t *fact)
{
    planArrIntFree(&fact->pre_op);
    planArrIntFree(&fact->pre2_op);
    planArrIntFree(&fact->pre2_op_fact);
}

static void addPrevail(plan_heur_max2_t *h, int op_id, int fact_id)
{
    op_t *op = h->op + op_id;
    int size = op->eff.size;
    int i, fid;

    // A all combinations of prevail and unary facts (those are always at
    // the beggining of the array).
    for (i = 0; i < size && op->eff.arr[i] < h->fact_id.fact1_size; ++i){
        fid = planFactIdFact2(&h->fact_id, op->eff.arr[i], fact_id);
        planArrIntAdd(&op->eff, fid);
    }

    // Sort the effects so the first are unary facts -- we need to do this
    // because we rely on this later.
    // TODO: I think we don't need this
    planArrIntSort(&op->eff);
}

static void addOpExtFact(plan_heur_max2_t *h, const plan_problem_t *p,
                         int op_id, int fact_id)
{
    const plan_part_state_t *pre = p->op[op_id].pre;
    op_t *op = h->op + op_id;
    int i, fact_id2, res_fact_id;

    // Cross reference extension fact and all its combinations with unary
    // facts from the operator
    planArrIntAdd(&h->fact[fact_id].pre2_op, op_id);
    planArrIntAdd(&h->fact[fact_id].pre2_op_fact, fact_id);
    for (i = 0; i < pre->vals_size; ++i){
        fact_id2 = planFactIdVar(&h->fact_id, pre->vals[i].var,
                                              pre->vals[i].val);
        res_fact_id = planFactIdFact2(&h->fact_id, fact_id, fact_id2);
        planArrIntAdd(&h->fact[res_fact_id].pre2_op, op_id);
        planArrIntAdd(&h->fact[res_fact_id].pre2_op_fact, fact_id);
    }
    // Set up number of preconditions that must be fullfilled
    op->pre2_size[fact_id] = pre->vals_size + 1;
}

static void addPrevailAndExtPre(plan_heur_max2_t *h,
                                const plan_problem_t *p, int op_id)
{
    const plan_part_state_t *pre = p->op[op_id].pre;
    const plan_part_state_t *eff = p->op[op_id].eff;
    int prei, effi, var, val, fact_id;

    for (prei = effi = var = 0; var < p->var_size; ++var){
        // Skip variables that set by operator's effect
        if (effi < eff->vals_size && var == eff->vals[effi].var){
            // Skip precondition variables that are changed by an effect
            if (prei < pre->vals_size
                    && pre->vals[prei].var == eff->vals[effi].var){
                ++prei;
            }
            ++effi;
            continue;
        }

        // Preconditions that are not changed by an effect are prevail
        // conditions
        if (prei < pre->vals_size && var == pre->vals[prei].var){
            fact_id = planFactIdVar(&h->fact_id, var, pre->vals[prei].val);
            addPrevail(h, op_id, fact_id);
            ++prei;
            continue;
        }

        // Variables that are not in preconditions or effects can be used
        // for extension of the operator
        for (val = 0; val < p->var[var].range; ++val){
            fact_id = planFactIdVar(&h->fact_id, var, val);
            addOpExtFact(h, p, op_id, fact_id);
        }
    }
}

static void loadOpFact(plan_heur_max2_t *h, const plan_problem_t *p)
{
    op_t *op;
    int i, op_id, fid, size;

    // Allocate facts and add one for empty-precondition fact and one for
    // goal fact
    h->fact_size = h->fact_id.fact_size + 2;
    h->fact = BOR_CALLOC_ARR(fact_t, h->fact_size);
    h->fact_goal = h->fact_size - 2;
    h->fact_nopre = h->fact_size - 1;

    // Allocate operators and add one artificial operator for goal
    h->op_size = p->op_size + 1;
    h->op = BOR_CALLOC_ARR(op_t, h->op_size);
    h->op_goal = h->op_size - 1;

    for (op_id = 0; op_id < p->op_size; ++op_id){
        // TODO: Conditional effects
        op = h->op + op_id;

        op->unsat2 = BOR_ALLOC_ARR(int, h->fact_id.fact1_size);
        op->pre2_size = BOR_ALLOC_ARR(int, h->fact_id.fact1_size);
        for (i = 0; i < h->fact_id.fact1_size; ++i){
            op->pre2_size[i] = -1;
        }

        // Set up effects
        op->eff.arr = planFactIdPartState2(&h->fact_id, p->op[op_id].eff,
                                           &size);
        op->eff.alloc = op->eff.size = size;
        op->cost = p->op[op_id].cost;

        // and preconditions
        op->pre_size = 0;
        PLAN_FACT_ID_FOR_EACH_PART_STATE(&h->fact_id, p->op[op_id].pre, fid){
            planArrIntAdd(&h->fact[fid].pre_op, op_id);
            ++op->pre_size;
        }

        // Record operator with no preconditions
        if (p->op[op_id].pre->vals_size == 0){
            planArrIntAdd(&h->fact[h->fact_nopre].pre_op, op_id);
            op->pre_size = 1;
        }

        // Extend operator with prevail condition and add extension facts
        addPrevailAndExtPre(h, p, op_id);
    }

    // Set up goal operator
    op = h->op + h->op_goal;
    planArrIntAdd(&op->eff, h->fact_goal);
    op->cost = 0;

    op->pre_size = 0;
    PLAN_FACT_ID_FOR_EACH_PART_STATE(&h->fact_id, p->goal, fid){
        planArrIntAdd(&h->fact[fid].pre_op, h->op_goal);
        ++op->pre_size;
    }

    /*
    for (int i = 0; i < h->op_size; ++i){
        fprintf(stderr, "op[%02d]: eff:", i);
        for (int j = 0; j < h->op[i].eff.size; ++j){
            fprintf(stderr, " %d", h->op[i].eff.arr[j]);
        }
        fprintf(stderr, ", pre_size: %d", h->op[i].pre_size);
        if (i == h->op_goal){
            fprintf(stderr, " (*goal*)");
        }else{
            fprintf(stderr, " (%s)", p->op[i].name);
        }
        fprintf(stderr, "\n");
    }

    for (int i = 0; i < h->fact_size; ++i){
        fprintf(stderr, "fact[%02d]: pre_op:", i);
        for (int j = 0; j < h->fact[i].pre_op.size; ++j){
            fprintf(stderr, " %d", h->fact[i].pre_op.arr[j]);
        }
        fprintf(stderr, ", pre2_op:");
        for (int j = 0; j < h->fact[i].pre2_op.size; ++j){
            fprintf(stderr, " %d(%d)",
                    h->fact[i].pre2_op.arr[j],
                    h->fact[i].pre2_op_fact.arr[j]);
        }
        fprintf(stderr, "\n");
    }
    */
}

