#include <sys/resource.h>
#include <boruvka/alloc.h>
#include <boruvka/timer.h>

#include "plan/search.h"

static void extractPath(plan_state_space_t *state_space,
                        plan_state_id_t goal_state,
                        plan_path_t *path);

/** Initializes and destroys a struct for holding applicable operators */
static void planSearchApplicableOpsInit(plan_search_t *search, int op_size);
static void planSearchApplicableOpsFree(plan_search_t *search);
/** Fills search->applicable_ops with operators applicable in specified
 *  state */
_bor_inline void planSearchApplicableOpsFind(plan_search_t *search,
                                             plan_state_id_t state_id);

void planSearchStatInit(plan_search_stat_t *stat)
{
    stat->elapsed_time = 0.f;
    stat->steps = 0L;
    stat->evaluated_states = 0L;
    stat->expanded_states = 0L;
    stat->generated_states = 0L;
    stat->peak_memory = 0L;
    stat->found = 0;
    stat->not_found = 0;
}

void planSearchStepChangeInit(plan_search_step_change_t *step_change)
{
    bzero(step_change, sizeof(*step_change));
}

void planSearchStepChangeFree(plan_search_step_change_t *step_change)
{
    if (step_change->closed_node)
        BOR_FREE(step_change->closed_node);
    bzero(step_change, sizeof(*step_change));
}

void planSearchStepChangeReset(plan_search_step_change_t *step_change)
{
    step_change->closed_node_size = 0;
}

void planSearchStepChangeAddClosedNode(plan_search_step_change_t *sc,
                                       plan_state_space_node_t *node)
{
    if (sc->closed_node_size + 1 > sc->closed_node_alloc){
        sc->closed_node_alloc = sc->closed_node_size + 1;
        sc->closed_node = BOR_REALLOC_ARR(sc->closed_node,
                                          plan_state_space_node_t *,
                                          sc->closed_node_alloc);
    }

    sc->closed_node[sc->closed_node_size++] = node;
}

void planSearchStatUpdatePeakMemory(plan_search_stat_t *stat)
{
    struct rusage usg;

    if (getrusage(RUSAGE_SELF, &usg) == 0){
        stat->peak_memory = usg.ru_maxrss;
    }
}

void planSearchParamsInit(plan_search_params_t *params)
{
    bzero(params, sizeof(*params));
}

void _planSearchInit(plan_search_t *search,
                     const plan_search_params_t *params,
                     plan_search_del_fn del_fn,
                     plan_search_init_fn init_fn,
                     plan_search_step_fn step_fn,
                     plan_search_inject_state_fn inject_state_fn)
{
    search->del_fn  = del_fn;
    search->init_fn = init_fn;
    search->step_fn = step_fn;
    search->inject_state_fn = inject_state_fn;
    search->params = *params;

    planSearchStatInit(&search->stat);

    search->state_pool  = params->prob->state_pool;
    search->state_space = planStateSpaceNew(search->state_pool);
    search->state       = planStateNew(search->state_pool);
    search->goal_state  = PLAN_NO_STATE;

    planSearchApplicableOpsInit(search, search->params.prob->op_size);
}

void _planSearchFree(plan_search_t *search)
{
    planSearchApplicableOpsFree(search);
    if (search->state)
        planStateDel(search->state);
    if (search->state_space)
        planStateSpaceDel(search->state_space);
}

void _planSearchFindApplicableOps(plan_search_t *search,
                                  plan_state_id_t state_id)
{
    planSearchApplicableOpsFind(search, state_id);
}

plan_cost_t _planSearchHeuristic(plan_search_t *search,
                                 plan_state_id_t state_id,
                                 plan_heur_t *heur,
                                 plan_search_applicable_ops_t *preferred_ops)
{
    plan_heur_preferred_ops_t pref_ops;
    plan_cost_t hval;

    planStatePoolGetState(search->state_pool, state_id, search->state);
    planSearchStatIncEvaluatedStates(&search->stat);

    if (preferred_ops){
        pref_ops.op = preferred_ops->op;
        pref_ops.op_size = preferred_ops->op_found;
        hval = planHeur(heur, search->state, &pref_ops);
        preferred_ops->op_preferred = pref_ops.preferred_size;

    }else{
        hval = planHeur(heur, search->state, NULL);
    }

    return hval;
}

void _planSearchAddLazySuccessors(plan_search_t *search,
                                  plan_state_id_t state_id,
                                  plan_operator_t **op, int op_size,
                                  plan_cost_t cost,
                                  plan_list_lazy_t *list)
{
    int i;

    for (i = 0; i < op_size; ++i){
        planListLazyPush(list, cost, state_id, op[i]);
        planSearchStatIncGeneratedStates(&search->stat);
    }
}

