#include <cu/cu.h>
#include <plan/search.h>

TEST(testSearchAStar)
{
    plan_search_astar_params_t params;
    plan_search_t *search;
    plan_path_t path;
    plan_problem_t *p;

    printf("proto/driverlog-pfile3.proto\n");
    planSearchAStarParamsInit(&params);
    p = planProblemFromProto("proto/driverlog-pfile3.proto",
                             PLAN_PROBLEM_USE_CG);
    params.search.prob = p;
    params.search.heur = planHeurLMCutNew(p, 0);
    params.search.heur_del = 1;
    search = planSearchAStarNew(&params);

    planPathInit(&path);
    assertEquals(planSearchRun(search, &path), PLAN_SEARCH_FOUND);
    planPathPrint(&path, stdout);
    assertEquals(planPathCost(&path), 12);

    planPathFree(&path);
    planSearchDel(search);
    planProblemDel(p);

    printf("proto/depot-pfile2.proto\n");
    planSearchAStarParamsInit(&params);
    p = planProblemFromProto("proto/depot-pfile2.proto",
                             PLAN_PROBLEM_USE_CG);
    params.search.prob = p;
    params.search.heur = planHeurLMCutNew(p, 0);
    params.search.heur_del = 1;
    params.pathmax = 1;
    search = planSearchAStarNew(&params);

    planPathInit(&path);
    assertEquals(planSearchRun(search, &path), PLAN_SEARCH_FOUND);
    planPathPrint(&path, stdout);
    assertEquals(planPathCost(&path), 15);

    planPathFree(&path);
    planSearchDel(search);
    planProblemDel(p);
}
