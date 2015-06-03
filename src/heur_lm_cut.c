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
    plan_heur_res_landmarks_t ldms; /*!< Cached landmarks */
    plan_state_id_t ldms_id;        /*!< ID of the state for which the
                                         landmarks are cached */
    plan_op_id_tr_t op_id_tr; /*!< Translation from global ID to local ID */
};
typedef struct _inc_local_t inc_local_t;

struct _plan_heur_lm_cut_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;

    int *fact_goal_zone; /*!< Flag for each fact whether it is in goal zone */
    int *fact_in_queue;
    plan_oparr_t cut;    /*!< Array of cut operators */

    inc_local_t inc_local; /*!< Struct for local incremental LM-Cut */
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
static void planHeurLMCutStateIncLocal(plan_heur_t *_heur,
                                       const plan_state_t *state,
                                       plan_heur_res_t *res);

static plan_heur_t *lmCutNew(const plan_var_t *var, int var_size,
                             const plan_part_state_t *goal,
                             const plan_op_t *op, int op_size,
                             int inc_local)
{
    plan_heur_lm_cut_t *heur;

    heur = BOR_ALLOC(plan_heur_lm_cut_t);
    if (inc_local){
        _planHeurInit(&heur->heur, planHeurLMCutDel,
                      planHeurLMCutStateIncLocal,
                      planHeurLMCutNodeIncLocal);
    }else{
        _planHeurInit(&heur->heur, planHeurLMCutDel, planHeurLMCutState, NULL);
    }
    planHeurRelaxInit(&heur->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      var, var_size, goal, op, op_size);

    heur->fact_goal_zone = BOR_ALLOC_ARR(int, heur->relax.cref.fact_size);
    heur->fact_in_queue  = BOR_ALLOC_ARR(int, heur->relax.cref.fact_size);
    heur->cut.op = BOR_ALLOC_ARR(int, heur->relax.cref.op_size);
    heur->cut.size = 0;

    heur->inc_local.enabled = 0;
    if (inc_local){
        heur->inc_local.enabled = 1;
        bzero(&heur->inc_local.ldms, sizeof(heur->inc_local.ldms));
        heur->inc_local.ldms_id = PLAN_NO_STATE;
        planOpIdTrInit(&heur->inc_local.op_id_tr, op, op_size);
    }

    return &heur->heur;
}

plan_heur_t *planHeurLMCutNew(const plan_var_t *var, int var_size,
                              const plan_part_state_t *goal,
                              const plan_op_t *op, int op_size)
{
    return lmCutNew(var, var_size, goal, op, op_size, 0);
}

plan_heur_t *planHeurLMCutIncLocalNew(const plan_var_t *var, int var_size,
                                      const plan_part_state_t *goal,
                                      const plan_op_t *op, int op_size)
{
    return lmCutNew(var, var_size, goal, op, op_size, 1);
}

static void planHeurLMCutDel(plan_heur_t *_heur)
{
    plan_heur_lm_cut_t *heur = HEUR(_heur);

    BOR_FREE(heur->cut.op);
    BOR_FREE(heur->fact_goal_zone);
    BOR_FREE(heur->fact_in_queue);
    if (heur->inc_local.enabled){
        planHeurResLandmarksFree(&heur->inc_local.ldms);
        planOpIdTrFree(&heur->inc_local.op_id_tr);
    }
    planHeurRelaxFree(&heur->relax);
    _planHeurFree(&heur->heur);
    BOR_FREE(heur);
}



