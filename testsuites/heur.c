#include <time.h>
#include <stdlib.h>
#include <cu/cu.h>
#include <boruvka/alloc.h>
#include "plan/problem.h"
#include "plan/heur.h"
#include "plan/search.h"
#include "state_pool.h"
#include "../src/heur_relax.h"


static plan_heur_t *goalCountNew(plan_problem_t *p)
{
    return planHeurGoalCountNew(p->goal);
}

static plan_heur_t *addNew(plan_problem_t *p)
{
    return planHeurRelaxAddNew(p->var, p->var_size, p->goal,
                               p->op, p->op_size);
}

static plan_heur_t *maxNew(plan_problem_t *p)
{
    return planHeurRelaxMaxNew(p->var, p->var_size, p->goal,
                               p->op, p->op_size);
}

static plan_heur_t *ffNew(plan_problem_t *p)
{
    return planHeurRelaxFFNew(p->var, p->var_size, p->goal,
                              p->op, p->op_size);
}

static plan_heur_t *lmCutNew(plan_problem_t *p)
{
    return planHeurLMCutNew(p->var, p->var_size, p->goal,
                            p->op, p->op_size);
}

static plan_heur_t *lmCutIncLocalNew(plan_problem_t *p)
{
    return planHeurLMCutIncLocalNew(p->var, p->var_size, p->goal,
                                    p->op, p->op_size);
}

static plan_heur_t *lmCutIncCacheNew(plan_problem_t *p)
{
    return planHeurLMCutIncCacheNew(p->var, p->var_size, p->goal,
                                    p->op, p->op_size, 0);
}

static plan_heur_t *lmCutIncCachePruneNew(plan_problem_t *p)
{
    return planHeurLMCutIncCacheNew(p->var, p->var_size, p->goal,
                                    p->op, p->op_size,
                                    PLAN_LANDMARK_CACHE_PRUNE);
}

static plan_heur_t *dtgNew(plan_problem_t *p)
{
    return planHeurDTGNew(p->var, p->var_size, p->goal,
                          p->op, p->op_size);
}

typedef plan_heur_t *(*new_heur_fn)(plan_problem_t *p);

static void runTest(const char *name,
                    const char *proto, const char *states,
                    new_heur_fn new_heur,
                    int pref, int landmarks)
{
    plan_problem_t *p;
    state_pool_t state_pool;
    plan_state_t *state;
    plan_heur_t *heur;
    plan_heur_res_t res;
    int i, j, si;
    plan_op_t **pref_ops;

    printf("-----\n%s\n%s\n", name, proto);
    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->state_pool->num_vars);
    statePoolInit(&state_pool, states);

    pref_ops = BOR_ALLOC_ARR(plan_op_t *, p->op_size);
    for (i = 0; i < p->op_size; ++i)
        pref_ops[i] = p->op + i;

    heur = new_heur(p);
    if (heur == NULL){
        fprintf(stderr, "Test Error: Cannot create a heuristic object!\n");
        goto run_test_end;
    }

    for (si = 0; statePoolNext(&state_pool, state) == 0; ++si){
        //if (si != 4)
        //    continue;
        planHeurResInit(&res);
        if (pref){
            res.pref_op = pref_ops;
            res.pref_op_size = p->op_size;
        }
        if (landmarks)
            res.save_landmarks = 1;

        planHeurState(heur, state, &res);
        printf("[%d] ", si);
        if (res.heur == PLAN_HEUR_DEAD_END){
            printf("DEAD END");
        }else{
            printf("%d", res.heur);
        }
        printf(" ::");
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

        if (landmarks){
            for (i = 0; i < res.landmarks.size; ++i){
                printf("Landmark [%d]:", i);
                for (j = 0; j < res.landmarks.landmark[i].size; ++j){
                    printf(" %d", res.landmarks.landmark[i].op_id[j]);
                }
                printf("\n");
            }
            planLandmarkSetFree(&res.landmarks);
        }
        fflush(stdout);
    }

