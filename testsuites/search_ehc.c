#include <cu/cu.h>
#include "plan/search_ehc.h"
#include "plan/heur_goalcount.h"

TEST(testSearchEHC)
{
    plan_problem_t *prob;
    plan_search_ehc_t *ehc;
    plan_heur_t *heur;
    plan_path_t path;

    prob = planProblemFromJson("load-from-file.in2.json");

    heur = planHeurGoalCountNew(prob->goal);
    ehc = planSearchEHCNew(prob, heur);
    planPathInit(&path);

    assertEquals(planSearchEHCRun(ehc, &path), 0);
    planPathPrint(&path, stdout);
    assertEquals(planPathCost(&path), 36);

    planPathFree(&path);
    planHeurDel(heur);
    planSearchEHCDel(ehc);
    planProblemDel(prob);
}
