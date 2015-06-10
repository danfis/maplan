/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#ifndef __PLAN_CAUSALGRAPH_H__
#define __PLAN_CAUSALGRAPH_H__

#include <plan/op.h>

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
    plan_var_id_t *var_order; /*!< Ordered array of variable IDs, array
                                   ends with PLAN_VAR_ID_UNDEFINED. */
    int var_order_size;       /*!< Number of elements in .var_order[] */
};
typedef struct _plan_causal_graph_t plan_causal_graph_t;

/**
 * Creates a new causal graph of variables
 */
plan_causal_graph_t *planCausalGraphNew(int var_size,
                                        const plan_op_t *op, int op_size,
                                        const plan_part_state_t *goal);

/**
 * Deletes causal graph object.
 */
void planCausalGraphDel(plan_causal_graph_t *cg);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_CAUSALGRAPH_H__ */
