#include <cu/cu.h>
#include "plan/search.h"

#include "state_pool.h"

typedef plan_heur_t *(*new_heur_fn)(const plan_problem_t *prob);

static plan_heur_t *heurLMCut(const plan_problem_t *p)
{
    return planHeurLMCutNew(p->var, p->var_size, p->goal, p->op, p->op_size, 0);
}

static plan_heur_t *heurMax(const plan_problem_t *p)
{
    return planHeurRelaxMaxNew(p, 0);
}

static plan_heur_t *heurFlow(const plan_problem_t *p)
{
    return planHeurFlowNew(p->var, p->var_size, p->goal,
                           p->op, p->op_size, 0);
}

static plan_heur_t *heurFlowLandmarks(const plan_problem_t *p)
{
    return planHeurFlowNew(p->var, p->var_size, p->goal,
                           p->op, p->op_size,
                           PLAN_HEUR_FLOW_LANDMARKS_LM_CUT);
}

static plan_heur_t *heurPotential(const plan_problem_t *p)
{
    PLAN_STATE_STACK(init_state, p->state_pool->num_vars);
    planStatePoolGetState(p->state_pool, p->initial_state, &init_state);
    return planHeurPotentialNew(p->var, p->var_size, p->goal,
                                p->op, p->op_size, &init_state, 0);
}

static void checkOptimalCost(new_heur_fn new_heur, const char *proto)
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
    params.search.heur = new_heur(p);
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

static void checkOptimalCost2(new_heur_fn new_heur, const char *proto,
                              const char *states, const char *costs)
{
    plan_problem_t *p;
    plan_heur_t *heur;
    plan_heur_res_t res;
    state_pool_t state_pool;
    plan_state_t *state;
    FILE *fcosts;
    int cost, si;

    fcosts = fopen(costs, "r");

    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->var_size);
    statePoolInit(&state_pool, states);
    heur = new_heur(p);

    for (si = 0; statePoolNext(&state_pool, state) == 0; ++si){
        planHeurResInit(&res);
        planHeurState(heur, state, &res);
        fscanf(fcosts, "%d", &cost);
        assertTrue((int)res.heur <= cost);
    }

    planHeurDel(heur);
    statePoolFree(&state_pool);
    planStateDel(state);
    planProblemDel(p);
    fclose(fcosts);
}

TEST(testHeurAdmissibleLMCut)
{
    checkOptimalCost(heurLMCut, "proto/depot-pfile1.proto");
    checkOptimalCost(heurLMCut, "proto/depot-pfile2.proto");
    checkOptimalCost(heurLMCut, "proto/rovers-p01.proto");
    checkOptimalCost(heurLMCut, "proto/rovers-p02.proto");
    checkOptimalCost(heurLMCut, "proto/rovers-p03.proto");

    checkOptimalCost2(heurLMCut,
                      "proto/depot-pfile1.proto",
                      "states/depot-pfile1.txt",
                      "states/depot-pfile1.cost.txt");
    checkOptimalCost2(heurLMCut,
                      "proto/driverlog-pfile1.proto",
                      "states/driverlog-pfile1.txt",
                      "states/driverlog-pfile1.cost.txt");
    checkOptimalCost2(heurLMCut,
                      "proto/rovers-p03.proto",
                      "states/rovers-p03.txt",
                      "states/rovers-p03.cost.txt");
}

TEST(testHeurAdmissibleMax)
{
    checkOptimalCost(heurMax, "proto/depot-pfile1.proto");
    checkOptimalCost(heurMax, "proto/depot-pfile2.proto");
    checkOptimalCost(heurMax, "proto/rovers-p01.proto");
    checkOptimalCost(heurMax, "proto/rovers-p02.proto");
    checkOptimalCost(heurMax, "proto/rovers-p03.proto");

    checkOptimalCost2(heurMax,
                      "proto/depot-pfile1.proto",
                      "states/depot-pfile1.txt",
                      "states/depot-pfile1.cost.txt");
    checkOptimalCost2(heurMax,
                      "proto/driverlog-pfile1.proto",
                      "states/driverlog-pfile1.txt",
                      "states/driverlog-pfile1.cost.txt");
    checkOptimalCost2(heurMax,
                      "proto/rovers-p03.proto",
                      "states/rovers-p03.txt",
                      "states/rovers-p03.cost.txt");
}

TEST(testHeurAdmissibleFlow)
{
    checkOptimalCost(heurFlow, "proto/depot-pfile1.proto");
    checkOptimalCost(heurFlow, "proto/depot-pfile2.proto");
    checkOptimalCost(heurFlow, "proto/rovers-p01.proto");
    checkOptimalCost(heurFlow, "proto/rovers-p02.proto");
    checkOptimalCost(heurFlow, "proto/rovers-p03.proto");

    checkOptimalCost2(heurFlow,
                      "proto/depot-pfile1.proto",
                      "states/depot-pfile1.txt",
                      "states/depot-pfile1.cost.txt");
    checkOptimalCost2(heurFlow,
                      "proto/driverlog-pfile1.proto",
                      "states/driverlog-pfile1.txt",
                      "states/driverlog-pfile1.cost.txt");
    checkOptimalCost2(heurFlow,
                      "proto/rovers-p03.proto",
                      "states/rovers-p03.txt",
                      "states/rovers-p03.cost.txt");
}

TEST(testHeurAdmissibleFlowLandmarks)
{
    checkOptimalCost(heurFlowLandmarks, "proto/depot-pfile1.proto");
    checkOptimalCost(heurFlowLandmarks, "proto/depot-pfile2.proto");
    checkOptimalCost(heurFlowLandmarks, "proto/rovers-p01.proto");
    checkOptimalCost(heurFlowLandmarks, "proto/rovers-p02.proto");
    checkOptimalCost(heurFlowLandmarks, "proto/rovers-p03.proto");

    checkOptimalCost2(heurFlowLandmarks,
                      "proto/depot-pfile1.proto",
                      "states/depot-pfile1.txt",
                      "states/depot-pfile1.cost.txt");
    checkOptimalCost2(heurFlowLandmarks,
                      "proto/driverlog-pfile1.proto",
                      "states/driverlog-pfile1.txt",
                      "states/driverlog-pfile1.cost.txt");
    checkOptimalCost2(heurFlowLandmarks,
                      "proto/rovers-p03.proto",
                      "states/rovers-p03.txt",
                      "states/rovers-p03.cost.txt");
}

TEST(testHeurAdmissiblePotential)
{
    checkOptimalCost(heurPotential, "proto/depot-pfile1.proto");
    checkOptimalCost(heurPotential, "proto/depot-pfile2.proto");
    checkOptimalCost(heurPotential, "proto/rovers-p01.proto");
    checkOptimalCost(heurPotential, "proto/rovers-p02.proto");
    checkOptimalCost(heurPotential, "proto/rovers-p03.proto");

    checkOptimalCost2(heurPotential,
                      "proto/depot-pfile1.proto",
                      "states/depot-pfile1.txt",
                      "states/depot-pfile1.cost.txt");
    checkOptimalCost2(heurPotential,
                      "proto/driverlog-pfile1.proto",
                      "states/driverlog-pfile1.txt",
                      "states/driverlog-pfile1.cost.txt");
    checkOptimalCost2(heurPotential,
                      "proto/rovers-p03.proto",
                      "states/rovers-p03.txt",
                      "states/rovers-p03.cost.txt");
}
