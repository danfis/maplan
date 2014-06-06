#include <boruvka/alloc.h>

#include "plan/heur_goalcount.h"

plan_heur_goalcount_t *planHeurGoalCountNew(const plan_part_state_t *goal)
{
    plan_heur_goalcount_t *h;
    h = BOR_ALLOC(plan_heur_goalcount_t);
    h->goal = goal;
    return h;
}

void planHeurGoalCountDel(plan_heur_goalcount_t *h)
{
    BOR_FREE(h);
}

unsigned planHeurGoalCount(plan_heur_goalcount_t *h,
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
