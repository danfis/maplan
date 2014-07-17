#include <boruvka/alloc.h>

#include "plan/heur.h"

struct _plan_heur_goalcount_t {
    plan_heur_t heur;
    const plan_part_state_t *goal;
};
typedef struct _plan_heur_goalcount_t plan_heur_goalcount_t;

static plan_cost_t planHeurGoalCount(void *h, const plan_state_t *state,
                                     plan_heur_preferred_ops_t *preferred_ops);
static void planHeurGoalCountDel(void *h);

plan_heur_t *planHeurGoalCountNew(const plan_part_state_t *goal)
{
    plan_heur_goalcount_t *h;
    h = BOR_ALLOC(plan_heur_goalcount_t);
    planHeurInit(&h->heur,
                 planHeurGoalCountDel,
                 planHeurGoalCount);
    h->goal = goal;
    return &h->heur;
}

static void planHeurGoalCountDel(void *_h)
{
    plan_heur_goalcount_t *h = _h;
    planHeurFree(&h->heur);
    BOR_FREE(h);
}

static plan_cost_t planHeurGoalCount(void *_h,
                                     const plan_state_t *state,
                                     plan_heur_preferred_ops_t *preferred_ops)
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
