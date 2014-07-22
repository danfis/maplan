#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/heur.h"


TEST(testHeurRelaxAdd)
{
    plan_problem_t *p;
    plan_heur_t *heur;
    plan_state_t *state;
    plan_state_id_t sid;
    plan_succ_gen_t *succgen;
    plan_operator_t **op;
    plan_cost_t h;
    plan_heur_preferred_ops_t preferred;
    int op_size;

    p = planProblemFromFD("load-from-file.in1.sas");
    succgen = planSuccGenNew(p->op, p->op_size);
    //planProblemDump(p, stderr);

    op = alloca(sizeof(plan_operator_t *) * p->op_size);

    heur = planHeurRelaxAddNew(p->var, p->var_size, p->goal,
                               p->op, p->op_size, p->succ_gen);

    state = planStateNew(p->state_pool);
    planStatePoolGetState(p->state_pool, p->initial_state, state);

    op_size = planSuccGenFind(succgen, state, op, p->op_size);
    preferred.op = op;
    preferred.op_size = op_size;
    h = planHeur(heur, state, &preferred);
    assertEquals(h, 10);
    assertEquals(preferred.preferred_size, 1);
    assertEquals(strcmp(preferred.op[0]->name, "unstack b c"), 0);

    assertTrue(planSuccGenFind(succgen, state, op, 1) > 0);
    sid = planOperatorApply(op[0], p->initial_state);
    planStatePoolGetState(p->state_pool, sid, state);
    op_size = planSuccGenFind(succgen, state, op, p->op_size);
    preferred.op = op;
    preferred.op_size = op_size;
    h = planHeur(heur, state, &preferred);
    assertEquals(h, 13);
    assertEquals(preferred.preferred_size, 1);
    assertEquals(strcmp(preferred.op[0]->name, "put-down b"), 0);

    assertTrue(planSuccGenFind(succgen, state, op, 2) > 0);
    sid = planOperatorApply(op[1], sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeur(heur, state, NULL);
    assertEquals(h, 7);

    assertTrue(planSuccGenFind(succgen, state, op, 2) > 0);
    sid = planOperatorApply(op[1], sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeur(heur, state, NULL);
    assertEquals(h, 10);

    assertTrue(planSuccGenFind(succgen, state, op, 1) > 0);
    sid = planOperatorApply(op[0], sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeur(heur, state, NULL);
    assertEquals(h, 8);

    planStateSet2(state, 9, 3, 1, 1, 1, 0, 0, 4, 3, 2);
    op_size = planSuccGenFind(succgen, state, op, p->op_size);
    preferred.op = op;
    preferred.op_size = op_size;
    h = planHeur(heur, state, &preferred);
    assertEquals(h, PLAN_HEUR_DEAD_END);
    assertEquals(preferred.preferred_size, 0);


    planStateSet2(state, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    op_size = planSuccGenFind(succgen, state, op, p->op_size);
    preferred.op = op;
    preferred.op_size = op_size;
    h = planHeur(heur, state, &preferred);
    assertEquals(h, 6);
    assertEquals(preferred.preferred_size, 3);
    assertEquals(strcmp(preferred.op[0]->name, "pick-up a"), 0);
    assertEquals(strcmp(preferred.op[1]->name, "unstack c d"), 0);
    assertEquals(strcmp(preferred.op[2]->name, "unstack d b"), 0);

    planStateDel(state);
    planHeurDel(heur);
    planSuccGenDel(succgen);
    planProblemDel(p);
}

TEST(testHeurRelaxMax)
{
    plan_problem_t *p;
    plan_heur_t *heur;
    plan_state_t *state;
    plan_state_id_t sid;
    plan_succ_gen_t *succgen;
    plan_operator_t **op;
    plan_cost_t h;
    plan_heur_preferred_ops_t preferred;
    int op_size;

    p = planProblemFromFD("load-from-file.in1.sas");
    succgen = planSuccGenNew(p->op, p->op_size);
    //planProblemDump(p, stderr);

    op = alloca(sizeof(plan_operator_t *) * p->op_size);

    heur = planHeurRelaxMaxNew(p->var, p->var_size, p->goal,
                               p->op, p->op_size, p->succ_gen);

    state = planStateNew(p->state_pool);
    planStatePoolGetState(p->state_pool, p->initial_state, state);

    h = planHeur(heur, state, NULL);
    assertEquals(h, 5);

    assertTrue(planSuccGenFind(succgen, state, op, 1) > 0);
    sid = planOperatorApply(op[0], p->initial_state);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeur(heur, state, NULL);
    assertEquals(h, 5);

    assertTrue(planSuccGenFind(succgen, state, op, 2) > 0);
    sid = planOperatorApply(op[1], sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeur(heur, state, NULL);
    assertEquals(h, 4);

    assertTrue(planSuccGenFind(succgen, state, op, 2) > 0);
    sid = planOperatorApply(op[1], sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeur(heur, state, NULL);
    assertEquals(h, 4);

    assertTrue(planSuccGenFind(succgen, state, op, 2) > 0);
    sid = planOperatorApply(op[0], sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeur(heur, state, NULL);
    assertEquals(h, 3);

    planStateSet2(state, 9, 3, 1, 1, 1, 0, 0, 4, 3, 2);
    h = planHeur(heur, state, NULL);
    assertEquals(h, PLAN_HEUR_DEAD_END);

    planStateSet2(state, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    op_size = planSuccGenFind(succgen, state, op, p->op_size);
    preferred.op = op;
    preferred.op_size = op_size;
    h = planHeur(heur, state, &preferred);
    assertEquals(h, 2);
    assertEquals(preferred.preferred_size, 3);
    assertEquals(strcmp(preferred.op[0]->name, "pick-up a"), 0);
    assertEquals(strcmp(preferred.op[1]->name, "unstack c d"), 0);
    assertEquals(strcmp(preferred.op[2]->name, "unstack d b"), 0);

    planStateDel(state);
    planHeurDel(heur);
    planSuccGenDel(succgen);
    planProblemDel(p);
}

TEST(testHeurRelaxFF)
{
    plan_problem_t *p;
    plan_heur_t *heur;
    plan_state_t *state;
    plan_state_id_t sid;
    plan_succ_gen_t *succgen;
    plan_operator_t **op;
    plan_cost_t h;
    plan_heur_preferred_ops_t preferred;
    int op_size;

    p = planProblemFromFD("load-from-file.in1.sas");
    succgen = planSuccGenNew(p->op, p->op_size);
    //planProblemDump(p, stderr);

    heur = planHeurRelaxFFNew(p->var, p->var_size, p->goal,
                              p->op, p->op_size, p->succ_gen);

    op = alloca(sizeof(plan_operator_t *) * p->op_size);

    state = planStateNew(p->state_pool);
    planStatePoolGetState(p->state_pool, p->initial_state, state);

    op_size = planSuccGenFind(succgen, state, op, p->op_size);
    preferred.op = op;
    preferred.op_size = op_size;
    h = planHeur(heur, state, &preferred);
    assertEquals(h, 6);
    assertEquals(preferred.preferred_size, 1);
    assertEquals(strcmp(preferred.op[0]->name, "unstack b c"), 0);

    op_size = planSuccGenFind(succgen, state, op, p->op_size);
    assertTrue(op_size > 0);
    sid = planOperatorApply(op[0], p->initial_state);
    planStatePoolGetState(p->state_pool, sid, state);
    op_size = planSuccGenFind(succgen, state, op, p->op_size);
    preferred.op = op;
    preferred.op_size = op_size;
    h = planHeur(heur, state, &preferred);
    assertEquals(h, 6);
    assertEquals(preferred.preferred_size, 1);
    assertEquals(strcmp(preferred.op[0]->name, "put-down b"), 0);

    assertTrue(planSuccGenFind(succgen, state, op, 2) > 0);
    sid = planOperatorApply(op[1], sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeur(heur, state, NULL);
    assertEquals(h, 5);

    assertTrue(planSuccGenFind(succgen, state, op, 1) > 0);
    sid = planOperatorApply(op[0], sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeur(heur, state, NULL);
    assertEquals(h, 6);

    assertTrue(planSuccGenFind(succgen, state, op, 1) > 0);
    sid = planOperatorApply(op[0], sid);
    planStatePoolGetState(p->state_pool, sid, state);
    h = planHeur(heur, state, NULL);
    assertEquals(h, 6);

    planStateSet2(state, 9, 3, 1, 1, 1, 0, 0, 4, 3, 2);
    op_size = planSuccGenFind(succgen, state, op, p->op_size);
    preferred.op = op;
    preferred.op_size = op_size;
    h = planHeur(heur, state, &preferred);
    assertEquals(h, PLAN_HEUR_DEAD_END);
    assertEquals(preferred.preferred_size, 0);

    planStateSet2(state, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    op_size = planSuccGenFind(succgen, state, op, p->op_size);
    preferred.op = op;
    preferred.op_size = op_size;
    h = planHeur(heur, state, &preferred);
    assertEquals(h, 6);
    assertEquals(preferred.preferred_size, 3);
    assertEquals(strcmp(preferred.op[0]->name, "pick-up a"), 0);
    assertEquals(strcmp(preferred.op[1]->name, "unstack c d"), 0);
    assertEquals(strcmp(preferred.op[2]->name, "unstack d b"), 0);

    planStateDel(state);
    planHeurDel(heur);
    planSuccGenDel(succgen);
    planProblemDel(p);
}

TEST(testHeurRelaxLMCut)
{
    plan_problem_t *p;
    plan_heur_t *heur;
    plan_state_t *state;
    plan_cost_t h;

    p = planProblemFromFD("../data/ma-benchmarks/depot/pfile5.sas");
    heur = planHeurLMCutNew(p->var, p->var_size, p->goal,
                            p->op, p->op_size, p->succ_gen);

    state = planStateNew(p->state_pool);

    planStatePoolGetState(p->state_pool, 0, state);
    h = planHeur(heur, state, NULL);
    fprintf(stderr, "h: %d\n", (int)h);


    planStateDel(state);
    planHeurDel(heur);
    planProblemDel(p);
}
