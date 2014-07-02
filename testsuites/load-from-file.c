#include <cu/cu.h>
#include "plan/problem.h"


TEST(testLoadFromFile)
{
    plan_problem_t *p;
    plan_state_t *initial_state;

    p = planProblemFromJson("load-from-file.in1.json");
    assertNotEquals(p, NULL);

    assertEquals(p->var_size, 9);
    assertEquals(p->var[0].range, 5);
    assertEquals(p->var[1].range, 2);
    assertEquals(p->var[2].range, 2);
    assertEquals(p->var[3].range, 2);
    assertEquals(p->var[4].range, 2);
    assertEquals(p->var[5].range, 2);
    assertEquals(p->var[6].range, 5);
    assertEquals(p->var[7].range, 5);
    assertEquals(p->var[8].range, 5);

    initial_state = planStateNew(p->state_pool);
    planStatePoolGetState(p->state_pool, 0, initial_state);

    assertEquals(planStateGet(initial_state, 0), 2);
    assertEquals(planStateGet(initial_state, 1), 0);
    assertEquals(planStateGet(initial_state, 2), 1);
    assertEquals(planStateGet(initial_state, 3), 1);
    assertEquals(planStateGet(initial_state, 4), 1);
    assertEquals(planStateGet(initial_state, 5), 0);
    assertEquals(planStateGet(initial_state, 6), 3);
    assertEquals(planStateGet(initial_state, 7), 1);
    assertEquals(planStateGet(initial_state, 8), 4);

    planStateDel(p->state_pool, initial_state);

    assertEquals(planPartStateGet(p->goal, 0), 0);
    assertEquals(planPartStateGet(p->goal, 1), 0);
    assertEquals(planPartStateGet(p->goal, 2), 0);
    assertEquals(planPartStateGet(p->goal, 3), 0);
    assertEquals(planPartStateGet(p->goal, 4), 0);
    assertEquals(planPartStateGet(p->goal, 5), 0);
    assertEquals(planPartStateGet(p->goal, 6), 1);
    assertEquals(planPartStateGet(p->goal, 7), 1);
    assertEquals(planPartStateGet(p->goal, 8), 3);

    assertFalse(planPartStateIsSet(p->goal, 0));
    assertFalse(planPartStateIsSet(p->goal, 1));
    assertFalse(planPartStateIsSet(p->goal, 2));
    assertFalse(planPartStateIsSet(p->goal, 3));
    assertFalse(planPartStateIsSet(p->goal, 4));
    assertFalse(planPartStateIsSet(p->goal, 5));
    assertTrue(planPartStateIsSet(p->goal, 6));
    assertTrue(planPartStateIsSet(p->goal, 7));
    assertTrue(planPartStateIsSet(p->goal, 8));

    assertEquals(p->op_size, 32);
    assertEquals(p->op[0].cost, 1);
    assertEquals(strcmp(p->op[0].name, "pick-up a"), 0);
    assertEquals(planPartStateGet(p->op[0].eff, 0), 0);
    assertEquals(planPartStateGet(p->op[0].eff, 2), 1);
    assertEquals(planPartStateGet(p->op[0].eff, 5), 1);
    assertEquals(planPartStateGet(p->op[0].eff, 6), 0);
    assertEquals(planPartStateGet(p->op[0].pre, 0), 0);
    assertEquals(planPartStateGet(p->op[0].pre, 2), 0);
    assertEquals(planPartStateGet(p->op[0].pre, 5), 0);
    assertEquals(planPartStateGet(p->op[0].pre, 6), 4);
    assertEquals(planPartStateGet(p->op[5].eff, 0), 4);
    assertTrue(planPartStateIsSet(p->op[5].eff, 0));
    assertEquals(planPartStateGet(p->op[5].eff, 1), 0);
    assertTrue(planPartStateIsSet(p->op[5].eff, 1));
    assertEquals(planPartStateGet(p->op[5].eff, 5), 0);
    assertTrue(planPartStateIsSet(p->op[5].eff, 5));
    assertEquals(planPartStateGet(p->op[5].eff, 6), 0);
    assertFalse(planPartStateIsSet(p->op[5].eff, 6));
    assertEquals(planPartStateGet(p->op[5].pre, 0), 0);
    assertTrue(planPartStateIsSet(p->op[5].pre, 0));
    assertEquals(planPartStateGet(p->op[5].pre, 1), 0);
    assertFalse(planPartStateIsSet(p->op[5].pre, 1));
    assertEquals(planPartStateGet(p->op[5].pre, 5), 0);
    assertFalse(planPartStateIsSet(p->op[5].pre, 5));
    assertEquals(planPartStateGet(p->op[5].pre, 6), 0);
    assertFalse(planPartStateIsSet(p->op[5].pre, 6));

    planProblemDel(p);
}