run_test_end:
    BOR_FREE(pref_ops);

    if (heur)
        planHeurDel(heur);
    statePoolFree(&state_pool);
    planStateDel(state);
    planProblemDel(p);
    printf("-----\n");
}

static int stopSearch(const plan_search_stat_t *state, void *_)
{
    return PLAN_SEARCH_ABORT;
}

static void runAStarTest(const char *name, const char *proto,
                         new_heur_fn new_heur, int max_steps)
{
    plan_search_astar_params_t params;
    plan_search_t *search;
    plan_problem_t *prob;
    plan_path_t path;
    const plan_state_t *state;
    const plan_state_space_node_t *node;
    int si, i;

    printf("----- A* test -----\n%s\n%s\n", name, proto);
    prob = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);

    planSearchAStarParamsInit(&params);
    params.search.heur = new_heur(prob);
    params.search.heur_del = 1;
    params.search.prob = prob;
    params.search.progress.fn = stopSearch;
    params.search.progress.freq = max_steps;
    search = planSearchAStarNew(&params);

    planPathInit(&path);
    planSearchRun(search, &path);
    planPathFree(&path);

    for (si = 0; si < prob->state_pool->num_states; ++si){
        state = planSearchLoadState(search, si);
        node = planSearchLoadNode(search, si);

        printf("[%d] ", si);
        if (node->heuristic == PLAN_HEUR_DEAD_END){
            printf("DEAD END");
        }else{
            printf("%d", (int)node->heuristic);
        }
        printf(" ::");
        for (i = 0; i < planStateSize(state); ++i){
            printf(" %d", planStateGet(state, i));
        }
        printf("\n");
    }

    planSearchDel(search);
    planProblemDel(prob);
    printf("-----\n");
}

static void heurRelaxIdentity(const char *proto, const char *states,
                              int type)
{
    plan_problem_t *p;
    state_pool_t state_pool;
    plan_state_t *state;
    plan_heur_relax_t relax;
    int si;
    plan_cost_t h, h2;

    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->state_pool->num_vars);
    statePoolInit(&state_pool, states);

    planHeurRelaxInit(&relax, type, p->var, p->var_size, p->goal,
                      p->op, p->op_size);

    for (si = 0; statePoolNext(&state_pool, state) == 0; ++si){
        planHeurRelax(&relax, state);
        h = relax.fact[relax.cref.goal_id].value;

        h2 = planHeurRelax2(&relax, state, p->goal);
        assertEquals(h, h2);
    }

    planHeurRelaxFree(&relax);
    statePoolFree(&state_pool);
    planStateDel(state);
    planProblemDel(p);
}

static plan_part_state_t *relaxStateToPartState(const plan_state_t *s,
                                                plan_state_pool_t *state_pool)
{
    plan_part_state_t *ps;
    int i, len;
    plan_val_t val;

    ps = planPartStateNew(state_pool->num_vars);
    len = planStateSize(s);
    for (i = 0; i < len; ++i){
        if (rand() < RAND_MAX / 2){
            val = planStateGet(s, i);
            planPartStateSet(ps, i, val);
        }
    }

    if (ps->vals_size == 0){
        val = planStateGet(s, 0);
        planPartStateSet(ps, 0, val);
    }

    return ps;
}

