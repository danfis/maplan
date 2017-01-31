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

struct _op_base_t {
    plan_arr_int_t eff; /*!< Facts in its effect */
    int cost;
};
typedef struct _op_base_t op_base_t;

struct _op_t {
    int parent;           /*!< ID of the parent operator or -1 */
    plan_arr_int_t child; /*!< List of child operators */
    int pre_size;         /*!< Number of preconditions */
    int ext_fact;         /*!< Extension fact or -1 */

    int unsat; /*!< Number of unsatisfied preconditions */
    int cost;  /*!< Current cost */
    int supp;  /*!< Supporter fact */
    int goal_zone; /*!< True if the operator is connected to a goal zone */
};
typedef struct _op_t op_t;

#define OPID(H, OP) ((int)((OP) - (H)->op))
#define OP_HAS_PARENT(OP) ((OP)->parent >= 0)
#define OP_PARENT(H, OP) ((H)->op + (op)->parent)
#define OPBASE_FROM_OP(H, OP) \
    ((H)->op_base + ((OP)->parent >= 0 ? (OP)->parent : OPID((H), (OP))))

struct _fact_t {
    plan_arr_int_t pre_op; /*!< Operators having this fact as its
                                precondition */
    plan_arr_int_t eff_op; /*!< Operators having this fact as its effect */
    plan_pq_el_t heap;     /*!< Connection to priority heap */
    int supp_cnt;          /*!< Number of operators that have this fact as
                                a supporter. */
};
typedef struct _fact_t fact_t;

/** Returns fact ID from fact object */
#define FID(heur, f) ((int)((f) - (heur)->fact))
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


#define CUT_UNDEF 0
#define CUT_INIT 1
#define CUT_GOAL 2

struct _plan_heur_lm_cut2_t {
    plan_heur_t heur;
    plan_fact_id_t fact_id;

    fact_t *fact;
    int fact_size;
    int fact_goal; /*!< ID of the artificial goal fact */
    int fact_nopre; /*!< ID of the fact representing empty preconditions */

    op_base_t *op_base;
    int op_base_size;
    op_t *op;
    int op_size;
    int op_alloc;
    int op_goal;

    plan_arr_int_t state; /*!< Current state from which heur is computed */
    plan_arr_int_t cut;   /*!< Current cut */

    /** Auxiliary structures to avoid re-allocation */
    int *fact_state;
    plan_arr_int_t queue;
    plan_pq_t pq;
};
typedef struct _plan_heur_lm_cut2_t plan_heur_lm_cut2_t;

#define HEUR(parent) bor_container_of((parent), plan_heur_lm_cut2_t, heur)

static void heurDel(plan_heur_t *_heur);
static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res);

static void opBaseFree(op_base_t *op);
static void opFree(op_t *op);
static void factFree(fact_t *fact);
static void loadOpFact(plan_heur_lm_cut2_t *h, const plan_problem_t *p);

static void debug(plan_heur_lm_cut2_t *h, const char *header);

