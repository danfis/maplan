#include <cu/cu.h>
#include "plan/plan.h"


TEST(testLoadFromFile)
{
    plan_t *plan;
    plan_state_t *initial_state;
    int res;

    plan = planNew();
    res = planLoadFromJsonFile(plan, "load-from-file.in1.json");
    assertEquals(res, 0);

    assertEquals(plan->var_size, 9);
    assertEquals(plan->var[0].range, 5);
    assertEquals(plan->var[1].range, 2);
    assertEquals(plan->var[2].range, 2);
    assertEquals(plan->var[3].range, 2);
    assertEquals(plan->var[4].range, 2);
    assertEquals(plan->var[5].range, 2);
    assertEquals(plan->var[6].range, 5);
    assertEquals(plan->var[7].range, 5);
    assertEquals(plan->var[8].range, 5);

    initial_state = planStateNew(plan->state_pool);
    planStatePoolGetState(plan->state_pool, 0, initial_state);

    assertEquals(planStateGet(initial_state, 0), 2);
    assertEquals(planStateGet(initial_state, 1), 0);
    assertEquals(planStateGet(initial_state, 2), 1);
    assertEquals(planStateGet(initial_state, 3), 1);
    assertEquals(planStateGet(initial_state, 4), 1);
    assertEquals(planStateGet(initial_state, 5), 0);
    assertEquals(planStateGet(initial_state, 6), 3);
    assertEquals(planStateGet(initial_state, 7), 1);
    assertEquals(planStateGet(initial_state, 8), 4);

    planStateDel(plan->state_pool, initial_state);

    assertEquals(planPartStateGet(plan->goal, 0), 0);
    assertEquals(planPartStateGet(plan->goal, 1), 0);
    assertEquals(planPartStateGet(plan->goal, 2), 0);
    assertEquals(planPartStateGet(plan->goal, 3), 0);
    assertEquals(planPartStateGet(plan->goal, 4), 0);
    assertEquals(planPartStateGet(plan->goal, 5), 0);
    assertEquals(planPartStateGet(plan->goal, 6), 1);
    assertEquals(planPartStateGet(plan->goal, 7), 1);
    assertEquals(planPartStateGet(plan->goal, 8), 3);

    assertFalse(planPartStateIsSet(plan->goal, 0));
    assertFalse(planPartStateIsSet(plan->goal, 1));
    assertFalse(planPartStateIsSet(plan->goal, 2));
    assertFalse(planPartStateIsSet(plan->goal, 3));
    assertFalse(planPartStateIsSet(plan->goal, 4));
    assertFalse(planPartStateIsSet(plan->goal, 5));
    assertTrue(planPartStateIsSet(plan->goal, 6));
    assertTrue(planPartStateIsSet(plan->goal, 7));
    assertTrue(planPartStateIsSet(plan->goal, 8));

    assertEquals(plan->op_size, 32);
    assertEquals(plan->op[0].cost, 1);
    assertEquals(strcmp(plan->op[0].name, "pick-up a"), 0);
    assertEquals(planPartStateGet(plan->op[0].eff, 0), 0);
    assertEquals(planPartStateGet(plan->op[0].eff, 2), 1);
    assertEquals(planPartStateGet(plan->op[0].eff, 5), 1);
    assertEquals(planPartStateGet(plan->op[0].eff, 6), 0);
    assertEquals(planPartStateGet(plan->op[0].pre, 0), 0);
    assertEquals(planPartStateGet(plan->op[0].pre, 2), 0);
    assertEquals(planPartStateGet(plan->op[0].pre, 5), 0);
    assertEquals(planPartStateGet(plan->op[0].pre, 6), 4);
    assertEquals(planPartStateGet(plan->op[5].eff, 0), 4);
    assertTrue(planPartStateIsSet(plan->op[5].eff, 0));
    assertEquals(planPartStateGet(plan->op[5].eff, 1), 0);
    assertTrue(planPartStateIsSet(plan->op[5].eff, 1));
    assertEquals(planPartStateGet(plan->op[5].eff, 5), 0);
    assertTrue(planPartStateIsSet(plan->op[5].eff, 5));
    assertEquals(planPartStateGet(plan->op[5].eff, 6), 0);
    assertFalse(planPartStateIsSet(plan->op[5].eff, 6));
    assertEquals(planPartStateGet(plan->op[5].pre, 0), 0);
    assertTrue(planPartStateIsSet(plan->op[5].pre, 0));
    assertEquals(planPartStateGet(plan->op[5].pre, 1), 0);
    assertFalse(planPartStateIsSet(plan->op[5].pre, 1));
    assertEquals(planPartStateGet(plan->op[5].pre, 5), 0);
    assertFalse(planPartStateIsSet(plan->op[5].pre, 5));
    assertEquals(planPartStateGet(plan->op[5].pre, 6), 0);
    assertFalse(planPartStateIsSet(plan->op[5].pre, 6));

    planDel(plan);
}
