#include <stdio.h>
#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/causalgraph.h"

TEST(testCausalGraphImportantVar)
{
    plan_problem_t *prob;
    plan_causal_graph_t *cg;
    int i;

    prob = planProblemFromProto("../data/ma-benchmarks/rovers/p03.proto");
    cg = planCausalGraphNew(prob->var, prob->var_size,
                            prob->op, prob->op_size, prob->goal);
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
