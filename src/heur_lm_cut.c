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
#include "plan/search.h"
#include "plan/arr.h"
#include "plan/pq.h"
#include "plan/fact_id.h"
#include "plan/heur.h"
#include "op_id_tr.h"

struct _op_t {
    int op_id;
    plan_arr_int_t eff; /*!< Facts in its effect */
    plan_arr_int_t pre; /*!< Facts in its effect */
    int op_cost;        /*!< Original operator's cost */

    int cost;           /*!< Current cost of the operator */
    int unsat;          /*!< Number of unsatisfied preconditions */
    int supp;           /*!< Supporter fact (that maximizes h^max) */
    int supp_cost;      /*!< Cost of the supported -- needed for hMaxInc() */
    int cut_candidate; /*!< True if the operator is candidate for a cut */
};
typedef struct _op_t op_t;

struct _fact_t {
    plan_arr_int_t pre_op; /*!< Operators having this fact as its precond */
    plan_arr_int_t eff_op; /*!< Operators having this fact as its effect */
    int value;
    plan_pq_el_t heap;     /*!< Connection to priority heap */
    int supp_cnt;          /*!< Number of operators that have this fact as
                                a supporter. */
};
typedef struct _fact_t fact_t;

#define FID(heur, f) ((f) - (heur)->fact)
#define FVALUE(fact) ((fact)->value)
#define FVALUE_INIT(fact) \
    do { (fact)->value = (fact)->heap.key = INT_MAX; } while(0)
#define FVALUE_IS_SET(fact) (FVALUE(fact) != INT_MAX)
#define FPUSH(pq, val, fact) \
    do { \
    ASSERT(val != INT_MAX); \
    if ((fact)->heap.key != INT_MAX){ \
        (fact)->value = (val); \
        planPQUpdate((pq), (val), &(fact)->heap); \
    }else{ \
        (fact)->value = (val); \
        planPQPush((pq), (val), &(fact)->heap); \
    } \
    } while (0)

_bor_inline fact_t *FPOP(plan_pq_t *pq, int *value)
{
    plan_pq_el_t *el = planPQPop(pq, value);
    fact_t *fact = bor_container_of(el, fact_t, heap);
    fact->heap.key = INT_MAX;
    return fact;
}

#define SET_OP_SUPP(h, op, fact_id) \
    do { \
        if ((op)->supp != -1) \
            F_UNSET_SUPP((h)->fact + (op)->supp); \
        (op)->supp = (fact_id); \
        (op)->supp_cost = (h)->fact[(fact_id)].value; \
        F_SET_SUPP((h)->fact + (fact_id)); \
    } while (0)

#define F_INIT_SUPP(fact) ((fact)->supp_cnt = 0)
#define F_SET_SUPP(fact) (++(fact)->supp_cnt)
#define F_UNSET_SUPP(fact) (--(fact)->supp_cnt)
#define F_IS_SUPP(fact) ((fact)->supp_cnt)

/** Incremental LM-Cut, the local version. */
#define INC_LOCAL 1

/** Incremental LM-Cut, the version with cached landmarks. */
#define INC_CACHE 2

struct _inc_local_t {
    int enabled;
    plan_landmark_set_t ldms; /*!< Cached landmarks */
    plan_state_id_t ldms_id;  /*!< ID of the state for which the landmarks
                                   are cached */
    plan_op_id_tr_t op_id_tr; /*!< Translation from global ID to local ID */
};
typedef struct _inc_local_t inc_local_t;

struct _inc_cache_t {
    int enabled;
    plan_landmark_cache_t *ldm_cache;
    plan_op_id_tr_t op_id_tr;
};
typedef struct _inc_cache_t inc_cache_t;

struct _plan_heur_lm_cut_t {
    plan_heur_t heur;
    unsigned flags;
    plan_fact_id_t fact_id;

    fact_t *fact;
    int fact_size;
    int fact_goal;
    int fact_nopre;

    op_t *op;
    int op_alloc;
    int op_size;
    int op_goal;

    plan_arr_int_t state; /*!< Current state from which heur is computed */
    plan_arr_int_t cut;   /*!< Current cut */

