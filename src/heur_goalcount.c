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

plan_cost_t planHeurGoalCount(plan_heur_goalcount_t *h,
                              const plan_state_t *state)
{
    int i;
    plan_var_id_t var;
    plan_val_t val;
    plan_cost_t heur;

    heur = PLAN_COST_ZERO;
    PLAN_PART_STATE_FOR_EACH(h->goal, i, var, val){
        if (val != planStateGet(state, var))
            ++heur;
    }
    return heur;
}
