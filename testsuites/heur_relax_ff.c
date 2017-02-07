#include <cu/cu.h>
#include "plan/heur.h"
#include "heur_common.h"

static plan_heur_t *ffNew(plan_problem_t *p)
{
    return planHeurRelaxFFNew(p, 0);
}

static plan_heur_t *ff1New(plan_problem_t *p)
{
    return planHeurRelaxFFNew(p, PLAN_HEUR_OP_UNIT_COST);
}

static plan_heur_t *ffPlus1New(plan_problem_t *p)
{
    return planHeurRelaxFFNew(p, PLAN_HEUR_OP_COST_PLUS_ONE);
}

TEST(testHeurRelaxFF)
{
    runHeurTest("ff", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", ffNew, 1, 0);
    runHeurTest("ff", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", ffNew, 0, 0);
    runHeurTest("ff", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", ffNew, 0, 0);
    runHeurTest("ff", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", ffNew, 0, 0);
    runHeurTest("ff", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", ffNew, 0, 0);

    runHeurTest("ff-1", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", ff1New, 0, 0);
    runHeurTest("ff-1", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", ff1New, 0, 0);

    runHeurTest("ff+1", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", ffPlus1New, 0, 0);
    runHeurTest("ff+1", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", ffPlus1New, 0, 0);
}