TEST(testLoadFromFD)
{
    plan_problem_t *p;
    plan_state_t *initial_state;

    p = planProblemFromFD("load-from-file.in1.txt");
    assertNotEquals(p, NULL);
    if (p == NULL)
        return;


    assertEquals(p->var_size, 9);
    assertEquals(p->var[0].range, 5);
    assertEquals(p->var[1].range, 2);
    assertEquals(p->var[2].range, 2);
    assertEquals(p->var[3].range, 2);
    assertEquals(p->var[4].range, 2);
    assertEquals(p->var[5].range, 2);
    assertEquals(p->var[6].range, 5);
    assertEquals(p->var[7].range, 5);
    assertEquals(p->var[8].range, 5);

    initial_state = planStateNew(p->state_pool);
    planStatePoolGetState(p->state_pool, 0, initial_state);

    assertEquals(planStateGet(initial_state, 0), 2);
    assertEquals(planStateGet(initial_state, 1), 0);
    assertEquals(planStateGet(initial_state, 2), 1);
    assertEquals(planStateGet(initial_state, 3), 1);
    assertEquals(planStateGet(initial_state, 4), 1);
    assertEquals(planStateGet(initial_state, 5), 0);
    assertEquals(planStateGet(initial_state, 6), 3);
    assertEquals(planStateGet(initial_state, 7), 1);
    assertEquals(planStateGet(initial_state, 8), 4);

    planStateDel(p->state_pool, initial_state);

    assertEquals(planPartStateGet(p->goal, 0), 0);
    assertEquals(planPartStateGet(p->goal, 1), 0);
    assertEquals(planPartStateGet(p->goal, 2), 0);
    assertEquals(planPartStateGet(p->goal, 3), 0);
    assertEquals(planPartStateGet(p->goal, 4), 0);
    assertEquals(planPartStateGet(p->goal, 5), 0);
    assertEquals(planPartStateGet(p->goal, 6), 1);
    assertEquals(planPartStateGet(p->goal, 7), 1);
    assertEquals(planPartStateGet(p->goal, 8), 3);

    assertFalse(planPartStateIsSet(p->goal, 0));
    assertFalse(planPartStateIsSet(p->goal, 1));
    assertFalse(planPartStateIsSet(p->goal, 2));
    assertFalse(planPartStateIsSet(p->goal, 3));
    assertFalse(planPartStateIsSet(p->goal, 4));
    assertFalse(planPartStateIsSet(p->goal, 5));
    assertTrue(planPartStateIsSet(p->goal, 6));
    assertTrue(planPartStateIsSet(p->goal, 7));
    assertTrue(planPartStateIsSet(p->goal, 8));

    assertEquals(p->op_size, 32);
    assertEquals(p->op[0].cost, 1);
    assertEquals(strcmp(p->op[0].name, "pick-up a"), 0);
    assertEquals(planPartStateGet(p->op[0].eff, 0), 0);
    assertEquals(planPartStateGet(p->op[0].eff, 2), 1);
    assertEquals(planPartStateGet(p->op[0].eff, 5), 1);
    assertEquals(planPartStateGet(p->op[0].eff, 6), 0);
    assertEquals(planPartStateGet(p->op[0].pre, 0), 0);
    assertEquals(planPartStateGet(p->op[0].pre, 2), 0);
    assertEquals(planPartStateGet(p->op[0].pre, 5), 0);
    assertEquals(planPartStateGet(p->op[0].pre, 6), 4);
    assertEquals(planPartStateGet(p->op[5].eff, 0), 4);
    assertTrue(planPartStateIsSet(p->op[5].eff, 0));
    assertEquals(planPartStateGet(p->op[5].eff, 1), 0);
    assertTrue(planPartStateIsSet(p->op[5].eff, 1));
    assertEquals(planPartStateGet(p->op[5].eff, 5), 0);
    assertTrue(planPartStateIsSet(p->op[5].eff, 5));
    assertEquals(planPartStateGet(p->op[5].eff, 6), 0);
    assertFalse(planPartStateIsSet(p->op[5].eff, 6));
    assertEquals(planPartStateGet(p->op[5].pre, 0), 0);
    assertTrue(planPartStateIsSet(p->op[5].pre, 0));
    assertEquals(planPartStateGet(p->op[5].pre, 1), 0);
    assertFalse(planPartStateIsSet(p->op[5].pre, 1));
    assertEquals(planPartStateGet(p->op[5].pre, 5), 0);
    assertFalse(planPartStateIsSet(p->op[5].pre, 5));
    assertEquals(planPartStateGet(p->op[5].pre, 6), 0);
    assertFalse(planPartStateIsSet(p->op[5].pre, 6));

    planProblemDel(p);
}