plan_heur_t *planHeurLMCut2New(const plan_problem_t *p, unsigned flags)
{
    plan_heur_lm_cut2_t *h;

    h = BOR_ALLOC(plan_heur_lm_cut2_t);
    bzero(h, sizeof(*h));
    _planHeurInit(&h->heur, heurDel, heurVal, NULL);
    planFactIdInit(&h->fact_id, p->var, p->var_size, PLAN_FACT_ID_H2);
    loadOpFact(h, p);

    h->fact_state = BOR_CALLOC_ARR(int, h->fact_size);
    planArrIntInit(&h->queue, h->fact_size / 2);
    planPQInit(&h->pq);

    return &h->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_lm_cut2_t *h = HEUR(_heur);
    int i;

    _planHeurFree(&h->heur);

    for (i = 0; i < h->fact_size; ++i)
        factFree(h->fact + i);
    if (h->fact)
        BOR_FREE(h->fact);

    for (i = 0; i < h->op_base_size; ++i)
        opBaseFree(h->op_base + i);
    if (h->op_base)
        BOR_FREE(h->op_base);

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

static void initFacts(plan_heur_lm_cut2_t *h)
{
    int i;

    for (i = 0; i < h->fact_size; ++i){
        FVALUE_INIT(h->fact + i);
        h->fact[i].supp_cnt = 0;
    }
}

static void initOps(plan_heur_lm_cut2_t *h, int init_cost)
{
    op_t *op;
    int i;

    for (i = 0; i < h->op_size; ++i){
        op = h->op + i;
        op->unsat = op->pre_size;
        op->supp = -1;
        if (init_cost)
            op->cost = OPBASE_FROM_OP(h, op)->cost;
        op->goal_zone = 0;
    }
}

static void addInitState(plan_heur_lm_cut2_t *h,
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

static void enqueueOpEffects(plan_heur_lm_cut2_t *h, op_t *op,
                             int enable_fact, int fact_value, plan_pq_t *pq)
{
    op_base_t *op_base = OPBASE_FROM_OP(h, op);
    fact_t *fact;
    op_t *child_op;
    int value = op->cost + fact_value;
    int fact_id, op_id;

    // Set supporter as the enabling fact
    op->supp = enable_fact;
    ++h->fact[enable_fact].supp_cnt;

    // Check all base effects
    PLAN_ARR_INT_FOR_EACH(&op_base->eff, fact_id){
        fact = h->fact + fact_id;
        if (FVALUE(fact) > value)
            FPUSH(pq, value, fact);
    }

    // Check all extension effects if necessary
    if (OP_HAS_PARENT(op)){
        PLAN_ARR_INT_FOR_EACH(&op_base->eff, fact_id){
            if (fact_id >= h->fact_id.fact1_size)
                break;

            fact_id = planFactIdFact2(&h->fact_id, fact_id, op->ext_fact);
            fact = h->fact + fact_id;
            if (FVALUE(fact) > value)
                FPUSH(pq, value, fact);
        }
    }

    // Process all children operators if they have satisfied all
    // preconditions
    PLAN_ARR_INT_FOR_EACH(&op->child, op_id){
        child_op = h->op + op_id;
        if (child_op->unsat == 0)
            enqueueOpEffects(h, child_op, enable_fact, fact_value, pq);
    }
}

static void hMaxFull(plan_heur_lm_cut2_t *h, const plan_state_t *state,
                     int init_cost)
{
    plan_pq_t pq;
    plan_pq_el_t *el;
    fact_t *fact;
    op_t *op;
    int value, op_id;

    planPQInit(&pq);
    initFacts(h);
    initOps(h, init_cost);

    addInitState(h, state, &pq);
    while (!planPQEmpty(&pq)){
        el = planPQPop(&pq, &value);
        fact = bor_container_of(el, fact_t, heap);

        // Check operators of which the current fact is in their
        // preconditions
        PLAN_ARR_INT_FOR_EACH(&fact->pre_op, op_id){
            op = h->op + op_id;
            if (--op->unsat == 0
                    && (!OP_HAS_PARENT(op) || OP_PARENT(h, op)->unsat == 0)){
                enqueueOpEffects(h, op, FID(h, fact), value, &pq);
            }
        }
    }
    planPQFree(&pq);
}

static void markGoalZoneOp(plan_heur_lm_cut2_t *h, op_t *op)
{
    int child_id;

    if (op->supp >= 0 && h->fact_state[op->supp] == CUT_UNDEF){
        if (op->cost == 0){
            planArrIntAdd(&h->queue, op->supp);
            h->fact_state[op->supp] = CUT_GOAL;
        }else{
            op->goal_zone = 1;
        }
    }

    PLAN_ARR_INT_FOR_EACH(&op->child, child_id)
        markGoalZoneOp(h, h->op + child_id);
}

/** Mark facts connected with the goal with zero cost paths */
static void markGoalZone(plan_heur_lm_cut2_t *h)
{
    fact_t *fact;
    int fact_id, op_id;

    h->queue.size = 0;
    planArrIntAdd(&h->queue, h->fact_goal);
    h->fact_state[h->fact_goal] = CUT_GOAL;
    while (h->queue.size > 0){
        fact_id = h->queue.arr[--h->queue.size];
        fact = h->fact + fact_id;

        PLAN_ARR_INT_FOR_EACH(&fact->eff_op, op_id)
            markGoalZoneOp(h, h->op + op_id);
    }
}

static int findCutOpGoal(plan_heur_lm_cut2_t *h, int op_id)
{
    op_t *op = h->op + op_id;

    if (op->goal_zone){
        planArrIntAdd(&h->cut, op_id);
        return op->cost;
    }else{
        return INT_MAX;
    }
}

static void findCutOpExpand(plan_heur_lm_cut2_t *h, int op_id)
{
    op_t *op = h->op + op_id;
    op_base_t *op_base = OPBASE_FROM_OP(h, op);
    int fact_id;

    PLAN_ARR_INT_FOR_EACH(&op_base->eff, fact_id){
        if (h->fact[fact_id].supp_cnt
                && h->fact_state[fact_id] == CUT_UNDEF){
            h->fact_state[fact_id] = CUT_INIT;
            planArrIntAdd(&h->queue, fact_id);
        }
    }

    if (OP_HAS_PARENT(op)){
        PLAN_ARR_INT_FOR_EACH(&op_base->eff, fact_id){
            if (fact_id >= h->fact_id.fact1_size)
                break;

            fact_id = planFactIdFact2(&h->fact_id, fact_id, op->ext_fact);
            if (h->fact[fact_id].supp_cnt
                    && h->fact_state[fact_id] == CUT_UNDEF){
                h->fact_state[fact_id] = CUT_INIT;
                planArrIntAdd(&h->queue, fact_id);
            }
        }
    }
}

static int findCutOp(plan_heur_lm_cut2_t *h, int op_id)
{
    op_t *op = h->op + op_id;
    op_base_t *op_base = OPBASE_FROM_OP(h, op);
    int fact_id;
    int min_cost = INT_MAX;

    PLAN_ARR_INT_FOR_EACH(&op_base->eff, fact_id){
        if (h->fact_state[fact_id] == CUT_UNDEF){
            h->fact_state[fact_id] = CUT_INIT;
            planArrIntAdd(&h->queue, fact_id);

        }else if (h->fact_state[fact_id] == CUT_GOAL){
            planArrIntAdd(&h->cut, op_id);
            min_cost = BOR_MIN(min_cost, op->cost);
            break;
        }
    }

    if (op->parent >= 0){
        PLAN_ARR_INT_FOR_EACH(&op_base->eff, fact_id){
            if (fact_id >= h->fact_id.fact1_size)
                break;

            fact_id = planFactIdFact2(&h->fact_id, fact_id, op->ext_fact);
            if (h->fact_state[fact_id] == CUT_UNDEF){
                h->fact_state[fact_id] = CUT_INIT;
                planArrIntAdd(&h->queue, fact_id);

            }else if (h->fact_state[fact_id] == CUT_GOAL){
                planArrIntAdd(&h->cut, op_id);
                min_cost = BOR_MIN(min_cost, op->cost);
                break;
            }
        }
    }

    return min_cost;
}

/** Finds cut (and fills h->cut) and returns cost of the cut.
 *  Requires marked goal zone. */
static int findCut(plan_heur_lm_cut2_t *h)
{
    op_t *op, *child;
    int fact_id, op_id, child_id;
    int cost, min_cost = INT_MAX;

    h->queue.size = 0;
    PLAN_ARR_INT_FOR_EACH(&h->state, fact_id){
        if (h->fact_state[fact_id] == CUT_GOAL){
            fprintf(stderr, "ERROR: Initial fact in goal-zone but non-zero"
                            "h^max!\n");
            exit(-1);
        }

        if (h->fact[fact_id].supp_cnt){
            planArrIntAdd(&h->queue, fact_id);
            h->fact_state[fact_id] = CUT_INIT;
        }
    }
    if (h->fact[h->fact_nopre].supp_cnt){
        planArrIntAdd(&h->queue, h->fact_nopre);
        h->fact_state[h->fact_nopre] = CUT_INIT;
    }

    h->cut.size = 0;
    while (h->queue.size > 0){
        fact_id = h->queue.arr[--h->queue.size];
        PLAN_ARR_INT_FOR_EACH(&h->fact[fact_id].pre_op, op_id){
            op = h->op + op_id;
            if (op->supp == fact_id){
                //cost = findCutOp(h, op_id);
                //min_cost = BOR_MIN(min_cost, cost);
                if ((cost = findCutOpGoal(h, op_id)) != INT_MAX){
                    min_cost = BOR_MIN(min_cost, cost);
                }else{
                    findCutOpExpand(h, op_id);
                }
            }

            PLAN_ARR_INT_FOR_EACH(&op->child, child_id){
                child = h->op + child_id;
                if (child->supp == fact_id){
                    //cost = findCutOp(h, child_id);
                    //min_cost = BOR_MIN(min_cost, cost);
                    if ((cost = findCutOpGoal(h, child_id)) != INT_MAX){
                        min_cost = BOR_MIN(min_cost, cost);
                    }else{
                        findCutOpExpand(h, op_id);
                    }
                }
            }
        }
    }

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
static void applyCutCost(plan_heur_lm_cut2_t *h, int min_cost)
{
    int op_id;

    PLAN_ARR_INT_FOR_EACH(&h->cut, op_id)
        h->op[op_id].cost -= min_cost;
}

/** Perform cut */
static int cut(plan_heur_lm_cut2_t *h)
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
    plan_heur_lm_cut2_t *h = HEUR(_heur);

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



static void opBaseFree(op_base_t *op)
{
    planArrIntFree(&op->eff);
}

static void opFree(op_t *op)
{
}

static void factFree(fact_t *fact)
{
    planArrIntFree(&fact->pre_op);
    planArrIntFree(&fact->eff_op);
}

static op_t *nextOp(plan_heur_lm_cut2_t *h)
{
    if (h->op_size == h->op_alloc){
        h->op_alloc *= 2;
        h->op = BOR_REALLOC_ARR(h->op, op_t, h->op_alloc);
        bzero(h->op + h->op_size, sizeof(op_t) * (h->op_alloc - h->op_size));
    }

    return h->op + h->op_size++;
}

static void addPrevail(plan_heur_lm_cut2_t *h, int op_id, int fact_id)
{
    op_base_t *op_base = h->op_base + op_id;
    int size = op_base->eff.size;
    int i, fid;

    // Add all combinations of the prevail condition with unary effect
    // facts (those are always at the beggining of the array).
    for (i = 0; i < size && op_base->eff.arr[i] < h->fact_id.fact1_size; ++i){
        fid = planFactIdFact2(&h->fact_id, op_base->eff.arr[i], fact_id);
        planArrIntAdd(&op_base->eff, fid);
    }
}

static void addOpExtFact(plan_heur_lm_cut2_t *h, const plan_problem_t *p,
                         int parent_id, int fact_id)
{
    const plan_part_state_t *pre = p->op[parent_id].pre;
    const plan_part_state_t *eff = p->op[parent_id].eff;
    op_t *op;
    int op_id, i, fact_id2, res_fact_id;

    // Allocate new operator structure
    op = nextOp(h);
    op_id = op - h->op;
    op->parent = parent_id;
    op->ext_fact = fact_id;
    planArrIntAdd(&h->op[parent_id].child, op_id);

    // Cross reference extension fact and all its combinations with unary
    // facts from the operator
    planArrIntAdd(&h->fact[fact_id].pre_op, op_id);
    for (i = 0; i < pre->vals_size; ++i){
        fact_id2 = planFactIdVar(&h->fact_id, pre->vals[i].var,
                                              pre->vals[i].val);
        res_fact_id = planFactIdFact2(&h->fact_id, fact_id, fact_id2);
        planArrIntAdd(&h->fact[res_fact_id].pre_op, op_id);
    }
    // Set up number of preconditions that must be fullfilled
    op->pre_size = pre->vals_size + 1;

    // Cross reference effects
    for (i = 0; i < eff->vals_size; ++i){
        fact_id2 = planFactIdVar(&h->fact_id, eff->vals[i].var,
                                              eff->vals[i].val);
        res_fact_id = planFactIdFact2(&h->fact_id, fact_id, fact_id2);
        planArrIntAdd(&h->fact[res_fact_id].eff_op, op_id);
    }
}

static void addPrevailAndExtPre(plan_heur_lm_cut2_t *h,
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

static void loadOpFact(plan_heur_lm_cut2_t *h, const plan_problem_t *p)
{
    op_base_t *op_base;
    op_t *op;
    int op_id, fid, size;

    // Allocate facts and add one for empty-precondition fact and one for
    // goal fact
    h->fact_size = h->fact_id.fact_size + 2;
    h->fact = BOR_CALLOC_ARR(fact_t, h->fact_size);
    h->fact_goal = h->fact_size - 2;
    h->fact_nopre = h->fact_size - 1;

    // Allocate base operators
    h->op_base_size = p->op_size + 1;
    h->op_base = BOR_CALLOC_ARR(op_base_t, h->op_base_size);
    h->op_size = h->op_alloc = p->op_size + 1;
    h->op = BOR_CALLOC_ARR(op_t, h->op_alloc);
    h->op_goal = h->op_size - 1;

    for (op_id = 0; op_id < p->op_size; ++op_id){
        if (p->op[op_id].cond_eff_size > 0){
            fprintf(stderr, "ERROR: Conditional effects are not"
                            "supported yet!\n");
            exit(-1);
        }

        op_base = h->op_base + op_id;
        op_base->cost = p->op[op_id].cost;

        op = h->op + op_id;
        op->parent = -1;
        op->ext_fact = -1;

        // Set up effects
        op_base->eff.arr = planFactIdPartState2(&h->fact_id, p->op[op_id].eff,
                                                &size);
        op_base->eff.alloc = op_base->eff.size = size;

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

        PLAN_ARR_INT_FOR_EACH(&op_base->eff, fid)
            planArrIntAdd(&h->fact[fid].eff_op, op_id);
    }

    // Set up goal operator
    op_base = h->op_base + h->op_goal;
    op = h->op + h->op_goal;
    op->parent = -1;
    op->ext_fact = -1;
    planArrIntAdd(&op_base->eff, h->fact_goal);
    op->cost = 0;

    op->pre_size = 0;
    PLAN_FACT_ID_FOR_EACH_PART_STATE(&h->fact_id, p->goal, fid){
        planArrIntAdd(&h->fact[fid].pre_op, h->op_goal);
        ++op->pre_size;
    }
    planArrIntAdd(&h->fact[h->fact_goal].eff_op, h->op_goal);

    // Free unneeded memory
    h->op_alloc = h->op_size;
    h->op = BOR_REALLOC_ARR(h->op, op_t, h->op_alloc);
}


static void debug(plan_heur_lm_cut2_t *h, const char *header)
{
    int i, j, f1, f2;

    if (header)
        fprintf(stderr, ":: %s ::\n", header);

    for (i = 0; i < h->fact_size; ++i){
        fprintf(stderr, "Fact[%d]:", i);
        if (i == h->fact_goal){
            fprintf(stderr, "*goal*");
        }else if (i == h->fact_nopre){
            fprintf(stderr, "*nopre*");
        }else if (i >= h->fact_id.fact1_size){
            planFactIdFromFactId(&h->fact_id, i, &f1, &f2);
            fprintf(stderr, " (%d:%d)", f1, f2);
        }

        fprintf(stderr, " value: %d", FVALUE(h->fact + i));
        fprintf(stderr, " pre_op:");
        PLAN_ARR_INT_FOR_EACH(&h->fact[i].pre_op, j)
            fprintf(stderr, " %d", j);
        fprintf(stderr, ", eff_op:");
        PLAN_ARR_INT_FOR_EACH(&h->fact[i].eff_op, j)
            fprintf(stderr, " %d", j);
        fprintf(stderr, "\n");
    }

    for (i = 0; i < h->op_size; ++i){
        fprintf(stderr, "Op[%d]:", i);
        if (i == h->op_goal)
            fprintf(stderr, "*goal*");
        if (h->op[i].parent >= 0){
            fprintf(stderr, " parent: %d", h->op[i].parent);
        }else{
            fprintf(stderr, " child:");
            PLAN_ARR_INT_FOR_EACH(&h->op[i].child, j)
                fprintf(stderr, " %d", j);
        }
        fprintf(stderr, ", pre_size: %d", h->op[i].pre_size);
        fprintf(stderr, ", ext_fact: %d", h->op[i].ext_fact);

        if (i < h->op_base_size){
            fprintf(stderr, ", cost: %d", h->op_base[i].cost);
            fprintf(stderr, ", eff:");
            PLAN_ARR_INT_FOR_EACH(&h->op_base[i].eff, j)
                fprintf(stderr, " %d", j);
        }

        fprintf(stderr, " |");
        fprintf(stderr, " unsat: %d", h->op[i].unsat);
        fprintf(stderr, " cost: %d", h->op[i].cost);
        fprintf(stderr, " supp: %d", h->op[i].supp);
        fprintf(stderr, "\n");
    }

    fprintf(stderr, "State:");
    PLAN_ARR_INT_FOR_EACH(&h->state, i)
        fprintf(stderr, " %d", i);
    fprintf(stderr, "\n");

    fprintf(stderr, "Goal Zone:");
    for (i = 0; i < h->fact_size; ++i){
        if (h->fact_state[i] == CUT_GOAL)
            fprintf(stderr, " %d", i);
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "Init Zone:");
    for (i = 0; i < h->fact_size; ++i){
        if (h->fact_state[i] == CUT_INIT)
            fprintf(stderr, " %d", i);
    }
    fprintf(stderr, "\n");

    planArrIntSort(&h->cut);
    fprintf(stderr, "Cut size: %d\n", (int)h->cut.size);
    fprintf(stderr, "Cut:");
    PLAN_ARR_INT_FOR_EACH(&h->cut, i)
        fprintf(stderr, " %d(%d)", i, h->op[i].cost);
    fprintf(stderr, "\n");
}
