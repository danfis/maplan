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
#include <boruvka/timer.h>

#include "plan/search.h"

static plan_state_id_t extractPath(plan_state_space_t *state_space,
                                   plan_state_id_t goal_state,
                                   plan_path_t *path);
static void _planSearchLoadState(plan_search_t *search,
                                 plan_state_id_t state_id);




void planSearchParamsInit(plan_search_params_t *params)
{
    bzero(params, sizeof(*params));
}

void planSearchDel(plan_search_t *search)
{
    search->del_fn(search);
}

int planSearchRun(plan_search_t *search, plan_path_t *path)
{
    int res;
    long steps = 0;

    planSearchStatStartTimer(&search->stat);

    if (search->succ_gen->num_operators == 0){
        res = PLAN_SEARCH_NOT_FOUND;
    }else{
        res = search->init_step_fn(search);
    }
    while (res == PLAN_SEARCH_CONT){
        res = search->step_fn(search);
        if (search->abort)
            res = PLAN_SEARCH_ABORT;

        ++steps;
        if (res == PLAN_SEARCH_CONT
                && search->progress.fn
                && steps >= search->progress.freq){
            search->stat.steps += steps;
            planSearchStatUpdate(&search->stat);
            res = search->progress.fn(&search->stat, search->progress.data);
            steps = 0;
        }

        if (search->poststep_fn){
            res = search->poststep_fn(search, res, search->poststep_data);
        }
    }

    if (res == PLAN_SEARCH_FOUND){
        if (search->goal_state != PLAN_NO_STATE)
            extractPath(search->state_space, search->goal_state, path);
        planSearchStatSetFound(&search->stat);
    }else{
        planSearchStatSetNotFound(&search->stat);
    }

    if (search->progress.fn && res != PLAN_SEARCH_ABORT && steps != 0){
        search->stat.steps += steps;
        planSearchStatUpdate(&search->stat);
        search->progress.fn(&search->stat, search->progress.data);
    }

    return res;
}

void planSearchAbort(plan_search_t *search)
{
    search->abort = 1;
}

plan_state_id_t planSearchExtractPath(const plan_search_t *search,
                                      plan_state_id_t goal_state,
                                      plan_path_t *path)
{
    return extractPath(search->state_space, goal_state, path);
}

plan_cost_t planSearchStateHeur(const plan_search_t *search,
                                plan_state_id_t state_id)
{
    plan_state_space_node_t *node;
    node = planStateSpaceNode(search->state_space, state_id);
    return node->heuristic;
}

void planSearchInsertNode(plan_search_t *search,
                          plan_state_space_node_t *node)
{
    if (search->insert_node_fn)
        search->insert_node_fn(search, node);
}

void planSearchSetPostStep(plan_search_t *search,
                           plan_search_poststep_fn fn, void *userdata)
{
    search->poststep_fn = fn;
    search->poststep_data = userdata;
}

void planSearchSetExpandedNode(plan_search_t *search,
                               plan_search_expanded_node_fn cb, void *ud)
{
    search->expanded_node_fn = cb;
    search->expanded_node_data = ud;
}

void planSearchSetReachedGoal(plan_search_t *search,
                              plan_search_reached_goal_fn cb,
                              void *userdata)
{
    search->reached_goal_fn = cb;
    search->reached_goal_data = userdata;
}

void planSearchSetMAHeur(plan_search_t *search,
                         plan_search_ma_heur_fn cb, void *userdata)
{
    search->ma_heur_fn = cb;
    search->ma_heur_data = userdata;
}

const plan_state_t *planSearchLoadState(plan_search_t *search,
                                        plan_state_id_t state_id)
{
    _planSearchLoadState(search, state_id);
    return search->state;
}

plan_state_space_node_t *planSearchLoadNode(plan_search_t *search,
                                            plan_state_id_t state_id)
{
    return planStateSpaceNode(search->state_space, state_id);
}

