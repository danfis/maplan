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
#include <boruvka/lifo.h>
#include "plan/heur.h"
#include "plan/search.h"

#include "heur_relax.h"
#include "op_id_tr.h"

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
    plan_heur_relax_t relax;

    int *fact_goal_zone; /*!< Flag for each fact whether it is in goal zone */
    int *fact_in_queue;
    plan_arr_int_t cut;    /*!< Array of cut operators */

    inc_local_t inc_local; /*!< Struct for local incremental LM-Cut */
    inc_cache_t inc_cache; /*!< Struct for cached incremental LM-Cut */
};
typedef struct _plan_heur_lm_cut_t plan_heur_lm_cut_t;

#define HEUR(parent) \
    bor_container_of((parent), plan_heur_lm_cut_t, heur)

/** Delete method */
static void planHeurLMCutDel(plan_heur_t *_heur);
/** Main function that returns heuristic value. */
static void planHeurLMCutState(plan_heur_t *_heur, const plan_state_t *state,
                               plan_heur_res_t *res);
static void planHeurLMCutNodeIncLocal(plan_heur_t *_heur,
                                      plan_state_id_t state_id,
                                      plan_search_t *search,
                                      plan_heur_res_t *res);
static void planHeurLMCutNodeIncCache(plan_heur_t *_heur,
                                      plan_state_id_t state_id,
                                      plan_search_t *search,
                                      plan_heur_res_t *res);
static void planHeurLMCutStateInc(plan_heur_t *_heur,
                                  const plan_state_t *state,
                                  plan_heur_res_t *res);
void planHeurLMCutDebug(const plan_heur_lm_cut_t *h);

static plan_heur_t *lmCutNew(const plan_problem_t *p,
                             int inc, unsigned flags, unsigned cache_flags)
{
    plan_heur_lm_cut_t *heur;

    heur = BOR_ALLOC(plan_heur_lm_cut_t);
    bzero(heur, sizeof(*heur));
    if (inc == 1){
        _planHeurInit(&heur->heur, planHeurLMCutDel,
                      planHeurLMCutStateInc,
                      planHeurLMCutNodeIncLocal);
    }else if (inc == 2){
        _planHeurInit(&heur->heur, planHeurLMCutDel,
                      planHeurLMCutStateInc,
                      planHeurLMCutNodeIncCache);
    }else{
        _planHeurInit(&heur->heur, planHeurLMCutDel, planHeurLMCutState, NULL);
    }
    planHeurRelaxInit(&heur->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      p->var, p->var_size, p->goal, p->op, p->op_size, flags);

    heur->fact_goal_zone = BOR_ALLOC_ARR(int, heur->relax.cref.fact_size);
    heur->fact_in_queue  = BOR_ALLOC_ARR(int, heur->relax.cref.fact_size);
    planArrIntInit(&heur->cut, 2);

    heur->inc_local.enabled = 0;
    heur->inc_cache.enabled = 0;
    if (inc == 1){
        heur->inc_local.enabled = 1;
        bzero(&heur->inc_local.ldms, sizeof(heur->inc_local.ldms));
        heur->inc_local.ldms_id = PLAN_NO_STATE;
        planOpIdTrInit(&heur->inc_local.op_id_tr, p->op, p->op_size);
    }else if (inc == 2){
        heur->inc_cache.enabled = 1;
        heur->inc_cache.ldm_cache = planLandmarkCacheNew(cache_flags);
        planOpIdTrInit(&heur->inc_cache.op_id_tr, p->op, p->op_size);
    }

    return &heur->heur;
}

plan_heur_t *planHeurRelaxLMCutNew(const plan_problem_t *p, unsigned flags)
{
    return lmCutNew(p, 0, flags, 0);
}

plan_heur_t *planHeurRelaxLMCutIncLocalNew(const plan_problem_t *p,
                                           unsigned flags)
{
    return lmCutNew(p, 1, flags, 0);
}

plan_heur_t *planHeurRelaxLMCutIncCacheNew(const plan_problem_t *p,
                                           unsigned flags,
                                           unsigned cache_flags)
{
    return lmCutNew(p, 2, flags, cache_flags);
}

plan_heur_t *planHeurH2LMCutNew(const plan_problem_t *p, unsigned flags)
{
    flags |= PLAN_HEUR_H2;
    return lmCutNew(p, 0, flags, 0);
}

