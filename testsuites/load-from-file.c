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
    planDel(plan);
}
