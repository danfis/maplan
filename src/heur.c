#include "plan/heur.h"

void _planHeurInit(plan_heur_t *heur,
                   plan_heur_del_fn del_fn,
                   plan_heur_fn heur_fn,
                   plan_heur2_fn heur2_fn)
{
    heur->del_fn   = del_fn;
    heur->heur_fn  = heur_fn;
    heur->heur2_fn = heur2_fn;
}

void _planHeurFree(plan_heur_t *heur)
{
}

void planHeurDel(plan_heur_t *heur)
{
    heur->del_fn(heur);
}

void planHeur(plan_heur_t *heur, const plan_state_t *state,
              plan_heur_res_t *res)
{
    res->pref_size = 0;
    heur->heur_fn(heur, state, res);
}

void planHeur2(plan_heur_t *heur,
               const plan_state_t *state,
               const plan_part_state_t *goal,
               plan_heur_res_t *res)
{
    res->pref_size = 0;
    heur->heur2_fn(heur, state, goal, res);
}