static void planHeurLMCutDel(plan_heur_t *_heur)
{
    plan_heur_lm_cut_t *heur = HEUR(_heur);

    planArrIntFree(&heur->cut);
    BOR_FREE(heur->fact_goal_zone);
    BOR_FREE(heur->fact_in_queue);
    if (heur->inc_local.enabled){
        planLandmarkSetFree(&heur->inc_local.ldms);
        planOpIdTrFree(&heur->inc_local.op_id_tr);
    }
    if (heur->inc_cache.enabled){
        planLandmarkCacheDel(heur->inc_cache.ldm_cache);
        planOpIdTrFree(&heur->inc_cache.op_id_tr);
    }
    planHeurRelaxFree(&heur->relax);
    _planHeurFree(&heur->heur);
    BOR_FREE(heur);
}



static void markGoalZoneRecursive(plan_heur_lm_cut_t *heur, int fact_id)
{
    int op_id;
    plan_heur_relax_op_t *op;

    if (heur->fact_goal_zone[fact_id])
        return;

    heur->fact_goal_zone[fact_id] = 1;

    PLAN_ARR_INT_FOR_EACH(heur->relax.cref.fact_eff + fact_id, op_id){
        op = heur->relax.op + op_id;
        if (op->cost == 0 && op->supp != -1)
            markGoalZoneRecursive(heur, op->supp);
    }
}

static void markGoalZone(plan_heur_lm_cut_t *heur)
{
    // Zeroize goal-zone flags and recursively mark facts in the goal-zone
    bzero(heur->fact_goal_zone, sizeof(int) * heur->relax.cref.fact_size);
    markGoalZoneRecursive(heur, heur->relax.cref.goal_id);
}



static void findCutAddInit(plan_heur_lm_cut_t *heur,
                           const plan_state_t *state,
                           bor_lifo_t *queue)
{
    int id;

    PLAN_FACT_ID_FOR_EACH_STATE(&heur->relax.cref.fact_id, state, id){
        heur->fact_in_queue[id] = 1;
        borLifoPush(queue, &id);
    }
    id = heur->relax.cref.fake_pre[0].fact_id;
    heur->fact_in_queue[id] = 1;
    borLifoPush(queue, &id);
}

static void findCutEnqueueEffects(plan_heur_lm_cut_t *heur,
                                  int op_id, bor_lifo_t *queue)
{
    int fact_id;
    int in_cut = 0;

    PLAN_ARR_INT_FOR_EACH(heur->relax.cref.op_eff + op_id, fact_id){
        // Determine whether the operator belongs to cut
        if (!in_cut && heur->fact_goal_zone[fact_id]){
            planArrIntAdd(&heur->cut, op_id);
            in_cut = 1;
        }

        if (!heur->fact_in_queue[fact_id]
                && !heur->fact_goal_zone[fact_id]){
            heur->fact_in_queue[fact_id] = 1;
            borLifoPush(queue, &fact_id);
        }
    }
}

static void findCut(plan_heur_lm_cut_t *heur, const plan_state_t *state)
{
    int fact_id, op_id;
    bor_lifo_t queue;

    // Zeroize in-queue flags
    bzero(heur->fact_in_queue, sizeof(int) * heur->relax.cref.fact_size);

    // Reset output structure
    heur->cut.size = 0;

    // Initialize queue and adds initial state
    borLifoInit(&queue, sizeof(int));
    findCutAddInit(heur, state, &queue);

    while (!borLifoEmpty(&queue)){
        // Pop next fact from queue
        fact_id = *(int *)borLifoBack(&queue);
        borLifoPop(&queue);

        PLAN_ARR_INT_FOR_EACH(heur->relax.cref.fact_pre + fact_id, op_id){
            if (heur->relax.op[op_id].supp == fact_id){
                findCutEnqueueEffects(heur, op_id, &queue);
            }
        }
    }

    borLifoFree(&queue);
}

static plan_cost_t updateCutCost(const plan_arr_int_t *cut,
                                 plan_heur_relax_op_t *op)
{
    int cut_cost = INT_MAX;
    int op_id;

    // Find minimal cost from the cut operators
    PLAN_ARR_INT_FOR_EACH(cut, op_id)
        cut_cost = BOR_MIN(cut_cost, op[op_id].cost);

    if (cut_cost <= 0){
        fprintf(stderr, "ERROR: Invalid cut (cost: %d)\n", cut_cost);
        exit(-1);
    }

    // Substract the minimal cost from all cut operators
    PLAN_ARR_INT_FOR_EACH(cut, op_id){
        op[op_id].cost  -= cut_cost;
        op[op_id].value -= cut_cost;
    }

    return cut_cost;
}

