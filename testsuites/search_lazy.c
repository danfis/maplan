#include <cu/cu.h>
#include "plan/search.h"

#define DEF_JSON "../data/ma-benchmarks/depot/pfile1.sas"

TEST(testSearchLazy)
{
    plan_search_lazy_params_t params;
    plan_search_t *lazy;
    plan_path_t path;

    planSearchLazyParamsInit(&params);

    params.search.prob = planProblemFromFD(DEF_JSON);

    params.heur = planHeurGoalCountNew(params.search.prob->goal);
    params.list = planListLazyHeapNew();
    lazy = planSearchLazyNew(&params);
    planPathInit(&path);

    assertEquals(planSearchRun(lazy, &path), PLAN_SEARCH_FOUND);
    planPathPrint(&path, stdout);
    assertEquals(planPathCost(&path), 11);

    planPathFree(&path);
    planListLazyDel(params.list);
    planHeurDel(params.heur);
    planSearchDel(lazy);
    planProblemDel(params.search.prob);
}
