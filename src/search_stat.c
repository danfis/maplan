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
    stat->elapsed_time = borTimerElapsedInSF(&stat->timer);
}

void planSearchStatUpdatePeakMemory(plan_search_stat_t *stat)
{
    struct rusage usg;

    if (getrusage(RUSAGE_SELF, &usg) == 0){
        stat->peak_memory = usg.ru_maxrss;
    }
}
