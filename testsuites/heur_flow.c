#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/heur.h"
#include "state_pool.h"

static void runTest(const char *name, const char *proto,
                    const char *states, int flags)
{
    plan_problem_t *p;
    state_pool_t state_pool;
    plan_state_t *state;
    plan_heur_t *heur;
    plan_heur_res_t res;
    int i, si;

    printf("-----\n%s\n%s\n", name, proto);
    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->state_pool->num_vars);
    statePoolInit(&state_pool, states);

    heur = planHeurFlowNew(p->var, p->var_size, p->goal,
                           p->op, p->op_size, flags);
    if (heur == NULL){
        fprintf(stderr, "Test Error: Cannot create a heuristic object!\n");
        goto run_test_end;
    }

    for (si = 0; statePoolNext(&state_pool, state) == 0; ++si){
        //if (si != 4)
        //    continue;
        planHeurResInit(&res);

        planHeur(heur, state, &res);
        printf("[%d] %d ::", si, res.heur);
        for (i = 0; i < planStateSize(state); ++i){
            printf(" %d", planStateGet(state, i));
        }
        printf("\n");
        fflush(stdout);
    }

run_test_end:
    if (heur)
        planHeurDel(heur);
    statePoolFree(&state_pool);
    planStateDel(state);
    planProblemDel(p);
    printf("-----\n");
}


TEST(testHeurFlow)
{
    runTest("Flow", "proto/simple.proto",
            "states/simple.txt", 0);
    runTest("Flow", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", 0);
    runTest("Flow", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", 0);
    runTest("Flow", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", 0);
    runTest("Flow", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", 0);
    runTest("Flow", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", 0);
}

TEST(testHeurFlowILP)
{
    int flags;
    
    flags = PLAN_HEUR_FLOW_ILP;
    runTest("Flow ILP", "proto/simple.proto",
            "states/simple.txt", flags);
    runTest("Flow ILP", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", flags);
    runTest("Flow ILP", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", flags);
    runTest("Flow ILP", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", flags);
    runTest("Flow ILP", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", flags);
    runTest("Flow ILP", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", flags);
}
