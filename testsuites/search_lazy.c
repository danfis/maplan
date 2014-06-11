#include <cu/cu.h>
#include "plan/search_lazy.h"
#include "plan/heur_goalcount.h"

TEST(testSearchLazy)
{
    plan_problem_t *prob;
    plan_search_lazy_t *lazy;
    plan_heur_t *heur;
    plan_path_t path;

    //prob = planProblemFromJson("load-from-file.in2.json");
    prob = planProblemFromJson("../data/ma-benchmarks/depot/pfile1.json");

    heur = planHeurGoalCountNew(prob->goal);
    lazy = planSearchLazyNew(prob, heur);
    planPathInit(&path);

    assertEquals(planSearchLazyRun(lazy, &path), 0);
    planPathPrint(&path, stdout);
    assertEquals(planPathCost(&path), 36);

    planPathFree(&path);
    planHeurDel(heur);
    planSearchLazyDel(lazy);
    planProblemDel(prob);
}