static void heurRelaxIdentity2(const char *proto, const char *states,
                               int type)
{
    plan_problem_t *p;
    state_pool_t state_pool;
    plan_state_t *state, *init_state;
    plan_part_state_t *goal;
    plan_heur_relax_t relax, relax2;
    int si;
    plan_cost_t h, h2;

    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->var_size);
    init_state = planStateNew(p->var_size);
    statePoolInit(&state_pool, states);

    statePoolNext(&state_pool, init_state);

    planHeurRelaxInit(&relax2, type, p->var, p->var_size, p->goal,
                      p->op, p->op_size);

    for (si = 0; statePoolNext(&state_pool, state) == 0 && si < 2000; ++si){
        goal = relaxStateToPartState(state, p->state_pool);

        planHeurRelaxInit(&relax, type, p->var, p->var_size, goal,
                          p->op, p->op_size);
        planHeurRelax(&relax, init_state);
        h = relax.fact[relax.cref.goal_id].value;
        planHeurRelaxFree(&relax);

        h2 = planHeurRelax2(&relax2, init_state, goal);
        assertEquals(h, h2);

        planPartStateDel(goal);
    }

    planHeurRelaxFree(&relax2);
    statePoolFree(&state_pool);
    planStateDel(state);
    planStateDel(init_state);
    planProblemDel(p);
}

TEST(testHeurRelax)
{
    heurRelaxIdentity("proto/depot-pfile1.proto",
                      "states/depot-pfile1.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity("proto/depot-pfile1.proto",
                      "states/depot-pfile1.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity("proto/depot-pfile5.proto",
                      "states/depot-pfile5.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity("proto/depot-pfile5.proto",
                      "states/depot-pfile5.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity("proto/rovers-p03.proto",
                      "states/rovers-p03.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity("proto/rovers-p03.proto",
                      "states/rovers-p03.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity("proto/rovers-p15.proto",
                      "states/rovers-p15.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity("proto/rovers-p15.proto",
                      "states/rovers-p15.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity("proto/CityCar-p3-2-2-0-1.proto",
                      "states/citycar-p3-2-2-0-1.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity("proto/CityCar-p3-2-2-0-1.proto",
                      "states/citycar-p3-2-2-0-1.txt", PLAN_HEUR_RELAX_TYPE_MAX);

    srand(time(NULL));
    heurRelaxIdentity2("proto/depot-pfile1.proto",
                       "states/depot-pfile1.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity2("proto/depot-pfile1.proto",
                       "states/depot-pfile1.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity2("proto/depot-pfile5.proto",
                       "states/depot-pfile5.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity2("proto/depot-pfile5.proto",
                       "states/depot-pfile5.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity2("proto/rovers-p03.proto",
                       "states/rovers-p03.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity2("proto/rovers-p03.proto",
                       "states/rovers-p03.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity2("proto/rovers-p15.proto",
                       "states/rovers-p15.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity2("proto/rovers-p15.proto",
                       "states/rovers-p15.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity2("proto/CityCar-p3-2-2-0-1.proto",
                       "states/citycar-p3-2-2-0-1.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity2("proto/CityCar-p3-2-2-0-1.proto",
                       "states/citycar-p3-2-2-0-1.txt", PLAN_HEUR_RELAX_TYPE_MAX);
}

