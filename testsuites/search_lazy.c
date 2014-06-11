#include <cu/cu.h>
#include "plan/search_lazy.h"
#include "plan/heur_goalcount.h"
#include "plan/list_lazy.h"

TEST(testSearchLazy)
{
    plan_problem_t *prob;
    plan_search_lazy_t *lazy;
    plan_heur_t *heur;
    plan_list_lazy_t *list;
    plan_path_t path;

    //prob = planProblemFromJson("load-from-file.in2.json");
    prob = planProblemFromJson("../data/ma-benchmarks/depot/pfile1.json");

    heur = planHeurGoalCountNew(prob->goal);
    list = planListLazyHeapNew();
    lazy = planSearchLazyNew(prob, heur, list);
    planPathInit(&path);

    assertEquals(planSearchLazyRun(lazy, &path), 0);
    planPathPrint(&path, stdout);
    assertEquals(planPathCost(&path), 36);

    planPathFree(&path);
    planListLazyDel(list);
    planHeurDel(heur);
    planSearchLazyDel(lazy);
    planProblemDel(prob);
}
