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

#include <sys/resource.h>
#include "plan/search_stat.h"

void planSearchStatInit(plan_search_stat_t *stat)
{
    stat->elapsed_time = 0.f;
    stat->steps = 0L;
    stat->evaluated_states = 0L;
    stat->expanded_states = 0L;
    stat->generated_states = 0L;
    stat->peak_memory = 0L;
    stat->found = -1;
}

void planSearchStatStartTimer(plan_search_stat_t *stat)
{
    borTimerStart(&stat->timer);
}

void planSearchStatUpdate(plan_search_stat_t *stat)
{
    planSearchStatUpdatePeakMemory(stat);

    borTimerStop(&stat->timer);
    stat->elapsed_time = borTimerElapsedInSF(&stat->timer);
}

void planSearchStatUpdatePeakMemory(plan_search_stat_t *stat)
{
    struct rusage usg;

    if (getrusage(RUSAGE_SELF, &usg) == 0){
        stat->peak_memory = usg.ru_maxrss / 1024L;
    }
}
