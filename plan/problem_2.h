/***
 * maplan
 * -------
 * Copyright (c)2017 Daniel Fiser <danfis@danfis.cz>,
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

#ifndef __PLAN_PROBLEM_2_H__
#define __PLAN_PROBLEM_2_H__

#include <plan/problem.h>
#include <plan/fact_id.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


struct _plan_fact_2_t {
    int *pre_op;
    int pre_op_size;
    int pre_op_alloc;

    int *eff_op;
    int eff_op_size;
    int eff_op_alloc;
};
typedef struct _plan_fact_2_t plan_fact_2_t;

struct _plan_op_2_t {
    int original_op_id;
    int parent;
    int *child;
    int child_size;
    int child_alloc;
    int pre_size;
    int *eff;
    int eff_size;
    plan_cost_t cost;
};
typedef struct _plan_op_2_t plan_op_2_t;

struct _plan_problem_2_t {
    plan_fact_id_t fact_id;
    plan_fact_2_t *fact;
    int fact_size;
    plan_op_2_t *op;
    int op_size;
    int op_alloc;
    int *goal;
    int goal_size;
    int goal_fact_id;
    int goal_op_id;
};
typedef struct _plan_problem_2_t plan_problem_2_t;

void planProblem2Init(plan_problem_2_t *p2, const plan_problem_t *p);
void planProblem2Free(plan_problem_2_t *p2);
void planProblem2PruneByMutex(plan_problem_2_t *p2, int fact_id);
void planProblem2PruneEmptyOps(plan_problem_2_t *p2);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PROBLEM_2_H__ */
