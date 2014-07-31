#include <boruvka/alloc.h>
#include "plan/ma_heur.h"

plan_ma_heur_lazy_ff_t *planMAHeurLazyFFNew(const plan_problem_agent_t *p)
{
    plan_ma_heur_lazy_ff_t *heur;

    heur = BOR_ALLOC(plan_ma_heur_lazy_ff_t);
    heur->ff = planHeurRelaxFFNew(p->prob.var, p->prob.var_size, p->prob.goal,
                                  p->projected_op, p->projected_op_size,
                                  NULL);

    return heur;
}

void planMAHeurLazyFFDel(plan_ma_heur_lazy_ff_t *heur)
{
    planHeurDel(heur->ff);
    BOR_FREE(heur);
}

int planMAHeurLazyFFStart(plan_ma_heur_lazy_ff_t *heur,
                          const plan_state_t *state)
{
    return 1;
}
