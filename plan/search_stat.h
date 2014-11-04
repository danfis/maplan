#ifndef __PLAN_SEARCH_STAT_H__
#define __PLAN_SEARCH_STAT_H__

#include <boruvka/timer.h>
#include <boruvka/fifo.h>

#include <plan/problem.h>
#include <plan/statespace.h>
#include <plan/heur.h>
#include <plan/list_lazy.h>
#include <plan/path.h>
#include <plan/ma_comm.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Struct for statistics from search.
 */
struct _plan_search_stat_t {
    float elapsed_time;
    long steps;
    long evaluated_states;
    long expanded_states;
    long generated_states;
    long peak_memory;
    int found;
    int not_found;
};
typedef struct _plan_search_stat_t plan_search_stat_t;

/**
 * Initializes stat struct.
 */
void planSearchStatInit(plan_search_stat_t *stat);

/**
 * Updates .peak_memory value of stat structure.
 */
void planSearchStatUpdatePeakMemory(plan_search_stat_t *stat);

/**
 * Increments number of evaluated states by one.
 */
_bor_inline void planSearchStatIncEvaluatedStates(plan_search_stat_t *stat);

/**
 * Increments number of expanded states by one.
 */
_bor_inline void planSearchStatIncExpandedStates(plan_search_stat_t *stat);

/**
 * Increments number of generated states by one.
 */
_bor_inline void planSearchStatIncGeneratedStates(plan_search_stat_t *stat);

/**
 * Set "found" flag which means that solution was found.
 */
_bor_inline void planSearchStatSetFoundSolution(plan_search_stat_t *stat);

/**
 * Sets "not_found" flag meaning no solution was found.
 */
_bor_inline void planSearchStatSetNotFound(plan_search_stat_t *stat);

/**** INLINES ****/
_bor_inline void planSearchStatIncEvaluatedStates(plan_search_stat_t *stat)
{
    ++stat->evaluated_states;
}

_bor_inline void planSearchStatIncExpandedStates(plan_search_stat_t *stat)
{
    ++stat->expanded_states;
}

_bor_inline void planSearchStatIncGeneratedStates(plan_search_stat_t *stat)
{
    ++stat->generated_states;
}

_bor_inline void planSearchStatSetFoundSolution(plan_search_stat_t *stat)
{
    stat->found = 1;
}

_bor_inline void planSearchStatSetNotFound(plan_search_stat_t *stat)
{
    stat->not_found = 1;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_SEARCH_STAT_H__ */
