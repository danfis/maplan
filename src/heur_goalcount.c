#include <boruvka/alloc.h>

#include "plan/heur_goalcount.h"

static plan_cost_t planHeurGoalCount(void *h,
                                     const plan_state_t *state);

plan_heur_t *planHeurGoalCountNew(const plan_part_state_t *goal)
{
    plan_heur_goalcount_t *h;
    h = BOR_ALLOC(plan_heur_goalcount_t);
    planHeurInit((plan_heur_t *)h, planHeurGoalCount);
    h->goal = goal;
    return &h->heur;
}

void planHeurGoalCountDel(plan_heur_t *h)
{
    planHeurFree(h);
    BOR_FREE(h);
}

static plan_cost_t planHeurGoalCount(void *_h,
                                     const plan_state_t *state)
{
    plan_heur_goalcount_t *h = _h;
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