static void markGoalZoneRecursive(plan_heur_lm_cut_t *heur, int fact_id)
{
    int i, len, *op_ids, op_id;
    plan_heur_relax_op_t *op;

    if (heur->fact_goal_zone[fact_id])
        return;

    heur->fact_goal_zone[fact_id] = 1;

    len    = heur->relax.cref.fact_eff[fact_id].size;
    op_ids = heur->relax.cref.fact_eff[fact_id].op;
    for (i = 0; i < len; ++i){
        op_id = op_ids[i];
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
    plan_var_id_t var, len;
    plan_val_t val;
    int id;

    len = planStateSize(state);
    for (var = 0; var < len; ++var){
        val = planStateGet(state, var);
        id = planFactId(&heur->relax.cref.fact_id, var, val);
        if (id >= 0){
            heur->fact_in_queue[id] = 1;
            borLifoPush(queue, &id);
        }
    }
    id = heur->relax.cref.fake_pre[0].fact_id;
    heur->fact_in_queue[id] = 1;
    borLifoPush(queue, &id);
}

static void findCutEnqueueEffects(plan_heur_lm_cut_t *heur,
                                  int op_id, bor_lifo_t *queue)
{
    int i, len, *facts, fact_id;
    int in_cut = 0;

    len   = heur->relax.cref.op_eff[op_id].size;
    facts = heur->relax.cref.op_eff[op_id].fact;
    for (i = 0; i < len; ++i){
        fact_id = facts[i];

        // Determine whether the operator belongs to cut
        if (!in_cut && heur->fact_goal_zone[fact_id]){
            heur->cut.op[heur->cut.size++] = op_id;
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
    int i, len, *ops, fact_id, op_id;
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

        len = heur->relax.cref.fact_pre[fact_id].size;
        ops = heur->relax.cref.fact_pre[fact_id].op;
        for (i = 0; i < len; ++i){
            op_id = ops[i];

            if (heur->relax.op[op_id].supp == fact_id){
                findCutEnqueueEffects(heur, op_id, &queue);
            }
        }
    }

    borLifoFree(&queue);
}

static plan_cost_t updateCutCost(const plan_oparr_t *cut, plan_heur_relax_op_t *op)
{
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

    // Substract the minimal cost from all cut operators
    for (i = 0; i < len; ++i){
        op_id = ops[i];
        op[op_id].cost  -= cut_cost;
        op[op_id].value -= cut_cost;
    }

    return cut_cost;
}

static void storeLandmarks(plan_heur_lm_cut_t *heur, int *op_ids, int size,
                           plan_heur_res_t *res)
{
    plan_heur_res_landmarks_t *ldms;
    plan_heur_res_landmark_t *ldm;
    int i;

    if (size == 0)
        return;

    ldms = &res->landmarks;
    ++ldms->num_landmarks;
    ldms->landmark = BOR_REALLOC_ARR(ldms->landmark, plan_heur_res_landmark_t,
                                     ldms->num_landmarks);
    ldm = ldms->landmark + ldms->num_landmarks - 1;

    ldm->size = size;
    ldm->op_id = BOR_ALLOC_ARR(int, ldm->size);
    for (i = 0; i < ldm->size; ++i){
        ldm->op_id[i] = heur->relax.cref.op_id[op_ids[i]];
    }
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
                                      const plan_heur_res_landmarks_t *ldms,
                                      int inc_op_id,
                                      plan_heur_res_t *res)
{
    plan_cost_t h = 0;
    const plan_heur_res_landmark_t *ldm;
    int cost, *op_changed, i, j, size, op_id;

    // Reuse structure for cut
    op_changed = heur->cut.op;
    bzero(op_changed, sizeof(*heur->cut.op) * heur->relax.cref.op_size);

    // Record operators that should be changed as well as value that should
    // be substracted from their cost.
    for (i = 0; i < ldms->num_landmarks; ++i){
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
    planHeurRelaxIncMax(&heur->relax, op_changed, size);

    return h;
}

static void lmCutState(plan_heur_lm_cut_t *heur, const plan_state_t *state,
                       const plan_heur_res_landmarks_t *ldms, int inc_op_id,
                       plan_heur_res_t *res)
{
    plan_cost_t h = 0;

    // Compute initial h^max
    planHeurRelax(&heur->relax, state);

    // If landmarks are given, apply them before continuing with LM-Cut and
    // set up initial heuristic value accordingly.
    if (ldms != NULL && ldms->num_landmarks > 0)
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
            storeLandmarks(heur, heur->cut.op, heur->cut.size, res);

        // Determine the minimal cost from all cut-operators. Substract
        // this cost from their cost and add it to the final heuristic
        // value.
        h += updateCutCost(&heur->cut, heur->relax.op);

        // Performat incremental h^max computation using changed operator
        // costs
        planHeurRelaxIncMax(&heur->relax, heur->cut.op, heur->cut.size);
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
    planHeurResLandmarksFree(&heur->inc_local.ldms);
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
        planHeurResLandmarksFree(&heur->inc_local.ldms);
        bzero(&heur->inc_local.ldms, sizeof(heur->inc_local.ldms));
    }

    // Compute heuristic for the current state
    state = planSearchLoadState(search, state_id);
    if (node->op != NULL)
        op_id = planOpIdTrLoc(&heur->inc_local.op_id_tr, node->op->global_id);
    lmCutState(heur, state, &heur->inc_local.ldms, op_id, res);
}

static void planHeurLMCutStateIncLocal(plan_heur_t *_heur,
                                       const plan_state_t *state,
                                       plan_heur_res_t *res)
{
    fprintf(stderr, "Error: Incremental LM-Cut cannot be called via"
                    " planHeurState() function!\n");
    exit(-1);
}
