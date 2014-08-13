#ifndef __PLAN_CAUSALGRAPH_H__
#define __PLAN_CAUSALGRAPH_H__

#include <boruvka/rbtree_int.h>
#include <plan/operator.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_causal_graph_t {
    bor_rbtree_int_t *successor_graph;
    bor_rbtree_int_t *predecessor_graph;
};
typedef struct _plan_causal_graph_t plan_causal_graph_t;

/**
 * Creates a new causal graph of variables
 */
plan_causal_graph_t *planCausalGraphNew(const plan_var_t *var, int var_size,
                                        const plan_operator_t *op, int op_size,
                                        const plan_part_state_t *goal);

/**
 * Deletes causal graph object.
 */
void planCausalGraphDel(plan_causal_graph_t *cg);


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_CAUSALGRAPH_H__ */
