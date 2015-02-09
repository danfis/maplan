#include <cu/cu.h>
#include <plan/problem.h>
#include <plan/dtg.h>

static void dtgDump(const char *proto)
{
    dtg_t dtg;
    plan_problem_t *p;
    int flags;

    fprintf(stdout, "-- %s --\n", proto);
    flags = PLAN_PROBLEM_USE_CG | PLAN_PROBLEM_PRUNE_DUPLICATES;
    p = planProblemFromProto(proto, flags);

    planDTGInit(&dtg, p->var, p->var_size, p->op, p->op_size);
    planDTGPrint(&dtg, stdout);
    planDTGFree(&dtg);
    planProblemDel(p);
    fprintf(stdout, "-- %s END --\n", proto);
}

TEST(dtgTest)
{
    dtgDump("../data/ma-benchmarks/rovers/p01.proto");
    dtgDump("../data/ma-benchmarks/depot/pfile1.proto");
    dtgDump("../data/ma-benchmarks/rovers/p03.proto");
}
