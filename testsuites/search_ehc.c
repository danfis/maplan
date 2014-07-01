#include <cu/cu.h>
#include "plan/search.h"

TEST(testSearchEHC)
{
    plan_search_ehc_params_t params;
    plan_search_t *ehc;
    plan_path_t path;

    planSearchEHCParamsInit(&params);

    params.search.prob = planProblemFromJson("load-from-file.in2.json");
    params.heur = planHeurGoalCountNew(params.search.prob->goal);
    ehc = planSearchEHCNew(&params);
    planPathInit(&path);

    assertEquals(planSearchRun(ehc, &path), PLAN_SEARCH_FOUND);
    planPathPrint(&path, stdout);
    assertEquals(planPathCost(&path), 38);

    planPathFree(&path);
    planHeurDel(params.heur);
    planSearchDel(ehc);
    planProblemDel(params.search.prob);
}
