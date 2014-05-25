#include <cu/cu.h>
#include "plan/plan.h"


TEST(testLoadFromFile)
{
    plan_t *plan;
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

    assertEquals(planStateGet(&plan->initial_state, 0), 3);
    assertEquals(planStateGet(&plan->initial_state, 1), 1);
    assertEquals(planStateGet(&plan->initial_state, 2), 2);
    assertEquals(planStateGet(&plan->initial_state, 3), 2);
    assertEquals(planStateGet(&plan->initial_state, 4), 2);
    assertEquals(planStateGet(&plan->initial_state, 5), 1);
    assertEquals(planStateGet(&plan->initial_state, 6), 4);
    assertEquals(planStateGet(&plan->initial_state, 7), 2);
    assertEquals(planStateGet(&plan->initial_state, 8), 5);

    assertEquals(planPartStateGet(&plan->goal, 0), 0);
    assertEquals(planPartStateGet(&plan->goal, 1), 0);
    assertEquals(planPartStateGet(&plan->goal, 2), 0);
    assertEquals(planPartStateGet(&plan->goal, 3), 0);
    assertEquals(planPartStateGet(&plan->goal, 4), 0);
    assertEquals(planPartStateGet(&plan->goal, 5), 0);
    assertEquals(planPartStateGet(&plan->goal, 6), 2);
    assertEquals(planPartStateGet(&plan->goal, 7), 2);
    assertEquals(planPartStateGet(&plan->goal, 8), 4);
    assertEquals(plan->goal.mask[0], 0);
    assertEquals(plan->goal.mask[8], ~0);

    planDel(plan);
}
