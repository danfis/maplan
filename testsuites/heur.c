#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/heur_relax.h"


TEST(testHeurRelax)
{
    plan_problem_t *p;
    plan_heur_relax_t *heur;
    plan_state_t *state;
    unsigned h;

    p = planProblemFromJson("load-from-file.in1.json");

    heur = planHeurRelaxNew(p->op, p->op_size,
                            p->var, p->var_size,
                            p->goal);

    state = planStateNew(p->state_pool);
    planStatePoolGetState(p->state_pool, p->initial_state, state);

    h = planHeurRelax(heur, state);
    fprintf(stderr, "Heur: %u\n", h);

    planStateDel(p->state_pool, state);
    planHeurRelaxDel(heur);
    planProblemDel(p);
}