static void storeLandmarks(plan_heur_lm_cut_t *heur, int *op_ids, int size,
                           plan_heur_res_t *res)
{
    int i, *ops;

    if (size == 0)
        return;

    ops = BOR_ALLOC_ARR(int, size);
    for (i = 0; i < size; ++i)
        ops[i] = heur->relax.cref.op_id[op_ids[i]];
    planLandmarkSetAdd(&res->landmarks, size, ops);
    BOR_FREE(ops);
}

static int landmarkCost(const plan_heur_relax_op_t *op, int *ldm,
                        int ldm_size, int used_op_id)
{
    plan_cost_t cost = PLAN_COST_MAX;
    int i;

    for (i = 0; i < ldm_size; ++i){
        // If this landmark contains the operator that was used during
        // creation of a new state, discard this landmark (i.e., return
        // zero cost).
        if (ldm[i] == used_op_id)
            return 0;
        cost = BOR_MIN(cost, op[ldm[i]].cost);
    }

    return cost;
}

static plan_cost_t applyInitLandmarks(plan_heur_lm_cut_t *heur,
                                      const plan_landmark_set_t *ldms,
                                      int inc_op_id,
                                      plan_heur_res_t *res)
{
    plan_cost_t h = 0;
    const plan_landmark_t *ldm;
    int cost, *op_changed, i, j, size, op_id;

    op_changed = BOR_CALLOC_ARR(int, heur->relax.cref.op_size);

    // Record operators that should be changed as well as value that should
    // be substracted from their cost.
    for (i = 0; i < ldms->size; ++i){
        ldm = ldms->landmark + i;
        if (ldm->size <= 0)
            continue;

        // Determine cost of the landmark and skip zero-cost landmarks
        cost = landmarkCost(heur->relax.op, ldm->op_id, ldm->size, inc_op_id);
        if (cost <= 0)
            continue;

        // Update initial heuristic value
        h += cost;

        // Mark each operator as changed and update it by substracting the
        // cost of the landmark.
        for (j = 0; j < ldm->size; ++j){
            op_id = ldm->op_id[j];
            op_changed[op_id] = 1;
            heur->relax.op[op_id].value -= cost;
            heur->relax.op[op_id].cost  -= cost;
        }

        if (res->save_landmarks)
            storeLandmarks(heur, ldm->op_id, ldm->size, res);
    }

    // Reorganize array of changed operators
    for (size = 0, i = 0; i < heur->relax.cref.op_size; ++i){
        if (op_changed[i])
            op_changed[size++] = i;
    }

    // Update relaxation heuristic
    planHeurRelaxIncMaxFull(&heur->relax, op_changed, size);

    if (op_changed)
        BOR_FREE(op_changed);

    return h;
}

static void updateOpSupp(plan_heur_lm_cut_t *h)
{
    /*
    int op_id;
    int i, fact_id, value;

    for (op_id = 0; op_id < h->relax.cref.op_size; ++op_id){
        if (h->relax.op[op_id].supp == -1)
            continue;
        if (op_id <= 17)
            continue;
        value = h->relax.fact[h->relax.op[op_id].supp].value;

        for (i = h->relax.cref.op_pre[op_id].size - 1; i >= 0; --i){
            fact_id = h->relax.cref.op_pre[op_id].arr[i];
            if (h->relax.fact[fact_id].value == value){
                h->relax.op[op_id].supp = fact_id;
                break;
            }
        }
    }
    */
}

static void lmCutState(plan_heur_lm_cut_t *heur, const plan_state_t *state,
                       const plan_landmark_set_t *ldms, int inc_op_id,
                       plan_heur_res_t *res)
{
    plan_cost_t h = 0;

    // Compute initial h^max
    planHeurRelaxFull(&heur->relax, state);

    // If landmarks are given, apply them before continuing with LM-Cut and
    // set up initial heuristic value accordingly.
    if (ldms != NULL && ldms->size > 0)
        h = applyInitLandmarks(heur, ldms, inc_op_id, res);

    // Check whether the goal is reachable.
    if (heur->relax.fact[heur->relax.cref.goal_id].supp == -1){
        // Goal is not reachable, return dead-end
        heur->relax.fact[heur->relax.cref.goal_id].value = 0;
        h = PLAN_HEUR_DEAD_END;
    }

    while (heur->relax.fact[heur->relax.cref.goal_id].value > 0){
        // Mark facts connected with a goal by zero-cost operators in
        // justification graph.
        markGoalZone(heur);
        updateOpSupp(heur);

        // Find operators that are reachable from the initial state and are
        // connected with the goal-zone facts in justification graph.
        findCut(heur, state);

        // If no cut was found we have reached dead-end
        if (heur->cut.size == 0){
            fprintf(stderr, "Error: LM-Cut: Empty cut! Something is"
                            " seriously wrong!\n");
            exit(-1);
        }

        // Store landmarks into output structure if requested.
        if (res->save_landmarks)
            storeLandmarks(heur, heur->cut.arr, heur->cut.size, res);

        // Determine the minimal cost from all cut-operators. Substract
        // this cost from their cost and add it to the final heuristic
        // value.
        h += updateCutCost(&heur->cut, heur->relax.op);

        // Performat incremental h^max computation using changed operator
        // costs
        planHeurRelaxIncMaxFull(&heur->relax, heur->cut.arr, heur->cut.size);
    }

    res->heur = h;
}

