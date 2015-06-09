#include <cu/cu.h>
#include "plan/heur.h"
#include "heur_common.h"

static plan_heur_t *dtgNew(plan_problem_t *p)
{
    return planHeurDTGNew(p->var, p->var_size, p->goal,
                          p->op, p->op_size);
}

TEST(testHeurDTG)
{
    runHeurTest("DTG", "proto/depot-pfile1.proto",
            "states/depot-pfile1.txt", dtgNew, 0, 0);
    runHeurTest("DTG", "proto/depot-pfile5.proto",
            "states/depot-pfile5.txt", dtgNew, 1, 0);
    runHeurTest("DTG", "proto/rovers-p03.proto",
            "states/rovers-p03.txt", dtgNew, 0, 0);
    runHeurTest("DTG", "proto/rovers-p15.proto",
            "states/rovers-p15.txt", dtgNew, 0, 0);
    runHeurTest("DTG", "proto/CityCar-p3-2-2-0-1.proto",
            "states/citycar-p3-2-2-0-1.txt", dtgNew, 0, 0);
}
