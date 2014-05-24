#include <cu/cu.h>
#include "plan/plan.h"


TEST(testLoadFromFile)
{
    plan_t *plan;

    plan = planNew();
    planLoadFromJsonFile(plan, "load-from-file.in1.json");
    planDel(plan);
}
