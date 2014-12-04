#include <boruvka/alloc.h>
#include "plan/heur.h"
#include "heur_relax.h"
#include "pref_op_selector.h"

struct _plan_heur_relax_add_max_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;
    const plan_op_t *base_op;
    int relax_op;
};
typedef struct _plan_heur_relax_add_max_t plan_heur_relax_add_max_t;

#define HEUR(parent) bor_container_of(parent, plan_heur_relax_add_max_t, heur)

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_relax_add_max_t *heur = HEUR(_heur);
    _planHeurFree(&heur->heur);
    planHeurRelaxFree(&heur->relax);
    BOR_FREE(heur);
}

static void prefOps(plan_heur_relax_add_max_t *heur, plan_heur_res_t *res)
{
    plan_pref_op_selector_t sel;
    int i;

    // Compute relaxed plan
    planHeurRelaxMarkPlan(&heur->relax);

    // Select preferred operators as those that are in relaxed plan
    planPrefOpSelectorInit(&sel, res, heur->base_op);
    for (i = 0; i < heur->relax.cref.op_size; ++i){
        if (heur->relax.plan_op[i])
            planPrefOpSelectorSelect(&sel, i);
    }
    planPrefOpSelectorFinalize(&sel);
}

static void heurVal(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    plan_heur_relax_add_max_t *heur = HEUR(_heur);

    // Compute relaxation heuristic
    planHeurRelaxRun(&heur->relax, heur->relax_op, state);

    // Pick up the value
    res->heur = heur->relax.fact[heur->relax.cref.goal_id].value;
    if (res->heur == PLAN_COST_MAX)
        res->heur = PLAN_HEUR_DEAD_END;

    // Select preferred operators if requested
    if (res->pref_op)
        prefOps(heur, res);
}

static plan_heur_t *heurNew(const plan_var_t *var, int var_size,
                            const plan_part_state_t *goal,
                            const plan_op_t *op, int op_size,
                            int relax_op)
{
    plan_heur_relax_add_max_t *heur;

    heur = BOR_ALLOC(plan_heur_relax_add_max_t);
    heur->base_op = op;
    heur->relax_op = relax_op;
    
    _planHeurInit(&heur->heur, heurDel, heurVal);
    planHeurRelaxInit(&heur->relax, var, var_size, goal, op, op_size);

    return &heur->heur;
}

plan_heur_t *planHeurRelaxAddNew(const plan_var_t *var, int var_size,
                                 const plan_part_state_t *goal,
                                 const plan_op_t *op, int op_size,
                                 const plan_succ_gen_t *succ_gen)
{
    return heurNew(var, var_size, goal, op, op_size,
                   PLAN_HEUR_RELAX_TYPE_ADD);
}

plan_heur_t *planHeurRelaxMaxNew(const plan_var_t *var, int var_size,
                                 const plan_part_state_t *goal,
                                 const plan_op_t *op, int op_size,
                                 const plan_succ_gen_t *succ_gen)
{
    return heurNew(var, var_size, goal, op, op_size,
                   PLAN_HEUR_RELAX_TYPE_MAX);
}
