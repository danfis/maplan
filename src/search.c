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
    search->succ_op     = BOR_ALLOC_ARR(plan_operator_t *, params->prob->op_size);
    search->goal_state  = PLAN_NO_STATE;
}

void _planSearchFree(plan_search_t *search)
{
    if (search->succ_op)
        BOR_FREE(search->succ_op);
    if (search->state_space)
        planStateSpaceDel(search->state_space);
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
