#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/heur.h"
#include "state_pool.h"

typedef plan_heur_t *(*new_heur_fn)(plan_problem_t *p);

static plan_heur_t *lmCutNew(plan_problem_t *p)
{
    return planHeurLMCutNew(p, 0);
}

static plan_heur_t *flowBaseNew(plan_problem_t *p)
{
    return planHeurFlowNew(p, 0);
}

static void _runTest(const char *name, const char *proto,
                     const char *states, int flags,
                     new_heur_fn check_min_heur_fn)
{
    plan_problem_t *p;
    state_pool_t state_pool;
    plan_state_t *state;
    plan_heur_t *heur, *heur_check_min = NULL;
    plan_heur_res_t res, res_check_min;
    int i, si;

    printf("-----\n%s\n%s\n", name, proto);
    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->state_pool->num_vars);
    statePoolInit(&state_pool, states);

    heur = planHeurFlowNew(p, flags);
    if (heur == NULL){
        fprintf(stderr, "Test Error: Cannot create a heuristic object!\n");
        goto run_test_end;
    }

    if (check_min_heur_fn != NULL)
        heur_check_min = check_min_heur_fn(p);

    for (si = 0; statePoolNext(&state_pool, state) == 0; ++si){
        //if (si != 7644)
        //    continue;

        planHeurResInit(&res);
        planHeurState(heur, state, &res);

        if (heur_check_min){
            planHeurResInit(&res_check_min);
            planHeurState(heur_check_min, state, &res_check_min);

            assertTrue(res_check_min.heur <= res.heur);
            if (!(res_check_min.heur <= res.heur)){
                printf("%d: %d > %d!\n", si, res_check_min.heur, res.heur);
            }

        }else{
            printf("[%d] %d ::", si, res.heur);
            for (i = 0; i < planStateSize(state); ++i){
                printf(" %d", planStateGet(state, i));
            }
            printf("\n");
            fflush(stdout);
        }
    }

run_test_end:
    if (heur)
        planHeurDel(heur);
    if (heur_check_min)
        planHeurDel(heur_check_min);
    statePoolFree(&state_pool);
    planStateDel(state);
    planProblemDel(p);
    printf("-----\n");
}

static void runTest(const char *name, const char *proto,
                    const char *states, int flags)
{
    _runTest(name, proto, states, flags, NULL);
}

static void runTestCheckMin(const char *name, const char *proto,
                            const char *states, int flags,
                            new_heur_fn fn)
{
    _runTest(name, proto, states, flags, fn);
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

TEST(testHeurFlowLandmarks)
{
    int flags;
    
    flags = PLAN_HEUR_FLOW_LANDMARKS_LM_CUT;
    runTest("Flow Landmarks", "proto/simple.proto",
            "states/simple.txt", flags);
    runTest("Flow Landmarks", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", flags);
    runTest("Flow Landmarks", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", flags);
    runTest("Flow Landmarks", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", flags);
    runTest("Flow Landmarks", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", flags);
    runTest("Flow Landmarks", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", flags);

    runTestCheckMin("Flow Landmarks Check Flow-Base", "proto/simple.proto",
                    "states/simple.txt", flags, flowBaseNew);
    runTestCheckMin("Flow Landmarks Check Flow-Base", "proto/depot-pfile1.proto",
                    "states/depot-pfile1.txt", flags, flowBaseNew);
    runTestCheckMin("Flow Landmarks Check Flow-Base", "proto/depot-pfile5.proto",
                    "states/depot-pfile5.txt", flags, flowBaseNew);
    runTestCheckMin("Flow Landmarks Check Flow-Base", "proto/rovers-p03.proto",
                    "states/rovers-p03.txt", flags, flowBaseNew);
    runTestCheckMin("Flow Landmarks Check Flow-Base", "proto/rovers-p15.proto",
                    "states/rovers-p15.txt", flags, flowBaseNew);
    runTestCheckMin("Flow Landmarks Check Flow-Base", "proto/CityCar-p3-2-2-0-1.proto",
                    "states/citycar-p3-2-2-0-1.txt", flags, flowBaseNew);

    flags = PLAN_HEUR_FLOW_LANDMARKS_LM_CUT;// | PLAN_HEUR_FLOW_ILP;
    runTestCheckMin("Flow Landmarks Check LM-Cut", "proto/simple.proto",
                    "states/simple.txt", flags, lmCutNew);
    runTestCheckMin("Flow Landmarks Check LM-Cut", "proto/depot-pfile1.proto",
                    "states/depot-pfile1.txt", flags, lmCutNew);
    runTestCheckMin("Flow Landmarks Check LM-Cut", "proto/depot-pfile5.proto",
                    "states/depot-pfile5.txt", flags, lmCutNew);
    runTestCheckMin("Flow Landmarks Check LM-Cut", "proto/rovers-p03.proto",
                    "states/rovers-p03.txt", flags, lmCutNew);
    runTestCheckMin("Flow Landmarks Check LM-Cut", "proto/rovers-p15.proto",
                    "states/rovers-p15.txt", flags, lmCutNew);
    runTestCheckMin("Flow Landmarks Check LM-Cut", "proto/CityCar-p3-2-2-0-1.proto",
                    "states/citycar-p3-2-2-0-1.txt", flags, lmCutNew);
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

TEST(testHeurFlowFAMutex)
{
    unsigned flags;

    flags = PLAN_HEUR_FLOW_FA_MUTEX;
    runTest("Flow", "proto/simple.proto",
            "states/simple.txt", flags);
    runTest("Flow", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", flags);
    runTest("Flow", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", flags);
    runTest("Flow", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", flags);
    runTest("Flow", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", flags);
    runTest("Flow", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", flags);
}
