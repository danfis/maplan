#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/heur.h"
#include "plan/search.h"
#include "state_pool.h"
#include "heur_common.h"

static plan_heur_t *h2LMCutNew(plan_problem_t *p)
{
    return planHeurH2LMCutNew(p, 0);
}

TEST(testHeurH2LMCut)
{
    runHeurTest("h2-lm-cut", "proto/simple.proto",
                "states/simple.txt", h2LMCutNew, 0, 0);
    runHeurTest("h2-lm-cut", "proto/depot-pfile1.proto",
                "states/depot-pfile1.txt", h2LMCutNew, 0, 0);
}

