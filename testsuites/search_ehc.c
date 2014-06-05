#include <cu/cu.h>
#include "plan/search_ehc.h"

TEST(testSearchEHC)
{
    plan_t *plan;
    plan_search_ehc_t *ehc;
    plan_path_t path;
    int res;

    plan = planNew();
    res = planLoadFromJsonFile(plan, "load-from-file.in2.json");
    assertEquals(res, 0);

    ehc = planSearchEHCNew(plan);
    planPathInit(&path);

    assertEquals(planSearchEHCRun(ehc, &path), 0);
    planPathPrint(&path, stdout);
    assertEquals(planPathCost(&path), 28);

    planPathFree(&path);
    planSearchEHCDel(ehc);
    planDel(plan);
}
