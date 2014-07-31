#ifndef __PLAN_MA_HEUR_H__
#define __PLAN_MA_HEUR_H__

#include <plan/heur.h>

struct _plan_ma_heur_lazy_ff_t {
    plan_heur_t *ff;
    plan_cost_t hval;
};
typedef struct _plan_ma_heur_lazy_ff_t plan_ma_heur_lazy_ff_t;


plan_ma_heur_lazy_ff_t *planMAHeurLazyFFNew(const plan_problem_agent_t *p);
void planMAHeurLazyFFDel(plan_ma_heur_lazy_ff_t *heur);

int planMAHeurLazyFFStart(plan_ma_heur_lazy_ff_t *heur,
                          const plan_state_t *state);
int planMAHeurLazyFFUpdate(plan_ma_heur_lazy_ff_t *heur);

_bor_inline plan_cost_t planMAHeurLazyFFVal(const plan_ma_heur_lazy_ff_t *heur);


/**** INLINES: ****/
_bor_inline plan_cost_t planMAHeurLazyFFVal(const plan_ma_heur_lazy_ff_t *heur)
{
    return heur->hval;
}

#endif /* __PLAN_MA_HEUR_H__ */
