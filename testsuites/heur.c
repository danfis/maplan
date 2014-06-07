#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/heur_relax.h"
#include "plan/succgen.h"


TEST(testHeurRelax)
{
    plan_problem_t *p;
    plan_heur_relax_t *heur;
    plan_state_t *state;
    plan_state_id_t sid;
    plan_succ_gen_t *succgen;
    plan_operator_t *op;
    unsigned h;

    p = planProblemFromJson("load-from-file.in1.json");
    succgen = planSuccGenNew(p->op, p->op_size);
    //planProblemDump(p, stderr);

    heur = planHeurRelaxNew(p->op, p->op_size,
                            p->var, p->var_size,
                            p->goal);

    state = planStateNew(p->state_pool);
    planStatePoolGetState(p->state_pool, p->initial_state, state);

    h = planHeurRelax(heur, state);
    assertEquals(h, 10);

    assertTrue(planSuccGenFind(succgen, state, &op, 1) > 0);
    sid = planOperatorApply(op, p->initial_state);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeurRelax(heur, state);
    assertEquals(h, 13);

    assertTrue(planSuccGenFind(succgen, state, &op, 1) > 0);
    sid = planOperatorApply(op, sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeurRelax(heur, state);
    assertEquals(h, 7);

    assertTrue(planSuccGenFind(succgen, state, &op, 1) > 0);
    sid = planOperatorApply(op, sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeurRelax(heur, state);
    assertEquals(h, 10);

    assertTrue(planSuccGenFind(succgen, state, &op, 1) > 0);
    sid = planOperatorApply(op, sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeurRelax(heur, state);
    assertEquals(h, 7);

    planStateDel(p->state_pool, state);
    planHeurRelaxDel(heur);
    planSuccGenDel(succgen);
    planProblemDel(p);
}
