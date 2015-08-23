#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/heur.h"
#include "state_pool.h"

static void _runTest(const char *name, const char *proto,
                     const char *states, int flags)
{
    plan_problem_t *p;
    state_pool_t state_pool;
    plan_state_t *state;
    plan_heur_t *heur, *heur_check_min = NULL;
    plan_heur_res_t res, res_check_min;
    plan_state_t *init_state;
    int i, si;

    printf("-----\n%s\n%s\n", name, proto);
    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->state_pool->num_vars);
    statePoolInit(&state_pool, states);

    init_state = planStateNew(p->state_pool->num_vars);
    planStatePoolGetState(p->state_pool, p->initial_state, init_state);
    heur = planHeurPotentialNew(p->var, p->var_size, p->goal,
                                p->op, p->op_size, init_state, flags);
    planStateDel(init_state);
    if (heur == NULL){
        fprintf(stderr, "Test Error: Cannot create a heuristic object!\n");
        goto run_test_end;
    }

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
    _runTest(name, proto, states, flags);
}



TEST(testHeurPotential)
{
    runTest("Potential", "proto/simple.proto",
            "states/simple.txt", 0);
    runTest("Potential", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", 0);
    runTest("Potential", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", 0);
    runTest("Potential", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", 0);
    runTest("Potential", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", 0);
    runTest("Potential", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", 0);
}
