#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/heur_relax.h"
#include "plan/succgen.h"


TEST(testHeurRelaxAdd)
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

    heur = planHeurRelaxAddNew(p);

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

    planStateSet2(state, 9, 3, 1, 1, 1, 0, 0, 4, 3, 2);
    h = planHeurRelax(heur, state);
    assertEquals(h, PLAN_HEUR_DEAD_END);

    planStateDel(p->state_pool, state);
    planHeurRelaxDel(heur);
    planSuccGenDel(succgen);
    planProblemDel(p);
}

TEST(testHeurRelaxMax)
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

    heur = planHeurRelaxMaxNew(p);

    state = planStateNew(p->state_pool);
    planStatePoolGetState(p->state_pool, p->initial_state, state);

    h = planHeurRelax(heur, state);
    assertEquals(h, 5);

    assertTrue(planSuccGenFind(succgen, state, &op, 1) > 0);
    sid = planOperatorApply(op, p->initial_state);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeurRelax(heur, state);
    assertEquals(h, 5);

    assertTrue(planSuccGenFind(succgen, state, &op, 1) > 0);
    sid = planOperatorApply(op, sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeurRelax(heur, state);
    assertEquals(h, 4);

    assertTrue(planSuccGenFind(succgen, state, &op, 1) > 0);
    sid = planOperatorApply(op, sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeurRelax(heur, state);
    assertEquals(h, 4);

    assertTrue(planSuccGenFind(succgen, state, &op, 1) > 0);
    sid = planOperatorApply(op, sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeurRelax(heur, state);
    assertEquals(h, 3);

    planStateSet2(state, 9, 3, 1, 1, 1, 0, 0, 4, 3, 2);
    h = planHeurRelax(heur, state);
    assertEquals(h, PLAN_HEUR_DEAD_END);

    planStateDel(p->state_pool, state);
    planHeurRelaxDel(heur);
    planSuccGenDel(succgen);
    planProblemDel(p);
}