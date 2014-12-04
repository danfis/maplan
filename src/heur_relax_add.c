#include <boruvka/alloc.h>
#include "plan/heur.h"
#include "heur_relax.h"

struct _plan_heur_relax_add_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;
};
typedef struct _plan_heur_relax_add_t plan_heur_relax_add_t;

#define HEUR(parent) bor_container_of(parent, plan_heur_relax_add_t, heur)

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_relax_add_t *heur = HEUR(_heur);
    _planHeurFree(&heur->heur);
    planHeurRelaxFree(&heur->relax);
    BOR_FREE(heur);
}

static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    plan_heur_relax_add_t *heur = HEUR(_heur);
    planHeurRelaxRun(&heur->relax, PLAN_HEUR_RELAX_TYPE_ADD, state);
    res->heur = heur->relax.fact[heur->relax.cref.goal_id].value;
    if (res->heur == PLAN_COST_MAX)
        res->heur = PLAN_HEUR_DEAD_END;
}

plan_heur_t *planHeurRelaxAddNew(const plan_var_t *var, int var_size,
                                 const plan_part_state_t *goal,
                                 const plan_op_t *op, int op_size,
                                 const plan_succ_gen_t *succ_gen)
{
    plan_heur_relax_add_t *heur;

    heur = BOR_ALLOC(plan_heur_relax_add_t);
    _planHeurInit(&heur->heur, heurDel, heurVal);
    planHeurRelaxInit(&heur->relax, var, var_size, goal, op, op_size);

    return &heur->heur;
}
