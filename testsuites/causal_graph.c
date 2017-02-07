#include <stdio.h>
#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/causal_graph.h"

TEST(testCausalGraphImportantVar)
{
    plan_problem_t *prob;
    plan_causal_graph_t *cg;

    prob = planProblemFromProto("proto/rovers-p03.proto", 0);
    cg = planCausalGraphNew(prob->var_size);
    planCausalGraphBuildFromOps(cg, prob->op, prob->op_size);
    planCausalGraph(cg, prob->goal);
    assertTrue(cg->important_var[0]);
    assertTrue(cg->important_var[1]);
    assertTrue(cg->important_var[2]);
    assertTrue(cg->important_var[3]);
    assertTrue(cg->important_var[4]);
    assertTrue(cg->important_var[5]);
    assertFalse(cg->important_var[6]);
    assertTrue(cg->important_var[7]);
    assertTrue(cg->important_var[8]);
    assertFalse(cg->important_var[9]);
    assertFalse(cg->important_var[10]);
    assertFalse(cg->important_var[11]);
    assertFalse(cg->important_var[12]);
    assertFalse(cg->important_var[13]);
    assertTrue(cg->important_var[14]);
    assertFalse(cg->important_var[15]);
    assertFalse(cg->important_var[16]);
    assertTrue(cg->important_var[17]);
    assertTrue(cg->important_var[18]);
    assertTrue(cg->important_var[19]);
    assertFalse(cg->important_var[20]);
    assertFalse(cg->important_var[21]);
    assertTrue(cg->important_var[22]);
    assertFalse(cg->important_var[23]);
    assertFalse(cg->important_var[24]);
    assertFalse(cg->important_var[25]);
    assertFalse(cg->important_var[26]);
    assertFalse(cg->important_var[27]);

    planCausalGraphDel(cg);
    planProblemDel(prob);
}

static void varOrder(const char *fn)
{
    plan_problem_t *prob;
    plan_causal_graph_t *cg;
    int i;

    prob = planProblemFromProto(fn, 0);
    cg = planCausalGraphNew(prob->var_size);
    planCausalGraphBuildFromOps(cg, prob->op, prob->op_size);
    planCausalGraph(cg, prob->goal);

    fprintf(stdout, "VarOrder for `%s':\n", fn);
    for (i = 0; i < cg->var_order_size; ++i){
        fprintf(stdout, "%d - %s\n", cg->var_order[i],
                prob->var[cg->var_order[i]].name);
    }

    planCausalGraphDel(cg);
    planProblemDel(prob);
}

TEST(testCausalGraphVarOrder)
{
    varOrder("proto/rovers-p03.proto");
}

TEST(testCausalGraphPruneUnimportantVar)
{
    plan_problem_t *prob;

    prob = planProblemFromProto("proto/rovers-p03.proto", 0);
    assertEquals(prob->var_size, 28);
    planProblemDel(prob);

    prob = planProblemFromProto("proto/rovers-p03.proto",
                                PLAN_PROBLEM_USE_CG);
    assertEquals(prob->var_size, 13);
    planProblemDel(prob);
}
