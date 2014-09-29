#include "_heur_relax.c"


static void heurDel(plan_heur_t *_heur)
{
    plan_heur_relax_t *heur = HEUR_FROM_PARENT(_heur);
    planHeurRelaxFree(heur);
    BOR_FREE(heur);
}

static plan_heur_t *heurNew(int type,
                            const plan_var_t *var, int var_size,
                            const plan_part_state_t *goal,
                            const plan_op_t *op, int op_size,
                            const plan_succ_gen_t *succ_gen)
{
    plan_heur_relax_t *heur;

    heur = BOR_ALLOC(plan_heur_relax_t);
    _planHeurInit(&heur->heur, heurDel, planHeurRelax);
    planHeurRelaxInit(heur, type, var, var_size, goal, op, op_size, succ_gen);

    return &heur->heur;
}

plan_heur_t *planHeurRelaxAddNew(const plan_var_t *var, int var_size,
                                 const plan_part_state_t *goal,
                                 const plan_op_t *op, int op_size,
                                 const plan_succ_gen_t *succ_gen)
{
    return heurNew(TYPE_ADD, var, var_size, goal, op, op_size, succ_gen);
}

plan_heur_t *planHeurRelaxMaxNew(const plan_var_t *var, int var_size,
                                 const plan_part_state_t *goal,
                                 const plan_op_t *op, int op_size,
                                 const plan_succ_gen_t *succ_gen)
{
    return heurNew(TYPE_MAX, var, var_size, goal, op, op_size, succ_gen);
}

plan_heur_t *planHeurRelaxFFNew(const plan_var_t *var, int var_size,
                                const plan_part_state_t *goal,
                                const plan_op_t *op, int op_size,
                                const plan_succ_gen_t *succ_gen)
{
    return heurNew(TYPE_FF, var, var_size, goal, op, op_size, succ_gen);
}
