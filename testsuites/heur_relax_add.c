#include <cu/cu.h>
#include "plan/heur.h"
#include "heur_common.h"

static plan_heur_t *addNew(plan_problem_t *p)
{
    return planHeurRelaxAddNew(p->var, p->var_size, p->goal,
                               p->op, p->op_size, 0);
}

static plan_heur_t *add1New(plan_problem_t *p)
{
    return planHeurRelaxAddNew(p->var, p->var_size, p->goal,
                               p->op, p->op_size, PLAN_HEUR_OP_UNIT_COST);
}

static plan_heur_t *addPlus1New(plan_problem_t *p)
{
    return planHeurRelaxAddNew(p->var, p->var_size, p->goal,
                               p->op, p->op_size,
                               PLAN_HEUR_OP_COST_PLUS_ONE);
}

TEST(testHeurRelaxAdd)
{
    runHeurTest("add", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", addNew, 1, 0);
    runHeurTest("add", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", addNew, 0, 0);
    runHeurTest("add", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", addNew, 0, 0);
    runHeurTest("add", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", addNew, 0, 0);
    runHeurTest("add", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", addNew, 0, 0);

    runHeurTest("add-1", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", add1New, 0, 0);
    runHeurTest("add-1", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", add1New, 0, 0);

    runHeurTest("add+1", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", addPlus1New, 0, 0);
    runHeurTest("add+1", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", addPlus1New, 0, 0);
}

