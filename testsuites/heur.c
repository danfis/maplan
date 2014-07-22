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

struct _lm_cut_test_t {
    plan_val_t state[38];
    plan_cost_t heur;
};
typedef struct _lm_cut_test_t lm_cut_test_t;

lm_cut_test_t lm_cut_test[] = {
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 0, 2, 1, 1, 1, 2, 1, 1, 1, 1, 0
        , 1, 1, 1, 0, 0, 0, 0, 0, 16, 14, 5, 6, 8, 7, 15, 11, 12, 10 }, 25},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 2, 1, 1, 1, 0, 1
        , 1, 1, 1, 0, 0, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 10 }, 22},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 3, 2, 1, 1, 1, 0, 1
        , 1, 1, 0, 1, 0, 1, 1, 0, 16, 14, 5, 6, 2, 7, 15, 11, 3, 10 }, 22},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 7, 15, 11, 12, 4 }, 20},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 0, 2, 1, 1, 1, 3, 1, 1, 1, 1, 0
        , 0, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 8, 7, 15, 11, 12, 4 }, 23},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 3, 3, 1, 1, 1, 0, 1
        , 0, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 6, 2, 7, 15, 11, 3, 4 }, 22},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 7, 15, 11, 12, 4 }, 22},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 7, 15, 11, 12, 4 }, 21},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 7, 15, 11, 12, 4 }, 21},
    { { 0, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 7, 15, 11, 12, 4 }, 21},
    { { 2, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 7, 15, 11, 12, 4 }, 22},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 0, 2, 1, 1, 1, 3, 1, 1, 1, 1, 0
        , 0, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 8, 7, 15, 11, 12, 4 }, 22},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 2, 1, 1, 1, 0, 1
        , 1, 1, 1, 0, 0, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 10 }, 24},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 3, 3, 1, 1, 1, 0, 1
        , 0, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 6, 2, 7, 15, 11, 3, 4 }, 21},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 0 }, 20},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 0, 2, 1, 1, 1, 3, 1, 1, 1, 1, 0
        , 0, 1, 1, 0, 1, 0, 0, 0, 16, 14, 5, 6, 8, 7, 15, 11, 12, 0 }, 21},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 4, 15, 11, 12, 0 }, 20},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 3, 3, 1, 1, 1, 0, 1
        , 0, 1, 0, 1, 1, 1, 1, 0, 16, 14, 5, 6, 2, 7, 15, 11, 3, 0 }, 20},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 0 }, 21},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 0 }, 21},
    { { 0, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 0 }, 21},
    { { 2, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 0 }, 20},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 0, 3, 1, 1, 1, 3, 1, 1, 0, 1, 0
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 8, 4, 15, 11, 12, 0 }, 21},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 3, 3, 1, 1, 0, 0, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 6, 2, 4, 15, 11, 3, 0 }, 20},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 4, 15, 11, 12, 0 }, 19},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 0, 3, 1, 1, 1, 3, 1, 1, 0, 1, 0
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 8, 4, 15, 11, 12, 0 }, 20},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 3, 3, 1, 1, 0, 0, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 6, 2, 4, 15, 11, 3, 0 }, 20},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 4, 15, 11, 12, 0 }, 20},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 0 }, 19},
    { { 0, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 4, 15, 11, 12, 0 }, 20},
    { { 2, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 4, 15, 11, 12, 0 }, 21},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 0, 0, 0, 16, 14, 5, 6, 0, 7, 15, 11, 12, 0 }, 20},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 4, 15, 11, 12, 0 }, 19},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 3, 3, 1, 1, 0, 0, 1
        , 1, 1, 0, 1, 1, 0, 1, 1, 16, 14, 5, 6, 0, 4, 15, 11, 3, 0 }, 21},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 0 }, 20},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 0 }, 20},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 2 }, 19},
    { { 0, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 0 }, 19},
    { { 2, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 0 }, 20},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 3, 3, 2, 1, 1, 1, 3, 1, 0, 1, 1, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 7, 15, 11, 12, 0 }, 22},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 3, 3, 1, 0, 0, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 2, 0, 4, 15, 11, 3, 0 }, 21},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 4, 15, 11, 12, 0 }, 20},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 4, 15, 11, 12, 0 }, 18},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 2, 1, 1, 1, 3, 1, 0, 1, 1, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 7, 15, 11, 12, 0 }, 19},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 3, 3, 1, 0, 0, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 2, 0, 4, 15, 11, 3, 0 }, 22},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 0, 15, 11, 12, 0 }, 18},
    { { 0, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 4, 15, 11, 12, 0 }, 22},
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 4, 15, 11, 12, 0 }, 18},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 0, 16, 14, 5, 6, 0, 0, 15, 11, 12, 0 }, 19},
    { { 1, 2, 1, 1, 1, 2, 0, 3, 3, 3, 3, 1, 1, 1, 3, 0, 0, 1, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 4, 2, 0, 0, 15, 11, 12, 0 }, 18},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 3, 3, 1, 0, 0, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 0, 16, 14, 5, 2, 0, 0, 15, 11, 3, 0 }, 21},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 0, 15, 11, 12, 0 }, 21},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 0, 15, 11, 12, 0 }, 19},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 }, 16},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 4, 0, 15, 11, 12, 0 }, 19},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 2, 3, 1, 1, 1, 3, 1, 0, 1, 1, 0
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 7, 0, 15, 11, 12, 0 }, 19},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 3, 3, 1, 0, 0, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 2, 4, 0, 15, 11, 3, 0 }, 20},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 }, 19},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 }, 18},
    { { 0, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 }, 20},
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 }, 16},
    { { 2, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 4, 0, 15, 11, 12, 0 }, 19},
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 2, 3, 1, 1, 1, 3, 1, 0, 1, 1, 0
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 7, 0, 15, 11, 12, 0 }, 19},
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 3, 3, 1, 0, 0, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 2, 4, 0, 15, 11, 3, 0 }, 20},
    { { 2, 0, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 }, 20},
    { { 2, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 }, 19},
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 0, 15, 11, 12, 0 }, 18},
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 1, 0, 15, 11, 12, 0 }, 18},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 0, 15, 11, 12, 4 }, 18},
    { { 0, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 0, 15, 11, 12, 0 }, 20},
    { { 2, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 0 }, 20},
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 3, 2, 1, 1, 1, 3, 1, 0, 1, 1, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 7, 15, 11, 12, 0 }, 19},
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 3, 3, 1, 0, 0, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 2, 0, 4, 15, 11, 3, 0 }, 18},
    { { 2, 0, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 4, 15, 11, 12, 0 }, 22},
    { { 2, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 4, 15, 11, 12, 0 }, 21},
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 1, 15, 11, 12, 0 }, 18},
    { { 1, 2, 1, 1, 1, 2, 0, 3, 0, 3, 3, 1, 1, 1, 3, 0, 1, 1, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 4, 6, 0, 0, 15, 11, 12, 0 }, 19},
    { { 1, 2, 1, 1, 1, 2, 0, 3, 3, 3, 3, 1, 1, 3, 3, 0, 0, 1, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 4, 2, 0, 0, 15, 11, 3, 0 }, 18},
    { { 1, 0, 1, 1, 1, 2, 0, 3, 3, 3, 3, 1, 1, 1, 3, 0, 0, 1, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 4, 2, 0, 0, 15, 11, 12, 0 }, 19},
    { { 1, 1, 1, 1, 1, 2, 0, 3, 3, 3, 3, 1, 1, 1, 3, 0, 0, 1, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 4, 2, 0, 0, 15, 11, 12, 0 }, 19},
    { { 1, 2, 1, 1, 1, 2, 0, 3, 3, 3, 3, 1, 1, 1, 3, 0, 0, 1, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 0, 2, 0, 0, 15, 11, 12, 0 }, 20},
    { { 0, 2, 1, 1, 1, 2, 0, 3, 3, 3, 3, 1, 1, 1, 3, 0, 0, 1, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 4, 2, 0, 0, 15, 11, 12, 0 }, 19},
    { { 2, 2, 1, 1, 1, 2, 0, 3, 3, 3, 3, 1, 1, 1, 3, 0, 0, 1, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 4, 2, 0, 0, 15, 11, 12, 0 }, 18},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 4, 0, 15, 11, 12, 0 }, 19},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 3, 2, 3, 1, 1, 1, 3, 1, 0, 1, 1, 0
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 7, 0, 15, 11, 12, 0 }, 18},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 3, 3, 1, 0, 0, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 2, 4, 0, 15, 11, 3, 0 }, 19},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 1, 1, 16, 14, 5, 2, 4, 3, 15, 11, 12, 0 }, 16},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 1, 1, 16, 14, 5, 6, 4, 3, 15, 11, 12, 0 }, 17},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 3, 2, 3, 1, 1, 1, 3, 1, 0, 1, 1, 0
        , 1, 1, 1, 0, 1, 1, 1, 0, 16, 14, 5, 2, 7, 3, 15, 11, 12, 0 }, 17},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 3, 3, 1, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 0, 1, 1, 1, 1, 1, 0, 1, 16, 14, 5, 2, 4, 12, 15, 11, 12, 0 }, 17},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 1, 1, 16, 14, 5, 2, 4, 3, 15, 11, 12, 0 }, 22},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 1, 1, 16, 14, 5, 2, 4, 3, 15, 11, 12, 0 }, 17},
    { { 0, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 1, 1, 16, 14, 5, 2, 4, 3, 15, 11, 12, 0 }, 22},
    { { 2, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 1, 1, 16, 14, 5, 2, 4, 3, 15, 11, 12, 0 }, 17},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 1, 15, 11, 12, 0 }, 18},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 0, 2, 3, 1, 1, 1, 3, 1, 1, 1, 0, 0
        , 1, 1, 1, 0, 1, 0, 1, 0, 16, 14, 5, 6, 7, 3, 15, 11, 12, 0 }, 17},
    { { 1, 1, 1, 1, 1, 2, 0, 2, 0, 3, 1, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 0, 1, 1, 1, 1, 0, 0, 1, 16, 14, 5, 6, 4, 12, 15, 11, 12, 0 }, 18},
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 1, 1, 16, 14, 5, 6, 4, 3, 15, 11, 12, 0 }, 18},
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 1, 1, 16, 14, 5, 6, 4, 3, 15, 11, 12, 0 }, 17},
    { { 0, 1, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 1, 1, 16, 14, 5, 6, 4, 3, 15, 11, 12, 0 }, 17}
};
int lm_cut_test_len = sizeof(lm_cut_test) / sizeof(lm_cut_test_t);

TEST(testHeurRelaxLMCut)
{
    plan_problem_t *p;
    plan_heur_t *heur;
    plan_state_t *state;
    plan_cost_t h;
    int i, var;

    p = planProblemFromFD("../data/ma-benchmarks/depot/pfile5.sas");
    heur = planHeurLMCutNew(p->var, p->var_size, p->goal,
                            p->op, p->op_size, p->succ_gen);

    state = planStateNew(p->state_pool);

    for (i = 0; i < lm_cut_test_len; ++i){
        for (var = 0; var < 38; ++var)
            planStateSet(state, var, lm_cut_test[i].state[var]);
        h = planHeur(heur, state, NULL);
        assertEquals(h, lm_cut_test[i].heur);
    }

    planStateDel(state);
    planHeurDel(heur);
    planProblemDel(p);
}
