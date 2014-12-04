#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/heur.h"
#include "state_pool.h"


static plan_heur_t *goalCountNew(plan_problem_t *p)
{
    return planHeurGoalCountNew(p->goal);
}

static plan_heur_t *addNew(plan_problem_t *p)
{
    return planHeurRelaxAddNew(p->var, p->var_size, p->goal,
                               p->op, p->op_size, p->succ_gen);
}

static plan_heur_t *maxNew(plan_problem_t *p)
{
    return planHeurRelaxMaxNew(p->var, p->var_size, p->goal,
                               p->op, p->op_size, p->succ_gen);
}

static plan_heur_t *ffNew(plan_problem_t *p)
{
    return planHeurRelaxFFNew(p->var, p->var_size, p->goal,
                              p->op, p->op_size, p->succ_gen);
}

static plan_heur_t *lmCutNew(plan_problem_t *p)
{
    return planHeurLMCutNew(p->var, p->var_size, p->goal,
                            p->op, p->op_size, p->succ_gen);
}

typedef plan_heur_t *(*new_heur_fn)(plan_problem_t *p);

static void runTest(const char *name,
                    const char *proto, const char *states,
                    new_heur_fn new_heur)
{
    plan_problem_t *p;
    state_pool_t state_pool;
    plan_state_t *state;
    plan_heur_t *heur;
    plan_heur_res_t res;
    int i, si;

    printf("-----\n%s\n%s\n", name, proto);
    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->state_pool);
    statePoolInit(&state_pool, states);

    heur = new_heur(p);

    for (si = 0; statePoolNext(&state_pool, state) == 0; ++si){
        planHeurResInit(&res);
        planHeur(heur, state, &res);
        printf("[%d] %d ::", si, res.heur);
        for (i = 0; i < planStateSize(state); ++i){
            printf(" %d", planStateGet(state, i));
        }
        printf("\n");
    }

    planHeurDel(heur);
    statePoolFree(&state_pool);
    planStateDel(state);
    planProblemDel(p);
    printf("-----\n");
}


TEST(testHeurGoalCount)
{
    runTest("goal-count", "../data/ma-benchmarks/depot/pfile1.proto",
            "states/depot-pfile1.txt", goalCountNew);
    runTest("goal-count", "../data/ma-benchmarks/depot/pfile5.proto",
            "states/depot-pfile5.txt", goalCountNew);
    runTest("goal-count", "../data/ma-benchmarks/rovers/p03.proto",
            "states/rovers-p03.txt", goalCountNew);
    runTest("goal-count", "../data/ma-benchmarks/rovers/p15.proto",
            "states/rovers-p15.txt", goalCountNew);
    runTest("goal-count", "../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", goalCountNew);
}

TEST(testHeurRelaxAdd)
{
    runTest("add", "../data/ma-benchmarks/depot/pfile1.proto",
            "states/depot-pfile1.txt", addNew);
    runTest("add", "../data/ma-benchmarks/depot/pfile5.proto",
            "states/depot-pfile5.txt", addNew);
    runTest("add", "../data/ma-benchmarks/rovers/p03.proto",
            "states/rovers-p03.txt", addNew);
    runTest("add", "../data/ma-benchmarks/rovers/p15.proto",
            "states/rovers-p15.txt", addNew);
    runTest("add", "../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", addNew);
}

TEST(testHeurRelaxMax)
{
    runTest("max", "../data/ma-benchmarks/depot/pfile1.proto",
            "states/depot-pfile1.txt", maxNew);
    runTest("max", "../data/ma-benchmarks/depot/pfile5.proto",
            "states/depot-pfile5.txt", maxNew);
    runTest("max", "../data/ma-benchmarks/rovers/p03.proto",
            "states/rovers-p03.txt", maxNew);
    runTest("max", "../data/ma-benchmarks/rovers/p15.proto",
            "states/rovers-p15.txt", maxNew);
    runTest("max", "../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", maxNew);
}

TEST(testHeurRelaxFF)
{
    runTest("ff", "../data/ma-benchmarks/depot/pfile1.proto",
            "states/depot-pfile1.txt", ffNew);
    runTest("ff", "../data/ma-benchmarks/depot/pfile5.proto",
            "states/depot-pfile5.txt", ffNew);
    runTest("ff", "../data/ma-benchmarks/rovers/p03.proto",
            "states/rovers-p03.txt", ffNew);
    runTest("ff", "../data/ma-benchmarks/rovers/p15.proto",
            "states/rovers-p15.txt", ffNew);
    runTest("ff", "../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", ffNew);
}

TEST(testHeurRelaxLMCut)
{
    runTest("LM-CUT", "../data/ma-benchmarks/depot/pfile1.proto",
            "states/depot-pfile1.txt", lmCutNew);
    runTest("LM-CUT", "../data/ma-benchmarks/depot/pfile5.proto",
            "states/depot-pfile5.txt", lmCutNew);
    runTest("LM-CUT", "../data/ma-benchmarks/rovers/p03.proto",
            "states/rovers-p03.txt", lmCutNew);
    runTest("LM-CUT", "../data/ma-benchmarks/rovers/p15.proto",
            "states/rovers-p15.txt", lmCutNew);
    /*
    runTest("LM-CUT", "../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", lmCutNew);
    */
}