static void planHeurLMCutState(plan_heur_t *_heur, const plan_state_t *state,
                               plan_heur_res_t *res)
{
    plan_heur_lm_cut_t *heur = HEUR(_heur);
    lmCutState(heur, state, NULL, -1, res);
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
    lmCutState(heur, state, NULL, -1, &res);

    // Save landmarks into cache
    planLandmarkSetFree(&heur->inc_local.ldms);
    heur->inc_local.ldms = res.landmarks;
    heur->inc_local.ldms_id = parent_state_id;
}

static void planHeurLMCutNodeIncLocal(plan_heur_t *_heur,
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
    lmCutState(heur, state, &heur->inc_local.ldms, op_id, res);
}

static void planHeurLMCutNodeIncCache(plan_heur_t *_heur,
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
    lmCutState(heur, state, ldms, op_id, &res);
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

static void planHeurLMCutStateInc(plan_heur_t *_heur,
                                  const plan_state_t *state,
                                  plan_heur_res_t *res)
{
    fprintf(stderr, "Error: Incremental LM-Cut cannot be called via"
                    " planHeurState() function!\n");
    exit(-1);
}

void planHeurLMCutDebug(const plan_heur_lm_cut_t *h)
{
    int i, j, f1, f2;

    fprintf(stderr, "Cut:");
    for (i = 0; i < h->cut.size; ++i){
        fprintf(stderr, " %d (op_id: %d, s: %d)", h->cut.arr[i],
                h->relax.cref.op_id[h->cut.arr[i]],
                h->relax.op[h->cut.arr[i]].supp);
    }
    fprintf(stderr, "\n");

    for (i = 0; i < h->relax.cref.op_size; ++i){
        fprintf(stderr, "op[%03d/%03d]: supp: %d", i,
                h->relax.cref.op_id[i], h->relax.op[i].supp);
        fprintf(stderr, ", pre:");
        for (j = 0; j < h->relax.cref.op_pre[i].size; ++j){
            fprintf(stderr, " %d(%d)", h->relax.cref.op_pre[i].arr[j],
                    h->relax.fact[h->relax.cref.op_pre[i].arr[j]].value);
        }
        fprintf(stderr, ", eff:");
        for (j = 0; j < h->relax.cref.op_eff[i].size; ++j){
            fprintf(stderr, " %d(%d)", h->relax.cref.op_eff[i].arr[j],
                    h->relax.fact[h->relax.cref.op_eff[i].arr[j]].value);
        }
        fprintf(stderr, ", cost: %d, value: %d",
                h->relax.op[i].cost, h->relax.op[i].value);
        fprintf(stderr, "\n");
    }
    for (i = 0; i < h->relax.cref.fact_size; ++i){
        f1 = f2 = i;
        if (i < h->relax.cref.fact_id.fact_size)
            planFactIdFromFactId(&h->relax.cref.fact_id, i, &f1, &f2);
        fprintf(stderr, "fact[%03d]: value: %d, supp: %d, gz: %d, (%d/%d)\n",
                i, h->relax.fact[i].value, h->relax.fact[i].supp,
                h->fact_goal_zone[i], f1, f2);
    }
    fprintf(stderr, "Goal: value: %d, supp: %d, gz:",
            h->relax.fact[h->relax.cref.goal_id].value,
            h->relax.fact[h->relax.cref.goal_id].supp);
    for (i = 0; i < h->relax.cref.fact_size; ++i){
        if (h->fact_goal_zone[i])
            fprintf(stderr, " %d", i);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "--------\n");
    fflush(stderr);
}
