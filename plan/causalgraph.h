#ifndef __PLAN_CAUSALGRAPH_H__
#define __PLAN_CAUSALGRAPH_H__

#include <plan/operator.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_causal_graph_graph_t {
    int **end_var;  /*!< ID of variable at the end of the edge */
    int **value;    /*!< Value of the edge */
    int *edge_size; /*!< Number edges emanating from i'th variable */
    int var_size;   /*!< Number of variables */
};
typedef struct _plan_causal_graph_graph_t plan_causal_graph_graph_t;

struct _plan_causal_graph_t {
    int var_size;

    /** Graph with edges from precondition vars to effect vars */
    plan_causal_graph_graph_t successor_graph;
    /** Graph with edges from effect vars to precondition vars */
    plan_causal_graph_graph_t predecessor_graph;
    /** Bool flag for each variable whether it is important, i.e., if it is
     *  connected through operators with the goal. */
    int *important_var;
    int *var_order; /*!< Ordered array of variable IDs */
    int var_order_size;
};
typedef struct _plan_causal_graph_t plan_causal_graph_t;

/**
 * Creates a new causal graph of variables
 */
plan_causal_graph_t *planCausalGraphNew(int var_size,
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
