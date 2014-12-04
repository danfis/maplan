#include <cu/cu.h>
#include <boruvka/alloc.h>
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
                    new_heur_fn new_heur,
                    int pref)
{
    plan_problem_t *p;
    state_pool_t state_pool;
    plan_state_t *state;
    plan_heur_t *heur;
    plan_heur_res_t res;
    int i, si;
    plan_op_t **pref_ops;

    printf("-----\n%s\n%s\n", name, proto);
    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->state_pool);
    statePoolInit(&state_pool, states);

    pref_ops = BOR_ALLOC_ARR(plan_op_t *, p->op_size);
    for (i = 0; i < p->op_size; ++i)
        pref_ops[i] = p->op + i;

    heur = new_heur(p);

    for (si = 0; statePoolNext(&state_pool, state) == 0; ++si){
        planHeurResInit(&res);
        if (pref){
            res.pref_op = pref_ops;
            res.pref_op_size = p->op_size;
        }

        planHeur(heur, state, &res);
        printf("[%d] %d ::", si, res.heur);
        for (i = 0; i < planStateSize(state); ++i){
            printf(" %d", planStateGet(state, i));
        }
        printf("\n");

        if (pref){
            printf("Pref ops[%d]:\n", res.pref_size);
            for (i = 0; i < res.pref_size; ++i){
                printf("%s\n", res.pref_op[i]->name);
            }
        }
    }

    BOR_FREE(pref_ops);

    planHeurDel(heur);
    statePoolFree(&state_pool);
    planStateDel(state);
    planProblemDel(p);
    printf("-----\n");
}


TEST(testHeurGoalCount)
{
    runTest("goal-count", "../data/ma-benchmarks/depot/pfile1.proto",
            "states/depot-pfile1.txt", goalCountNew, 0);
    runTest("goal-count", "../data/ma-benchmarks/depot/pfile5.proto",
            "states/depot-pfile5.txt", goalCountNew, 1);
    runTest("goal-count", "../data/ma-benchmarks/rovers/p03.proto",
            "states/rovers-p03.txt", goalCountNew, 0);
    runTest("goal-count", "../data/ma-benchmarks/rovers/p15.proto",
            "states/rovers-p15.txt", goalCountNew, 0);
    runTest("goal-count", "../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", goalCountNew, 0);
}

TEST(testHeurRelaxAdd)
{
    runTest("add", "../data/ma-benchmarks/depot/pfile1.proto",
            "states/depot-pfile1.txt", addNew, 1);
    runTest("add", "../data/ma-benchmarks/depot/pfile5.proto",
            "states/depot-pfile5.txt", addNew, 0);
    runTest("add", "../data/ma-benchmarks/rovers/p03.proto",
            "states/rovers-p03.txt", addNew, 0);
    runTest("add", "../data/ma-benchmarks/rovers/p15.proto",
            "states/rovers-p15.txt", addNew, 0);
    runTest("add", "../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", addNew, 0);
}

TEST(testHeurRelaxMax)
{
    runTest("max", "../data/ma-benchmarks/depot/pfile1.proto",
            "states/depot-pfile1.txt", maxNew, 0);
    runTest("max", "../data/ma-benchmarks/depot/pfile5.proto",
            "states/depot-pfile5.txt", maxNew, 0);
    runTest("max", "../data/ma-benchmarks/rovers/p03.proto",
            "states/rovers-p03.txt", maxNew, 1);
    runTest("max", "../data/ma-benchmarks/rovers/p15.proto",
            "states/rovers-p15.txt", maxNew, 0);
    runTest("max", "../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", maxNew, 0);
}

TEST(testHeurRelaxFF)
{
    runTest("ff", "../data/ma-benchmarks/depot/pfile1.proto",
            "states/depot-pfile1.txt", ffNew, 1);
    runTest("ff", "../data/ma-benchmarks/depot/pfile5.proto",
            "states/depot-pfile5.txt", ffNew, 0);
    runTest("ff", "../data/ma-benchmarks/rovers/p03.proto",
            "states/rovers-p03.txt", ffNew, 0);
    runTest("ff", "../data/ma-benchmarks/rovers/p15.proto",
            "states/rovers-p15.txt", ffNew, 0);
    runTest("ff", "../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", ffNew, 0);
}

TEST(testHeurRelaxLMCut)
{
    runTest("LM-CUT", "../data/ma-benchmarks/depot/pfile1.proto",
            "states/depot-pfile1.txt", lmCutNew, 0);
    runTest("LM-CUT", "../data/ma-benchmarks/depot/pfile5.proto",
            "states/depot-pfile5.txt", lmCutNew, 1);
    runTest("LM-CUT", "../data/ma-benchmarks/rovers/p03.proto",
            "states/rovers-p03.txt", lmCutNew, 0);
    runTest("LM-CUT", "../data/ma-benchmarks/rovers/p15.proto",
            "states/rovers-p15.txt", lmCutNew, 0);
    /*
    runTest("LM-CUT", "../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", lmCutNew);
    */
}
