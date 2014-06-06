#include <cu/cu.h>
#include "plan/search_ehc.h"

TEST(testSearchEHC)
{
    plan_problem_t *prob;
    plan_search_ehc_t *ehc;
    plan_path_t path;

    prob = planProblemFromJson("load-from-file.in2.json");

    ehc = planSearchEHCNew(prob);
    planPathInit(&path);

    assertEquals(planSearchEHCRun(ehc, &path), 0);
    planPathPrint(&path, stdout);
    assertEquals(planPathCost(&path), 36);

    planPathFree(&path);
    planSearchEHCDel(ehc);
    planProblemDel(prob);
}
