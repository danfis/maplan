#include <cu/cu.h>
#include "plan/heur.h"
#include "heur_common.h"

static plan_heur_t *lmCutIncLocalNew(plan_problem_t *p)
{
    return planHeurLMCutIncLocalNew(p->var, p->var_size, p->goal,
                                    p->op, p->op_size, 0);
}

static plan_heur_t *lmCutIncCacheNew(plan_problem_t *p)
{
    return planHeurLMCutIncCacheNew(p->var, p->var_size, p->goal,
                                    p->op, p->op_size, 0, 0);
}

static plan_heur_t *lmCutIncCachePruneNew(plan_problem_t *p)
{
    return planHeurLMCutIncCacheNew(p->var, p->var_size, p->goal,
                                    p->op, p->op_size, 0,
                                    PLAN_LANDMARK_CACHE_PRUNE);
}

TEST(testHeurLMCutIncLocal)
{
    runHeurAStarTest("LM-Cut-inc-local", "proto/depot-pfile1.proto",
                     lmCutIncLocalNew, 100);
    runHeurAStarTest("LM-Cut-inc-local", "proto/depot-pfile5.proto",
                     lmCutIncLocalNew, 100);
    runHeurAStarTest("LM-Cut-inc-local", "proto/rovers-p03.proto",
                     lmCutIncLocalNew, 100);
    runHeurAStarTest("LM-Cut-inc-local", "proto/rovers-p15.proto",
                     lmCutIncLocalNew, 100);
    runHeurAStarTest("LM-Cut-inc-local", "proto/CityCar-p3-2-2-0-1.proto",
                     lmCutIncLocalNew, 100);
    runHeurAStarTest("LM-Cut-inc-local", "proto/sokoban-p01.proto",
                     lmCutIncLocalNew, 100);
}

TEST(testHeurLMCutIncCache)
{
    runHeurAStarTest("LM-Cut-inc-cache", "proto/depot-pfile1.proto",
                     lmCutIncCacheNew, 100);
    runHeurAStarTest("LM-Cut-inc-cache", "proto/depot-pfile5.proto",
                     lmCutIncCacheNew, 100);
    runHeurAStarTest("LM-Cut-inc-cache", "proto/rovers-p03.proto",
                     lmCutIncCacheNew, 100);
    runHeurAStarTest("LM-Cut-inc-cache", "proto/rovers-p15.proto",
                     lmCutIncCacheNew, 100);
    runHeurAStarTest("LM-Cut-inc-cache", "proto/CityCar-p3-2-2-0-1.proto",
                     lmCutIncCacheNew, 100);
    runHeurAStarTest("LM-Cut-inc-cache", "proto/sokoban-p01.proto",
                     lmCutIncCacheNew, 100);
}

TEST(testHeurLMCutIncCachePrune)
{
    runHeurAStarTest("LM-Cut-inc-cache-prune", "proto/depot-pfile1.proto",
                     lmCutIncCachePruneNew, 100);
    runHeurAStarTest("LM-Cut-inc-cache-prune", "proto/depot-pfile5.proto",
                     lmCutIncCachePruneNew, 100);
    runHeurAStarTest("LM-Cut-inc-cache-prune", "proto/rovers-p03.proto",
                     lmCutIncCachePruneNew, 100);
    runHeurAStarTest("LM-Cut-inc-cache-prune", "proto/rovers-p15.proto",
                     lmCutIncCachePruneNew, 100);
    runHeurAStarTest("LM-Cut-inc-cache-prune", "proto/CityCar-p3-2-2-0-1.proto",
                     lmCutIncCachePruneNew, 100);
    runHeurAStarTest("LM-Cut-inc-cache-prune", "proto/sokoban-p01.proto",
                     lmCutIncCachePruneNew, 100);
}
