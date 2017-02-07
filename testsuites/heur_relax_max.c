#include <cu/cu.h>
#include "plan/heur.h"
#include "heur_common.h"

static plan_heur_t *maxNew(plan_problem_t *p)
{
    return planHeurRelaxMaxNew(p, 0);
}

TEST(testHeurRelaxMax)
{
    runHeurTest("max", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", maxNew, 0, 0);
    runHeurTest("max", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", maxNew, 0, 0);
    runHeurTest("max", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", maxNew, 1, 0);
    runHeurTest("max", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", maxNew, 0, 0);
    runHeurTest("max", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", maxNew, 0, 0);
}

