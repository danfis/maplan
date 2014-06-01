#include <boruvka/alloc.h>

#include "plan/heuristic/goalcount.h"

plan_heuristic_goalcount_t *planHeuristicGoalCountNew(
                const plan_part_state_t *goal)
{
    plan_heuristic_goalcount_t *h;
    h = BOR_ALLOC(plan_heuristic_goalcount_t);
    h->goal = goal;
    return h;
}

void planHeuristicGoalCountDel(plan_heuristic_goalcount_t *h)
{
    BOR_FREE(h);
}

unsigned planHeuristicGoalCount(plan_heuristic_goalcount_t *h,
                                const plan_state_t *state)
{
    size_t i, size;
    unsigned val;

    val = 0;
    size = planPartStateSize(h->goal);
    for (i = 0; i < size; ++i){
        if (planPartStateIsSet(h->goal, i)
                && planPartStateGet(h->goal, i) != planStateGet(state, i)){
            ++val;
        }
    }

    return val;
}
