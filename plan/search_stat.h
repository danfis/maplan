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

#ifndef __PLAN_SEARCH_STAT_H__
#define __PLAN_SEARCH_STAT_H__

#include <boruvka/timer.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Struct for statistics from search.
 */
struct _plan_search_stat_t {
    bor_timer_t timer;

    float elapsed_time;
    long steps;
    long evaluated_states;
    long expanded_states;
    long generated_states;
    long peak_memory;
    int found;
};
typedef struct _plan_search_stat_t plan_search_stat_t;

/**
 * Initializes stat struct.
 */
void planSearchStatInit(plan_search_stat_t *stat);

/**
 * Starts internal timer.
 */
void planSearchStatStartTimer(plan_search_stat_t *stat);

/**
 * Updates .peak_memory and .elapsed_time members.
 * Note that .elapsed_time is computed from last call of
 * planSearchStatStartTimer().
 */
void planSearchStatUpdate(plan_search_stat_t *stat);

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
_bor_inline void planSearchStatSetFound(plan_search_stat_t *stat);

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

_bor_inline void planSearchStatSetFound(plan_search_stat_t *stat)
{
    stat->found = 1;
}

_bor_inline void planSearchStatSetNotFound(plan_search_stat_t *stat)
{
    stat->found = 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_SEARCH_STAT_H__ */