    inc_local_t inc_local; /*!< Struct for local incremental LM-Cut */
    inc_cache_t inc_cache; /*!< Struct for cached incremental LM-Cut */

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
static void heurValIncLocal(plan_heur_t *_heur,
                            plan_state_id_t state_id,
                            plan_search_t *search,
                            plan_heur_res_t *res);
static void heurValIncCache(plan_heur_t *_heur,
                            plan_state_id_t state_id,
                            plan_search_t *search,
                            plan_heur_res_t *res);
static void planHeurLMCutStateInc(plan_heur_t *_heur,
                                  const plan_state_t *state,
                                  plan_heur_res_t *res);

static void opFree(op_t *op);
static void factFree(fact_t *fact);
static void loadOpFact(plan_heur_lm_cut_t *h, const plan_problem_t *p);

#if 0
static void debug(plan_heur_lm_cut_t *h)
{
    for (int i = 0; i < h->fact_size; ++i){
        fprintf(stderr, "F[%03d]: value: %d, supp_cnt: %d",
                i, FVALUE(h->fact + i), h->fact[i].supp_cnt);
        fprintf(stderr, ", pre:");
        for (int j = 0; j < h->fact[i].pre_op.size; ++j){
            fprintf(stderr, " %d", h->fact[i].pre_op.arr[j]);
        }
        fprintf(stderr, ", eff:");
        for (int j = 0; j < h->fact[i].eff_op.size; ++j){
            fprintf(stderr, " %d", h->fact[i].eff_op.arr[j]);
        }

        for (int j = 0; j < h->state.size; ++j){
            if (i == h->state.arr[j]){
                fprintf(stderr, " *");
                break;
            }
        }
        if (h->fact_goal == i)
            fprintf(stderr, " +G");
        if (h->fact_nopre == i)
            fprintf(stderr, " +NP");
        fprintf(stderr, "\n");
    }

    for (int i = 0; i < h->op_size; ++i){
        fprintf(stderr, "O[%03d]: cost: %d, unsat: %d, supp: %d,"
                        " supp_cost: %d, cut_candidate: %d",
                i, h->op[i].cost, h->op[i].unsat, h->op[i].supp,
                h->op[i].supp_cost, h->op[i].cut_candidate);
        fprintf(stderr, ", pre:");
        for (int j = 0; j < h->op[i].pre.size; ++j){
            fprintf(stderr, " %d", h->op[i].pre.arr[j]);
        }
        fprintf(stderr, ", eff:");
        for (int j = 0; j < h->op[i].eff.size; ++j){
            fprintf(stderr, " %d", h->op[i].eff.arr[j]);
        }
        fprintf(stderr, "\n");
    }
}
#endif

plan_heur_t *lmCutNew(const plan_problem_t *p, unsigned type,
                      unsigned flags, unsigned cache_flags)
{
    plan_heur_lm_cut_t *h;

    h = BOR_ALLOC(plan_heur_lm_cut_t);
    bzero(h, sizeof(*h));
    h->flags = flags;
    if (type == INC_LOCAL){
        _planHeurInit(&h->heur, heurDel, planHeurLMCutStateInc,
                      heurValIncLocal);
    }else if (type == INC_CACHE){
        _planHeurInit(&h->heur, heurDel, planHeurLMCutStateInc,
                      heurValIncCache);
    }else{
        _planHeurInit(&h->heur, heurDel, heurVal, NULL);
    }
    planFactIdInit(&h->fact_id, p->var, p->var_size, 0);
    loadOpFact(h, p);

    if (type == INC_LOCAL){
        h->inc_local.enabled = 1;
        bzero(&h->inc_local.ldms, sizeof(h->inc_local.ldms));
        h->inc_local.ldms_id = PLAN_NO_STATE;
        planOpIdTrInit(&h->inc_local.op_id_tr, p->op, p->op_size);
    }else if (type == INC_CACHE){
        h->inc_cache.enabled = 1;
        h->inc_cache.ldm_cache = planLandmarkCacheNew(cache_flags);
        planOpIdTrInit(&h->inc_cache.op_id_tr, p->op, p->op_size);
    }

    h->fact_state = BOR_ALLOC_ARR(int, h->fact_size);
    planArrIntInit(&h->queue, h->fact_size / 2);
    planPQInit(&h->pq);

    return &h->heur;
}

plan_heur_t *planHeurLMCutNew(const plan_problem_t *p, unsigned flags)
{
    return lmCutNew(p, 0, flags, 0);
}

plan_heur_t *planHeurLMCutIncLocalNew(const plan_problem_t *p, unsigned flags)
{
    return lmCutNew(p, INC_LOCAL, flags, 0);
}

plan_heur_t *planHeurLMCutIncCacheNew(const plan_problem_t *p,
                                      unsigned flags, unsigned cache_flags)
{
    return lmCutNew(p, INC_CACHE, flags, cache_flags);
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

    for (i = 0; i < h->op_size; ++i)
        opFree(h->op + i);
    if (h->op)
        BOR_FREE(h->op);

    if (h->inc_local.enabled){
        planLandmarkSetFree(&h->inc_local.ldms);
        planOpIdTrFree(&h->inc_local.op_id_tr);
    }
    if (h->inc_cache.enabled){
        planLandmarkCacheDel(h->inc_cache.ldm_cache);
        planOpIdTrFree(&h->inc_cache.op_id_tr);
    }

    if (h->fact_state)
        BOR_FREE(h->fact_state);
    planArrIntFree(&h->queue);
    planPQFree(&h->pq);
    planArrIntFree(&h->cut);
    planArrIntFree(&h->state);

    planFactIdFree(&h->fact_id);
    BOR_FREE(h);
}

static void initFacts(plan_heur_lm_cut_t *h)
{
    int i;

    for (i = 0; i < h->fact_size; ++i){
        FVALUE_INIT(h->fact + i);
        F_INIT_SUPP(h->fact + i);
    }
}

static void initOps(plan_heur_lm_cut_t *h, int init_cost)
{
    int i;

    for (i = 0; i < h->op_size; ++i){
        h->op[i].unsat = h->op[i].pre.size;
        h->op[i].supp = -1;
        h->op[i].supp_cost = INT_MAX;
        if (init_cost)
            h->op[i].cost = h->op[i].op_cost;
        h->op[i].cut_candidate = 0;
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
    fact_t *fact;
    op_t *op;
    int i, value;

    planPQInit(&pq);
    initFacts(h);
    initOps(h, init_cost);
    addInitState(h, state, &pq);
    while (!planPQEmpty(&pq)){
        fact = FPOP(&pq, &value);
        ASSERT(FVALUE(fact) == value);

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


static void updateSupp(plan_heur_lm_cut_t *h, op_t *op)
{
    fact_t *fact;
    int fact_id, supp = -1, value = -1;

    PLAN_ARR_INT_FOR_EACH(&op->pre, fact_id){
        fact = h->fact + fact_id;
        if (FVALUE_IS_SET(fact) && FVALUE(fact) > value){
            value = FVALUE(fact);
            supp = fact_id;
        }
    }

    ASSERT(supp != -1);
    SET_OP_SUPP(h, op, supp);
}

static void enqueueOpEffectsInc(plan_heur_lm_cut_t *h, op_t *op,
                                int fact_value, plan_pq_t *pq)
{
    fact_t *fact;
    int value = op->cost + fact_value;
    int fact_id;

    // Check all base effects
    PLAN_ARR_INT_FOR_EACH(&op->eff, fact_id){
        fact = h->fact + fact_id;
        if (FVALUE(fact) > value)
            FPUSH(pq, value, fact);
    }
}

static void hMaxIncUpdateOp(plan_heur_lm_cut_t *h, op_t *op,
                            int fact_id, int fact_value)
{
    int old_supp_value;

    if (op->supp != fact_id || op->unsat > 0)
        return;

    old_supp_value = op->supp_cost;
    if (old_supp_value <= fact_value)
        return;

    updateSupp(h, op);
    if (op->supp_cost != old_supp_value){
        ASSERT(op->supp_cost < old_supp_value);
        enqueueOpEffectsInc(h, op, op->supp_cost, &h->pq);
    }
}

static void hMaxInc(plan_heur_lm_cut_t *h, const plan_arr_int_t *cut)
{
    fact_t *fact;
    op_t *op;
    int op_id, fact_id, fact_value;

    for (op_id = 0; op_id < h->op_size; ++op_id)
        h->op[op_id].cut_candidate = 0;

    PLAN_ARR_INT_FOR_EACH(cut, op_id){
        op = h->op + op_id;
        enqueueOpEffectsInc(h, op, op->supp_cost, &h->pq);
    }

    while (!planPQEmpty(&h->pq)){
        fact = FPOP(&h->pq, &fact_value);
        fact_id = FID(h, fact);

        PLAN_ARR_INT_FOR_EACH(&fact->pre_op, op_id){
            op = h->op + op_id;
            hMaxIncUpdateOp(h, op, fact_id, fact_value);
        }
    }
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
            if (op->supp >= 0 && h->fact_state[op->supp] == CUT_UNDEF){
                if (op->cost == 0){
                    planArrIntAdd(&h->queue, op->supp);
                    h->fact_state[op->supp] = CUT_GOAL;
                }else{
                    op->cut_candidate = 1;
                }
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
            if (op->cut_candidate){
                planArrIntAdd(&h->cut, op_id);
                min_cost = BOR_MIN(min_cost, op->cost);
                continue;
            }

            PLAN_ARR_INT_FOR_EACH(&op->eff, next){
                if (h->fact_state[next] == CUT_UNDEF){
                    if (F_IS_SUPP(h->fact + next)){
                        h->fact_state[next] = CUT_INIT;
                        planArrIntAdd(&h->queue, next);
                    }
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

static void _saveLandmark(plan_heur_lm_cut_t *h,
                          const int *ldm, int ldm_size,
                          plan_heur_res_t *res)
{
    plan_arr_int_t ops;
    int i;

    if (ldm_size == 0)
        return;

    planArrIntInit(&ops, ldm_size);
    for (i = 0; i < ldm_size; ++i)
        planArrIntAdd(&ops, h->op[ldm[i]].op_id);
    planArrIntSort(&ops);
    planArrIntUniq(&ops);
    planLandmarkSetAdd(&res->landmarks, ops.size, ops.arr);
    planArrIntFree(&ops);
}

static void saveLandmark(plan_heur_lm_cut_t *h, plan_heur_res_t *res)
{
    _saveLandmark(h, h->cut.arr, h->cut.size, res);
}

static int landmarkCost(const plan_heur_lm_cut_t *h,
                        const int *ldm, int ldm_size,
                        int used_op_id)
{
    plan_cost_t cost = PLAN_COST_MAX;
    int i, op_id;

    for (i = 0; i < ldm_size; ++i){
        op_id = ldm[i];
        // If this landmark contains the operator that was used during
        // creation of a new state, discard this landmark (i.e., return
        // zero cost).
        if (op_id == used_op_id)
            return 0;
        cost = BOR_MIN(cost, h->op[op_id].cost);
    }

    return cost;
}

static void applyInitLandmarks(plan_heur_lm_cut_t *h,
                               const plan_landmark_set_t *ldms,
                               int inc_op_id,
                               plan_heur_res_t *res)
{
    plan_arr_int_t ldm_ops;
    const plan_landmark_t *ldm;
    int cost, i, j, op_id;

    planArrIntInit(&ldm_ops, 8);

    // Record operators that should be changed as well as value that should
    // be substracted from their cost.
    for (i = 0; i < ldms->size; ++i){
        ldm = ldms->landmark + i;
        if (ldm->size <= 0)
            continue;

        // Determine cost of the landmark and skip zero-cost landmarks
        cost = landmarkCost(h, ldm->op_id, ldm->size, inc_op_id);
        if (cost <= 0)
            continue;

        // Update initial heuristic value
        res->heur += cost;

        // Mark each operator as changed and update it by substracting the
        // cost of the landmark.
        for (j = 0; j < ldm->size; ++j){
            op_id = ldm->op_id[j];
            planArrIntAdd(&ldm_ops, op_id);
            h->op[op_id].cost -= cost;
        }

        if (res->save_landmarks)
            _saveLandmark(h, ldm->op_id, ldm->size, res);
    }

    planArrIntSort(&ldm_ops);
    planArrIntUniq(&ldm_ops);

    // Update h^max
    hMaxInc(h, &ldm_ops);

    planArrIntFree(&ldm_ops);
}

static void _heurVal(plan_heur_lm_cut_t *h, const plan_state_t *state,
                     const plan_landmark_set_t *ldms, int inc_op_id,
                     plan_heur_res_t *res)
{
    res->heur = 0;

    hMaxFull(h, state, 1);
    if (!FVALUE_IS_SET(h->fact + h->fact_goal)){
        res->heur = PLAN_HEUR_DEAD_END;
        return;
    }

    // If landmarks are given, apply them before continuing with LM-Cut and
    // set up initial heuristic value accordingly.
    if (ldms != NULL && ldms->size > 0)
        applyInitLandmarks(h, ldms, inc_op_id, res);

    while (FVALUE(h->fact + h->fact_goal) > 0){
        res->heur += cut(h);

        // Store landmarks into output structure if requested.
        if (res->save_landmarks)
            saveLandmark(h, res);

        //hMaxFull(h, state, 0);
        hMaxInc(h, &h->cut);
    }
}

static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    plan_heur_lm_cut_t *h = HEUR(_heur);
    _heurVal(h, state, NULL, -1, res);
}

static void lmCutIncLocalParent(plan_heur_lm_cut_t *heur,
                                plan_search_t *search,
                                plan_state_id_t parent_state_id)
{
    const plan_state_t *state;
    plan_heur_res_t res;

    if (heur->inc_local.ldms_id == parent_state_id)
        return;

    // Run LM-Cut on parent state to obtain initial landmarks
    state = planSearchLoadState(search, parent_state_id);
    planHeurResInit(&res);
    res.save_landmarks = 1;
    _heurVal(heur, state, NULL, -1, &res);

    // Save landmarks into cache
    planLandmarkSetFree(&heur->inc_local.ldms);
    heur->inc_local.ldms = res.landmarks;
    heur->inc_local.ldms_id = parent_state_id;
}

static void heurValIncLocal(plan_heur_t *_heur,
                            plan_state_id_t state_id,
                            plan_search_t *search,
                            plan_heur_res_t *res)
{
    plan_heur_lm_cut_t *heur = HEUR(_heur);
    const plan_state_t *state;
    plan_state_space_node_t *node;
    int op_id = -1;

    // Obtain initial landmarks from the parent state
    node = planSearchLoadNode(search, state_id);
    if (node->parent_state_id >= 0){
        lmCutIncLocalParent(heur, search, node->parent_state_id);
    }else{
        planLandmarkSetFree(&heur->inc_local.ldms);
        bzero(&heur->inc_local.ldms, sizeof(heur->inc_local.ldms));
        heur->inc_local.ldms_id = -1;
    }

    // Compute heuristic for the current state
    state = planSearchLoadState(search, state_id);
    if (node->op != NULL)
        op_id = planOpIdTrLoc(&heur->inc_local.op_id_tr, node->op->global_id);
    _heurVal(heur, state, &heur->inc_local.ldms, op_id, res);
}

static void heurValIncCache(plan_heur_t *_heur,
                            plan_state_id_t state_id,
                            plan_search_t *search,
                            plan_heur_res_t *res_out)
{
    plan_heur_lm_cut_t *heur = HEUR(_heur);
    const plan_landmark_set_t *ldms = NULL;
    const plan_state_t *state;
    plan_state_space_node_t *node;
    plan_heur_res_t res;
    int op_id = -1, ret;

    if (res_out->save_landmarks){
        fprintf(stderr, "Warning: LM-Cut-inc-cache: Saving landmarks is not"
                        " currently supported by this version of LM-Cut.\n");
    }

    // Obtain initial landmarks from the parent state
    node = planSearchLoadNode(search, state_id);
    if (node->parent_state_id >= 0){
        ldms = planLandmarkCacheGet(heur->inc_cache.ldm_cache,
                                    node->parent_state_id);
    }

    // Compute heuristic for the current state
    state = planSearchLoadState(search, state_id);
    if (node->op != NULL)
        op_id = planOpIdTrLoc(&heur->inc_cache.op_id_tr, node->op->global_id);

    res = *res_out;
    res.save_landmarks = 1;
    _heurVal(heur, state, ldms, op_id, &res);
    //lmCutState(heur, state, NULL, -1, &res);
    ret = planLandmarkCacheAdd(heur->inc_cache.ldm_cache, state_id,
                               &res.landmarks);
    if (ret != 0)
        planLandmarkSetFree(&res.landmarks);

    //lmCutState(heur, state, ldms, op_id, res_out);
    *res_out = res;
    res_out->save_landmarks = 0;
    bzero(&res_out->landmarks, sizeof(res_out->landmarks));
}

static void opFree(op_t *op)
{
    planArrIntFree(&op->eff);
    planArrIntFree(&op->pre);
}

static void factFree(fact_t *fact)
{
    planArrIntFree(&fact->pre_op);
    planArrIntFree(&fact->eff_op);
}

static op_t *nextOp(plan_heur_lm_cut_t *h)
{
    if (h->op_size == h->op_alloc){
        h->op_alloc *= 2;
        h->op = BOR_REALLOC_ARR(h->op, op_t, h->op_alloc);
        bzero(h->op + h->op_size, sizeof(op_t) * (h->op_alloc - h->op_size));
    }

    return h->op + h->op_size++;
}

static int getCost(const plan_heur_lm_cut_t *h, const plan_op_t *op)
{
    int cost;

    cost = op->cost;
    if (h->flags & PLAN_HEUR_OP_UNIT_COST)
        cost = 1;
    if (h->flags & PLAN_HEUR_OP_COST_PLUS_ONE)
        cost = cost + 1;
    return cost;
}

static void addOp(plan_heur_lm_cut_t *h, const plan_op_t *pop,
                  int parent_op_id,
                  const plan_op_cond_eff_t *cond_eff)
{
    op_t *op;
    int op_id, fid;

    if (cond_eff != NULL){
        op = nextOp(h);
    }else{
        op = h->op + parent_op_id;
    }
    op_id = op - h->op;

    op->op_id = parent_op_id;
    op->op_cost = getCost(h, pop);

    // Set effects
    PLAN_FACT_ID_FOR_EACH_PART_STATE(&h->fact_id, pop->eff, fid){
        planArrIntAdd(&op->eff, fid);
        planArrIntAdd(&h->fact[fid].eff_op, op_id);
    }
    if (cond_eff){
        PLAN_FACT_ID_FOR_EACH_PART_STATE(&h->fact_id, cond_eff->eff, fid){
            planArrIntAdd(&op->eff, fid);
            planArrIntAdd(&h->fact[fid].eff_op, op_id);
        }
    }

    // Set preconditions
    PLAN_FACT_ID_FOR_EACH_PART_STATE(&h->fact_id, pop->pre, fid){
        planArrIntAdd(&h->fact[fid].pre_op, op_id);
        planArrIntAdd(&op->pre, fid);
    }
    if (cond_eff){
        PLAN_FACT_ID_FOR_EACH_PART_STATE(&h->fact_id, cond_eff->pre, fid){
            planArrIntAdd(&h->fact[fid].pre_op, op_id);
            planArrIntAdd(&op->pre, fid);
        }
    }

    // Record operator with no preconditions
    if (op->pre.size == 0){
        planArrIntAdd(&h->fact[h->fact_nopre].pre_op, op_id);
        planArrIntAdd(&h->op[op_id].pre, h->fact_nopre);
    }

    planArrIntSort(&op->eff);
}

static void loadOpFact(plan_heur_lm_cut_t *h, const plan_problem_t *p)
{
    op_t *op;
    int i, op_id, fid;

    // Allocate facts and add one for empty-precondition fact and one for
    // goal fact
    h->fact_size = h->fact_id.fact_size + 2;
    h->fact = BOR_CALLOC_ARR(fact_t, h->fact_size);
    h->fact_goal = h->fact_size - 2;
    h->fact_nopre = h->fact_size - 1;

    // Allocate operators and add one artificial for goal
    h->op_alloc = h->op_size = p->op_size + 1;
    h->op = BOR_CALLOC_ARR(op_t, h->op_size);
    h->op_goal = h->op_size - 1;

    for (op_id = 0; op_id < p->op_size; ++op_id){
        addOp(h, p->op + op_id, op_id, NULL);
        for (i = 0; i < p->op[op_id].cond_eff_size; ++i)
            addOp(h, p->op + op_id, op_id, p->op[op_id].cond_eff + i);
    }

    // Set up goal operator
    op = h->op + h->op_goal;
    planArrIntAdd(&op->eff, h->fact_goal);
    op->op_cost = 0;

    PLAN_FACT_ID_FOR_EACH_PART_STATE(&h->fact_id, p->goal, fid){
        planArrIntAdd(&h->fact[fid].pre_op, h->op_goal);
        planArrIntAdd(&op->pre, fid);
    }
    planArrIntAdd(&h->fact[h->fact_goal].eff_op, h->op_goal);
}


static void planHeurLMCutStateInc(plan_heur_t *_heur,
                                  const plan_state_t *state,
                                  plan_heur_res_t *res)
{
    fprintf(stderr, "Error: Incremental LM-Cut cannot be called via"
                    " planHeurState() function!\n");
    exit(-1);
}
