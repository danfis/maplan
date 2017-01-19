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

#ifndef __PLAN_FACT_OP_CROSS_REF_H__
#define __PLAN_FACT_OP_CROSS_REF_H__

#include "plan/arr.h"
#include "plan/problem.h"
#include "plan/fact_id.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_fake_pre_t {
    int fact_id;
    plan_cost_t value;
};
typedef struct _plan_fake_pre_t plan_fake_pre_t;

struct _plan_fact_op_cross_ref_t {
    unsigned flags;
    plan_fact_id_t fact_id; /*!< Translation from var-val pair to fact ID */

    int fact_size; /*!< Number of facts -- value from .fact_id */
    int op_size;   /*!< Number of operators */
    int op_alloc;  /*!< Allocated space for operators */
    plan_arr_int_t *fact_pre; /*!< Operators for which the corresponding fact
                                   is precondition -- the size of this array
                                   equals to .fact_size */
    plan_arr_int_t *fact_eff; /*!< Operators for which the corresponding fact
                                   is effect. This array is created only in
                                   case of lm-cut heuristic. */
    plan_arr_int_t *op_pre; /*!< Precondition facts of operators. Size of
                                 this array is .op_size */
    plan_arr_int_t *op_eff; /*!< Unrolled effects of operators. Size of
                                 this array equals to .op_size */
    int goal_id;            /*!< Fact ID of the artificial goal */
    int goal_op_id;         /*!< ID of artificial operator that has
                                 .goal_id as an effect */
    plan_fake_pre_t *fake_pre; /*!< Artificial precondition facts */
    int fake_pre_size;      /*!< Number of precondition facts */
    int *op_id;             /*!< Returns original operator ID. This is here
                                 mainly because of the conditional effects */
};
typedef struct _plan_fact_op_cross_ref_t plan_fact_op_cross_ref_t;


/**
 * Initialize cross reference table
 */
void planFactOpCrossRefInit(plan_fact_op_cross_ref_t *cr,
                            const plan_var_t *var, int var_size,
                            const plan_part_state_t *goal,
                            const plan_op_t *op, int op_size,
                            unsigned flags);

/**
 * Frees allocated resources.
 */
void planFactOpCrossRefFree(plan_fact_op_cross_ref_t *cr);

/**
 * Adds a new fake precondition fact with a specified initial value.
 * Returns ID of the fact.
 */
int planFactOpCrossRefAddFakePre(plan_fact_op_cross_ref_t *cr,
                                 plan_cost_t value);

/**
 * Sets initial value of the specified fake precondition.
 */
void planFactOpCrossRefSetFakePreValue(plan_fact_op_cross_ref_t *cr,
                                       int fact_id, plan_cost_t value);

/**
 * Adds specified fact as a precondition to the operator.
 */
void planFactOpCrossRefAddPre(plan_fact_op_cross_ref_t *cr,
                              int op_id, int fact_id);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_FACT_OP_CROSS_REF_H__ */
