#include <cu/cu.h>
#include <plan/problem.h>
#include <plan/dtg.h>

static void dtgDump(const char *proto)
{
    plan_dtg_t dtg;
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

TEST(dtgPathTest)
{
    plan_dtg_t dtg;
    plan_problem_t *p;
    plan_dtg_path_t path;
    int i, ret, flags;

    flags = PLAN_PROBLEM_USE_CG | PLAN_PROBLEM_PRUNE_DUPLICATES;
    p = planProblemFromProto("../data/ma-benchmarks/rovers/p01.proto", flags);

    planDTGInit(&dtg, p->var, p->var_size, p->op, p->op_size);
    planDTGPathInit(&path);
    ret = planDTGPath(&dtg, 0, 0, 2, &path);
    assertEquals(ret, 0);
    assertEquals(path.length, 3);
    for (i = 0; i < path.length; ++i)
        assertTrue(path.trans[i]->ops_size > 0);
    planDTGPathFree(&path);

    planDTGPathInit(&path);
    ret = planDTGPath(&dtg, 0, 0, 3, &path);
    assertEquals(ret, 0);
    assertEquals(path.length, 1);
    for (i = 0; i < path.length; ++i)
        assertTrue(path.trans[i]->ops_size > 0);
    planDTGPathFree(&path);

    planDTGPathInit(&path);
    ret = planDTGPath(&dtg, 12, 0, 1, &path);
    assertNotEquals(ret, 0);
    planDTGPathFree(&path);

    planDTGFree(&dtg);
    planProblemDel(p);
}
