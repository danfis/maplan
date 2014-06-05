#include <cu/cu.h>
#include "plan/search_ehc.h"

TEST(testSearchEHC)
{
    plan_t *plan;
    plan_search_ehc_t *ehc;
    int res;

    plan = planNew();
    res = planLoadFromJsonFile(plan, "load-from-file.in2.json");
    assertEquals(res, 0);

    ehc = planSearchEHCNew(plan);
    planSearchEHCRun(ehc);

    planSearchEHCDel(ehc);
    planDel(plan);
}
