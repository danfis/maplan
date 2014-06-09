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
    size_t i;
    unsigned var, val;
    unsigned heur;

    heur = 0;
    PLAN_PART_STATE_FOR_EACH(h->goal, i, var, val){
        if (val != planStateGet(state, var))
            ++heur;
    }
    return heur;
}