TEST(testHeurGoalCount)
{
    runTest("goal-count", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", goalCountNew, 0, 0);
    runTest("goal-count", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", goalCountNew, 1, 0);
    runTest("goal-count", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", goalCountNew, 0, 0);
    runTest("goal-count", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", goalCountNew, 0, 0);
    runTest("goal-count", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", goalCountNew, 0, 0);
}

TEST(testHeurRelaxAdd)
{
    runTest("add", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", addNew, 1, 0);
    runTest("add", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", addNew, 0, 0);
    runTest("add", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", addNew, 0, 0);
    runTest("add", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", addNew, 0, 0);
    runTest("add", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", addNew, 0, 0);
}

TEST(testHeurRelaxMax)
{
    runTest("max", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", maxNew, 0, 0);
    runTest("max", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", maxNew, 0, 0);
    runTest("max", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", maxNew, 1, 0);
    runTest("max", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", maxNew, 0, 0);
    runTest("max", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", maxNew, 0, 0);
}

TEST(testHeurRelaxFF)
{
    runTest("ff", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", ffNew, 1, 0);
    runTest("ff", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", ffNew, 0, 0);
    runTest("ff", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", ffNew, 0, 0);
    runTest("ff", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", ffNew, 0, 0);
    runTest("ff", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", ffNew, 0, 0);
}

TEST(testHeurLMCut)
{
    runTest("LM-CUT", "proto/sokoban-p01.proto",
            "states/sokoban-p01.txt", lmCutNew, 0, 0);
    runTest("LM-CUT", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", lmCutNew, 0, 1);
    runTest("LM-CUT", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", lmCutNew, 1, 0);
    runTest("LM-CUT", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", lmCutNew, 0, 0);
    runTest("LM-CUT", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", lmCutNew, 0, 0);
    runTest("LM-CUT", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", lmCutNew, 0, 0);
}

TEST(testHeurLMCutIncLocal)
{
    runAStarTest("LM-Cut-inc-local", "proto/depot-pfile1.proto",
                 lmCutIncLocalNew, 100);
    runAStarTest("LM-Cut-inc-local", "proto/depot-pfile5.proto",
                 lmCutIncLocalNew, 100);
    runAStarTest("LM-Cut-inc-local", "proto/rovers-p03.proto",
                 lmCutIncLocalNew, 100);
    runAStarTest("LM-Cut-inc-local", "proto/rovers-p15.proto",
                 lmCutIncLocalNew, 100);
    runAStarTest("LM-Cut-inc-local", "proto/CityCar-p3-2-2-0-1.proto",
                 lmCutIncLocalNew, 100);
    runAStarTest("LM-Cut-inc-local", "proto/sokoban-p01.proto",
                 lmCutIncLocalNew, 100);
}

TEST(testHeurLMCutIncCache)
{
    runAStarTest("LM-Cut-inc-cache", "proto/depot-pfile1.proto",
                 lmCutIncCacheNew, 100);
    runAStarTest("LM-Cut-inc-cache", "proto/depot-pfile5.proto",
                 lmCutIncCacheNew, 100);
    runAStarTest("LM-Cut-inc-cache", "proto/rovers-p03.proto",
                 lmCutIncCacheNew, 100);
    runAStarTest("LM-Cut-inc-cache", "proto/rovers-p15.proto",
                 lmCutIncCacheNew, 100);
    runAStarTest("LM-Cut-inc-cache", "proto/CityCar-p3-2-2-0-1.proto",
                 lmCutIncCacheNew, 100);
    runAStarTest("LM-Cut-inc-cache", "proto/sokoban-p01.proto",
                 lmCutIncCacheNew, 100);
}

TEST(testHeurLMCutIncCachePrune)
{
    runAStarTest("LM-Cut-inc-cache-prune", "proto/depot-pfile1.proto",
                 lmCutIncCachePruneNew, 100);
    runAStarTest("LM-Cut-inc-cache-prune", "proto/depot-pfile5.proto",
                 lmCutIncCachePruneNew, 100);
    runAStarTest("LM-Cut-inc-cache-prune", "proto/rovers-p03.proto",
                 lmCutIncCachePruneNew, 100);
    runAStarTest("LM-Cut-inc-cache-prune", "proto/rovers-p15.proto",
                 lmCutIncCachePruneNew, 100);
    runAStarTest("LM-Cut-inc-cache-prune", "proto/CityCar-p3-2-2-0-1.proto",
                 lmCutIncCachePruneNew, 100);
    runAStarTest("LM-Cut-inc-cache-prune", "proto/sokoban-p01.proto",
                 lmCutIncCachePruneNew, 100);
}

TEST(testHeurDTG)
{
    runTest("DTG", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", dtgNew, 0, 0);
    runTest("DTG", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", dtgNew, 1, 0);
    runTest("DTG", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", dtgNew, 0, 0);
    runTest("DTG", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", dtgNew, 0, 0);
    runTest("DTG", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", dtgNew, 0, 0);
}
