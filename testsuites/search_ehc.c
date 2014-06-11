#include <cu/cu.h>
#include "plan/search_ehc.h"
#include "plan/heur_goalcount.h"

TEST(testSearchEHC)
{
    plan_search_ehc_params_t params;
    plan_search_ehc_t *ehc;
    plan_path_t path;

    planSearchEHCParamsInit(&params);

    params.prob = planProblemFromJson("load-from-file.in2.json");

    params.heur = planHeurGoalCountNew(params.prob->goal);
    ehc = planSearchEHCNew(&params);
    planPathInit(&path);

    assertEquals(planSearchEHCRun(ehc, &path), 0);
    planPathPrint(&path, stdout);
    assertEquals(planPathCost(&path), 36);

    planPathFree(&path);
    planHeurDel(params.heur);
    planSearchEHCDel(ehc);
    planProblemDel(params.prob);
}
