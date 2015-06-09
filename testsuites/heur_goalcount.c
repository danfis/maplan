#include <cu/cu.h>
#include "plan/heur.h"
#include "heur_common.h"

static plan_heur_t *goalCountNew(plan_problem_t *p)
{
    return planHeurGoalCountNew(p->goal);
}


TEST(testHeurGoalCount)
{
    runHeurTest("goal-count", "proto/depot-pfile1.proto",
                "states/depot-pfile1.txt", goalCountNew, 0, 0);
    runHeurTest("goal-count", "proto/depot-pfile5.proto",
                "states/depot-pfile5.txt", goalCountNew, 1, 0);
    runHeurTest("goal-count", "proto/rovers-p03.proto",
                "states/rovers-p03.txt", goalCountNew, 0, 0);
    runHeurTest("goal-count", "proto/rovers-p15.proto",
                "states/rovers-p15.txt", goalCountNew, 0, 0);
    runHeurTest("goal-count", "proto/CityCar-p3-2-2-0-1.proto",
                "states/citycar-p3-2-2-0-1.txt", goalCountNew, 0, 0);
}

