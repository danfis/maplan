#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/heur.h"
#include "plan/search.h"
#include "state_pool.h"

static void runAStar(const char *proto, unsigned flags,
                     int expected_cost)
{
    plan_search_astar_params_t params;
    plan_search_t *search;
    plan_problem_t *p;
    plan_path_t path;
    plan_state_t *state;
    int cost;

    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->state_pool->num_vars);
    planStatePoolGetState(p->state_pool, p->initial_state, state);

    planSearchAStarParamsInit(&params);
    params.search.heur = planHeurPotentialNew(p, state, flags);
    //params.search.heur = planHeurLMCutNew(p->var, p->var_size, p->goal,
    //                                      p->op, p->op_size, flags);
    planStateDel(state);
    params.search.heur_del = 1;
    params.search.prob = p;
    //params.search.progress.fn = stopSearch;
    //params.search.progress.freq = max_steps;
    search = planSearchAStarNew(&params);

    planPathInit(&path);
    planSearchRun(search, &path);
    cost = planPathCost(&path);
    planPathFree(&path);

    planSearchDel(search);
    planProblemDel(p);

    if (cost != expected_cost)
        fprintf(stderr, "%s: cost: %d, expected cost: %d\n",
                proto, cost, expected_cost);
    assertEquals(cost, expected_cost);
}

TEST(testHeurPotential)
{
    runAStar("proto/simple.proto", 0, 10);
    runAStar("proto/depot-pfile1.proto", 0, 10);
    runAStar("proto/depot-pfile2.proto", 0, 15);
    runAStar("proto/driverlog-pfile1.proto", 0, 7);
    runAStar("proto/driverlog-pfile3.proto", 0, 12);
    runAStar("proto/rovers-p01.proto", 0, 10);
    runAStar("proto/rovers-p02.proto", 0, 8);
    runAStar("proto/rovers-p03.proto", 0, 11);
    runAStar("proto/sokoban-p01.proto", 0, 9);
}
