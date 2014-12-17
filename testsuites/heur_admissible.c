#include <cu/cu.h>
#include "plan/search.h"

#include "state_pool.h"

static void checkOptimalCost(const char *proto)
{
    plan_search_astar_params_t params;
    plan_search_t *search;
    plan_path_t path;
    plan_problem_t *p;
    plan_path_op_t *op;
    plan_cost_t cost;
    plan_state_space_node_t *node;

    planSearchAStarParamsInit(&params);
    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    params.search.prob = p;
    params.search.heur = planHeurLMCutNew(p->var, p->var_size, p->goal,
                                          p->op, p->op_size, p->succ_gen);
    params.search.heur_del = 1;
    search = planSearchAStarNew(&params);

    planPathInit(&path);
    assertEquals(planSearchRun(search, &path), PLAN_SEARCH_FOUND);

    cost = planPathCost(&path);
    BOR_LIST_FOR_EACH_ENTRY(&path, plan_path_op_t, op, path){
        node = planStateSpaceNode(search->state_space, op->from_state);
        assertTrue(node->heuristic <= cost);
        node = planStateSpaceNode(search->state_space, op->to_state);
        assertTrue(node->heuristic - op->cost <= cost);
        cost -= op->cost;
    }

    planPathFree(&path);
    planSearchDel(search);
    planProblemDel(p);
}

TEST(testHeurAdmissibleLMCut)
{
    checkOptimalCost("../data/ma-benchmarks/depot/pfile1.proto");
    checkOptimalCost("../data/ma-benchmarks/depot/pfile2.proto");
    checkOptimalCost("../data/ma-benchmarks/rovers/p01.proto");
    checkOptimalCost("../data/ma-benchmarks/rovers/p02.proto");
    checkOptimalCost("../data/ma-benchmarks/rovers/p03.proto");
    //checkOptimalCost("../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto");
}
