#include <cu/cu.h>
#include <plan/search.h>

TEST(testSearchAStar)
{
    plan_search_astar_params_t params;
    plan_search_t *search;
    plan_path_t path;
    plan_problem_t *p;

    printf("../data/ma-benchmarks/driverlog/pfile3.sas");
    planSearchAStarParamsInit(&params);
    p = planProblemFromFD("../data/ma-benchmarks/driverlog/pfile3.sas");
    params.search.prob = p;
    params.heur = planHeurLMCutNew(p->var, p->var_size, p->goal,
                                   p->op, p->op_size, p->succ_gen);
    params.heur_del = 1;
    search = planSearchAStarNew(&params);

    planPathInit(&path);
    assertEquals(planSearchRun(search, &path), PLAN_SEARCH_FOUND);
    planPathPrint(&path, stdout);
    assertEquals(planPathCost(&path), 12);

    planPathFree(&path);
    planSearchDel(search);
    planProblemDel(p);

    printf("../data/ma-benchmarks/depot/pfile2.sas");
    planSearchAStarParamsInit(&params);
    p = planProblemFromFD("../data/ma-benchmarks/depot/pfile2.sas");
    params.search.prob = p;
    params.heur = planHeurLMCutNew(p->var, p->var_size, p->goal,
                                   p->op, p->op_size, p->succ_gen);
    params.heur_del = 1;
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
