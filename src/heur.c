#include "plan/heur.h"

void planHeurInit(plan_heur_t *heur,
                  plan_heur_del_fn del_fn,
                  plan_heur_fn heur_fn)
{
    heur->del_fn  = del_fn;
    heur->heur_fn = heur_fn;
}

void planHeurFree(plan_heur_t *heur)
{
}

void planHeurDel(plan_heur_t *heur)
{
    heur->del_fn(heur);
}

plan_cost_t planHeur(plan_heur_t *heur, const plan_state_t *state,
                     plan_heur_preferred_ops_t *preferred_ops)
{
    return heur->heur_fn(heur, state);
}
