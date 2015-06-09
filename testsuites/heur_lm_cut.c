#include <cu/cu.h>
#include "plan/heur.h"
#include "heur_common.h"

static plan_heur_t *lmCutNew(plan_problem_t *p)
{
    return planHeurLMCutNew(p->var, p->var_size, p->goal,
                            p->op, p->op_size, 0);
}

static plan_heur_t *lmCut1New(plan_problem_t *p)
{
    return planHeurLMCutNew(p->var, p->var_size, p->goal,
                            p->op, p->op_size, PLAN_HEUR_OP_UNIT_COST);
}

static plan_heur_t *lmCutPlus1New(plan_problem_t *p)
{
    return planHeurLMCutNew(p->var, p->var_size, p->goal,
                            p->op, p->op_size,
                            PLAN_HEUR_OP_COST_PLUS_ONE);
}

TEST(testHeurLMCut)
{
    runHeurTest("LM-CUT", "proto/sokoban-p01.proto",
            "states/sokoban-p01.txt", lmCutNew, 0, 0);
    runHeurTest("LM-CUT", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", lmCutNew, 0, 1);
    runHeurTest("LM-CUT", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", lmCutNew, 1, 0);
    runHeurTest("LM-CUT", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", lmCutNew, 0, 0);
    runHeurTest("LM-CUT", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", lmCutNew, 0, 0);
    runHeurTest("LM-CUT", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", lmCutNew, 0, 0);

    runHeurTest("LM-CUT-1", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", lmCut1New, 0, 0);
    runHeurTest("LM-CUT-1", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", lmCut1New, 0, 0);

    runHeurTest("LM-CUT+1", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", lmCutPlus1New, 0, 0);
    runHeurTest("LM-CUT+1", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", lmCutPlus1New, 0, 0);
}
