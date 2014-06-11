#ifndef __PLAN_SEARCH_H__
#define __PLAN_SEARCH_H__

#define PLAN_SEARCH_CONTINUE 0
#define PLAN_SEARCH_ABORT    -1
#define PLAN_SEARCH_END      1

struct _plan_search_stat_t {
    float elapsed_time;
    long steps;
    long evaluated_states;
    long expanded_states;
    long generated_states;
    long peak_memory;
    int found_solution;
    int dead_end;
};
typedef struct _plan_search_stat_t plan_search_stat_t;

void planSearchStatInit(plan_search_stat_t *stat);
void planSearchStatUpdatePeakMemory(plan_search_stat_t *stat);

typedef int (*plan_search_progress_fn)(const plan_search_stat_t *stat);

#endif /* __PLAN_SEARCH_H__ */
