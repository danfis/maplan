#include "plan/heur.h"

void planHeurInit(plan_heur_t *heur,
                  plan_heur_fn heur_fn)
{
    heur->heur_fn = heur_fn;
}

void planHeurFree(plan_heur_t *heur)
{
}

plan_cost_t planHeur(plan_heur_t *heur, const plan_state_t *state)
{
    return heur->heur_fn(heur, state);
}