int _planSearchLazyInjectState(plan_search_t *search,
                               plan_heur_t *heur,
                               plan_list_lazy_t *list,
                               plan_state_id_t state_id,
                               plan_cost_t cost, plan_cost_t heur_val)
{
    plan_state_space_node_t *node;

    // Retrieve node corresponding to the state
    node = planStateSpaceNode(search->state_space, state_id);

    // If the node was not discovered yet insert it into open-list
    if (planStateSpaceNodeIsNew(node)){
        // Compute heuristic value
        if (heur){
            heur_val = _planSearchHeuristic(search, state_id, heur, NULL);
        }

        // Set node to closed state with appropriate cost and heuristic
        // value
        _planSearchNodeOpenClose(search, state_id,
                                 PLAN_NO_STATE, NULL, cost, heur_val);

        // Add node's successor to the open-list
        _planSearchFindApplicableOps(search, state_id);
        _planSearchAddLazySuccessors(search, state_id,
                                     search->applicable_ops.op,
                                     search->applicable_ops.op_found,
                                     heur_val, list);
    }

    return 0;
}

void _planSearchReachedDeadEnd(plan_search_t *search)
{
    planSearchStatSetNotFound(&search->stat);
}

int _planSearchNewState(plan_search_t *search,
                        plan_operator_t *operator,
                        plan_state_id_t parent_state,
                        plan_state_id_t *new_state_id,
                        plan_state_space_node_t **new_node)
{
    plan_state_id_t state_id;
    plan_state_space_node_t *node;

    state_id = planOperatorApply(operator, parent_state);
    node     = planStateSpaceNode(search->state_space, state_id);

    if (new_state_id)
        *new_state_id = state_id;
    if (new_node)
        *new_node = node;

    if (planStateSpaceNodeIsNew(node))
        return 0;
    return -1;
}

plan_state_space_node_t *_planSearchNodeOpenClose(plan_search_t *search,
                                                  plan_state_id_t state,
                                                  plan_state_id_t parent_state,
                                                  plan_operator_t *parent_op,
                                                  plan_cost_t cost,
                                                  plan_cost_t heur)
{
    plan_state_space_node_t *node;

    node = planStateSpaceOpen2(search->state_space, state,
                               parent_state, parent_op,
                               cost, heur);
    planStateSpaceClose(search->state_space, node);
    return node;
}

int _planSearchCheckGoal(plan_search_t *search, plan_state_id_t state_id)
{
    if (planProblemCheckGoal(search->params.prob, state_id)){
        search->goal_state = state_id;
        planSearchStatSetFoundSolution(&search->stat);
        return 1;
    }

    return 0;
}

void planSearchDel(plan_search_t *search)
{
    search->del_fn(search);
}

int planSearchRun(plan_search_t *search, plan_path_t *path)
{
    int res;
    long steps = 0;
    bor_timer_t timer;

    borTimerStart(&timer);

    res = search->init_fn(search);
    while (res == PLAN_SEARCH_CONT){
        res = search->step_fn(search, NULL);

        ++steps;
        if (res == PLAN_SEARCH_CONT
                && search->params.progress_fn
                && steps >= search->params.progress_freq){
            _planUpdateStat(&search->stat, steps, &timer);
            res = search->params.progress_fn(&search->stat,
                                             search->params.progress_data);
            steps = 0;
        }
    }

    if (search->params.progress_fn && res != PLAN_SEARCH_ABORT && steps != 0){
        _planUpdateStat(&search->stat, steps, &timer);
        search->params.progress_fn(&search->stat,
                                   search->params.progress_data);
    }

    if (res == PLAN_SEARCH_FOUND){
        extractPath(search->state_space, search->goal_state, path);
    }

    return res;
}

void planSearchBackTrackPath(plan_search_t *search, plan_path_t *path)
{
    if (search->goal_state != PLAN_NO_STATE)
        planSearchBackTrackPathFrom(search, search->goal_state, path);
}

void planSearchBackTrackPathFrom(plan_search_t *search,
                                 plan_state_id_t from_state,
                                 plan_path_t *path)
{
    extractPath(search->state_space, from_state, path);
}


void _planUpdateStat(plan_search_stat_t *stat,
                     long steps, bor_timer_t *timer)
{
    stat->steps += steps;

    borTimerStop(timer);
    stat->elapsed_time = borTimerElapsedInSF(timer);

    planSearchStatUpdatePeakMemory(stat);
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

static void planSearchApplicableOpsInit(plan_search_t *search, int op_size)
{
    search->applicable_ops.op = BOR_ALLOC_ARR(plan_operator_t *, op_size);
    search->applicable_ops.op_size = op_size;
    search->applicable_ops.op_found = 0;
    search->applicable_ops.state = PLAN_NO_STATE;
}

static void planSearchApplicableOpsFree(plan_search_t *search)
{
    BOR_FREE(search->applicable_ops.op);
}

_bor_inline void planSearchApplicableOpsFind(plan_search_t *search,
                                             plan_state_id_t state_id)
{
    plan_search_applicable_ops_t *app = &search->applicable_ops;

    if (state_id == app->state)
        return;

    // unroll the state into search->state struct
    planStatePoolGetState(search->state_pool, state_id, search->state);

    // get operators to get successors
    app->op_found = planSuccGenFind(search->params.prob->succ_gen,
                                    search->state, app->op, app->op_size);

    // remember the corresponding state
    app->state = state_id;
}
