#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/heur.h"
#include "plan/search.h"
#include "state_pool.h"
#include "heur_common.h"

static plan_heur_t *h2New(plan_problem_t *p)
{
    return planHeurH2MaxNew(p->var, p->var_size, p->goal,
                            p->op, p->op_size, 0);
}

TEST(testHeurH2Max)
{
    runHeurTest("h2", "proto/depot-pfile1.proto",
                "states/depot-pfile1.txt", h2New, 0, 0);
}
