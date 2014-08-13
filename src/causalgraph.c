#include "plan/causalgraph.h"

plan_causal_graph_t *planCausalGraphNew(const plan_var_t *var, int var_size,
                                        const plan_operator_t *op, int op_size,
                                        const plan_part_state_t *goal)
{
    plan_causal_graph_t *cg;
    cg = NULL;

    return cg;
}

void planCausalGraphDel(plan_causal_graph_t *cg)
{
}
