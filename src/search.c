#include <sys/resource.h>
#include <boruvka/alloc.h>
#include <boruvka/timer.h>

#include "plan/search.h"

static void updateStat(plan_search_stat_t *stat,
                       long steps,
                       bor_timer_t *timer);

static void extractPath(plan_state_space_t *state_space,
                        plan_state_id_t goal_state,
                        plan_path_t *path);

void planSearchStatInit(plan_search_stat_t *stat)
{
    stat->elapsed_time = 0.f;
    stat->steps = 0L;
    stat->evaluated_states = 0L;
    stat->expanded_states = 0L;
    stat->generated_states = 0L;
    stat->peak_memory = 0L;
    stat->found_solution = 0;
    stat->dead_end = 0;
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
                     plan_search_step_fn step_fn)
{
    search->del_fn  = del_fn;
    search->init_fn = init_fn;
    search->step_fn = step_fn;
    search->params = *params;

    planSearchStatInit(&search->stat);

    search->state_space = planStateSpaceNew(params->prob->state_pool);
    search->state       = planStateNew(params->prob->state_pool);
    search->succ_op     = BOR_ALLOC_ARR(plan_operator_t *, params->prob->op_size);
    search->goal_state  = PLAN_NO_STATE;
}

void _planSearchFree(plan_search_t *search)
{
    if (search->succ_op)
        BOR_FREE(search->succ_op);
    if (search->state)
        planStateDel(search->params.prob->state_pool, search->state);
    if (search->state_space)
        planStateSpaceDel(search->state_space);
}

plan_cost_t _planSearchHeuristic(plan_search_t *search,
                                 plan_state_id_t state_id,
                                 plan_heur_t *heur)
{
    planStatePoolGetState(search->params.prob->state_pool,
                          state_id, search->state);
    planSearchStatIncEvaluatedStates(&search->stat);
    return planHeur(heur, search->state);
}

static int findApplicableOperators(plan_search_t *search,
                                   plan_state_id_t state_id)
{
    // unroll the state into search->state struct
    planStatePoolGetState(search->params.prob->state_pool,
                          state_id, search->state);

    // get operators to get successors
    return planSuccGenFind(search->params.prob->succ_gen,
                           search->state,
                           search->succ_op,
                           search->params.prob->op_size);
}

void _planSearchAddLazySuccessors(plan_search_t *search,
                                  plan_state_id_t state_id,
                                  plan_cost_t cost,
                                  plan_list_lazy_t *list)
{
    int i, op_size;
    plan_operator_t *op;

    // Store applicable operators in search->succ_op[]
    op_size = findApplicableOperators(search, state_id);

    // go trough all applicable operators
    for (i = 0; i < op_size; ++i){
        op = search->succ_op[i];
        planListLazyPush(list, cost, state_id, op);
    }

    planSearchStatIncExpandedStates(&search->stat);
}

void _planSearchReachedDeadEnd(plan_search_t *search)
{
    planSearchStatSetDeadEnd(&search->stat);
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
    planSearchStatIncGeneratedStates(&search->stat);

    if (new_state_id)
        *new_state_id = state_id;
    if (new_node)
        *new_node = node;

    if (planStateSpaceNodeIsNew(node))
        return 0;
    return -1;
}

void _planSearchNodeOpenClose(plan_search_t *search,
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
        res = search->step_fn(search);

        ++steps;
        if (search->params.progress_fn
                && steps == search->params.progress_freq){
            updateStat(&search->stat, steps, &timer);
            res = search->params.progress_fn(&search->stat);
            steps = 0;
        }
    }

    if (search->params.progress_fn && res != PLAN_SEARCH_ABORT && steps != 0){
        updateStat(&search->stat, steps, &timer);
        res = search->params.progress_fn(&search->stat);
    }

    if (res == PLAN_SEARCH_FOUND){
        extractPath(search->state_space, search->goal_state, path);
    }

    return res;
}

static void updateStat(plan_search_stat_t *stat,
                       long steps,
                       bor_timer_t *timer)
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
        planPathPrepend(path, node->op);
        node = planStateSpaceNode(state_space, node->parent_state_id);
    }
}
