#include <boruvka/alloc.h>
#include <boruvka/timer.h>

#include "plan/search.h"

static void extractPath(plan_state_space_t *state_space,
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

    res = search->init_step_fn(search);
    while (res == PLAN_SEARCH_CONT){
        res = search->step_fn(search);

        ++steps;
        if (res == PLAN_SEARCH_CONT
                && search->progress.fn
                && steps >= search->progress.freq){
            planSearchStatUpdate(&search->stat);
            res = search->progress.fn(&search->stat, search->progress.data);
            steps = 0;
        }
    }

    if (res == PLAN_SEARCH_FOUND){
        extractPath(search->state_space, search->goal_state, path);
        planSearchStatSetFound(&search->stat);
    }else{
        planSearchStatSetNotFound(&search->stat);
    }

    if (search->progress.fn && res != PLAN_SEARCH_ABORT && steps != 0){
        planSearchStatUpdate(&search->stat);
        search->progress.fn(&search->stat, search->progress.data);
    }

    return res;
}


void _planSearchInit(plan_search_t *search,
                     const plan_search_params_t *params,
                     plan_search_del_fn del_fn,
                     plan_search_init_step_fn init_step_fn,
                     plan_search_step_fn step_fn)
{
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

    search->state    = planStateNew(search->state_pool);
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

void _planSearchFindApplicableOps(plan_search_t *search,
                                  plan_state_id_t state_id)
{
    _planSearchLoadState(search, state_id);
    planSearchApplicableOpsFind(&search->app_ops, search->state, state_id,
                                search->succ_gen);
}

int _planSearchHeuristic(plan_search_t *search,
                         plan_state_id_t state_id,
                         plan_cost_t *heur_val,
                         plan_search_applicable_ops_t *preferred_ops)
{
    plan_heur_res_t res;
    int fres = PLAN_SEARCH_CONT;

    _planSearchLoadState(search, state_id);

    planHeurResInit(&res);
    if (preferred_ops){
        res.pref_op = preferred_ops->op;
        res.pref_op_size = preferred_ops->op_found;
    }

    planHeur(search->heur, search->state, &res);
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
    if (found)
        search->goal_state = node->state_id;
    return found;
}




static void extractPath(plan_state_space_t *state_space,
                        plan_state_id_t goal_state,
                        plan_path_t *path)
{
    plan_state_space_node_t *node;

    planPathInit(path);

    node = planStateSpaceNode(state_space, goal_state);
    while (node && node->op){
        planPathPrepend(path, node->op,
                        node->parent_state_id, node->state_id);
        node = planStateSpaceNode(state_space, node->parent_state_id);
    }
}

static void _planSearchLoadState(plan_search_t *search,
                                 plan_state_id_t state_id)
{
    if (search->state_id == state_id)
        return;
    planStatePoolGetState(search->state_pool, state_id, search->state);
    search->state_id = state_id;
}