void _planSearchInit(plan_search_t *search,
                     const plan_search_params_t *params,
                     plan_search_del_fn del_fn,
                     plan_search_init_step_fn init_step_fn,
                     plan_search_step_fn step_fn,
                     plan_search_insert_node_fn insert_node_fn,
                     plan_search_top_node_cost_fn top_node_cost_fn)
{
    search->abort         = 0;
    search->heur          = params->heur;
    search->heur_del      = params->heur_del;
    search->initial_state = params->prob->initial_state;
    search->state_pool    = params->prob->state_pool;
    search->state_space   = planStateSpaceNew(search->state_pool);
    search->succ_gen      = params->prob->succ_gen;
    search->goal          = params->prob->goal;
    search->progress      = params->progress;

    search->del_fn  = del_fn;
    search->init_step_fn = init_step_fn;
    search->step_fn = step_fn;
    search->insert_node_fn = insert_node_fn;
    search->top_node_cost_fn = top_node_cost_fn;
    search->poststep_fn = NULL;
    search->poststep_data = NULL;
    search->expanded_node_fn = NULL;
    search->expanded_node_data = NULL;
    search->reached_goal_fn = NULL;
    search->reached_goal_data = NULL;
    search->ma_heur_fn = NULL;
    search->ma_heur_data = NULL;

    search->state    = planStateNew(search->state_pool->num_vars);
    search->state_id = PLAN_NO_STATE;
    planSearchStatInit(&search->stat);
    planSearchApplicableOpsInit(&search->app_ops, params->prob->op_size);
    search->goal_state  = PLAN_NO_STATE;
}

void _planSearchFree(plan_search_t *search)
{
    planSearchApplicableOpsFree(&search->app_ops);
    if (search->heur && search->heur_del)
        planHeurDel(search->heur);
    if (search->state)
        planStateDel(search->state);
    if (search->state_space)
        planStateSpaceDel(search->state_space);
}

int _planSearchFindApplicableOps(plan_search_t *search,
                                 plan_state_id_t state_id)
{
    _planSearchLoadState(search, state_id);
    return planSearchApplicableOpsFind(&search->app_ops, search->state,
                                       state_id, search->succ_gen);
}

int _planSearchHeur(plan_search_t *search,
                    plan_state_space_node_t *node,
                    plan_cost_t *heur_val,
                    plan_search_applicable_ops_t *preferred_ops)
{
    plan_heur_res_t res;
    int fres = PLAN_SEARCH_CONT;

    planHeurResInit(&res);
    if (preferred_ops){
        res.pref_op = preferred_ops->op;
        res.pref_op_size = preferred_ops->op_found;
    }

    if (search->heur->ma){
        if (search->ma_heur_fn){
            search->ma_heur_fn(search, search->heur, node->state_id, &res,
                               search->ma_heur_data);
        }else{
            fprintf(stderr, "Search Error: ma_heur_fn callback is not set."
                            " Multi agent heuristic cannot be computed!\n");
            res.heur = PLAN_HEUR_DEAD_END;
        }
    }else{
        planHeurNode(search->heur, node->state_id, search, &res);
    }
    planSearchStatIncEvaluatedStates(&search->stat);

    if (preferred_ops){
        preferred_ops->op_preferred = res.pref_size;
    }

    *heur_val = res.heur;
    return fres;
}

int _planSearchCheckGoal(plan_search_t *search, plan_state_space_node_t *node)
{
    int found;

    found = planStatePoolPartStateIsSubset(search->state_pool,
                                           search->goal, node->state_id);
    if (found){
        search->goal_state = node->state_id;
        if (search->reached_goal_fn)
            search->reached_goal_fn(search, node, search->reached_goal_data);
    }
    return found;
}

plan_state_id_t planSearchApplyOp(plan_search_t *search,
                                  plan_state_id_t state_id,
                                  plan_op_t *op)
{
    plan_state_id_t next_state;
    plan_state_space_node_t *cur_node, *next_node;
    plan_cost_t heur;

    cur_node = planStateSpaceNode(search->state_space, state_id);
    next_state = planOpApply(op, search->state_pool, state_id);
    next_node = planStateSpaceNode(search->state_space, next_state);
    _planSearchHeur(search, next_node, &heur, NULL);

    next_node->parent_state_id = state_id;
    next_node->op = op;
    next_node->cost = cur_node->cost + op->cost;
    next_node->heuristic = heur;

    return next_state;
}

static plan_state_id_t extractPath(plan_state_space_t *state_space,
                                   plan_state_id_t goal_state,
                                   plan_path_t *path)
{
    plan_state_space_node_t *node;

    planPathInit(path);

    node = planStateSpaceNode(state_space, goal_state);
    while (node && node->op){
        planPathPrependOp(path, node->op,
                          node->parent_state_id, node->state_id);
        node = planStateSpaceNode(state_space, node->parent_state_id);
    }

    if (node)
        return node->state_id;
    return PLAN_NO_STATE;
}

static void _planSearchLoadState(plan_search_t *search,
                                 plan_state_id_t state_id)
{
    if (search->state_id == state_id)
        return;
    planStatePoolGetState(search->state_pool, state_id, search->state);
    search->state_id = state_id;
}
