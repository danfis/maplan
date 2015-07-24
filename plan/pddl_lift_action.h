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

#ifndef __PLAN_PDDL_LIFT_ACTION_H__
#define __PLAN_PDDL_LIFT_ACTION_H__

#include <plan/pddl.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_lift_action_t {
    int action_id;
    const plan_pddl_action_t *action;
    int param_size;
    int obj_size;
    const plan_pddl_facts_t *func;
    int **allowed_arg;
    plan_pddl_facts_t pre;
    plan_pddl_facts_t pre_neg;
    plan_pddl_facts_t eq;
    int owner_param;
};
typedef struct _plan_pddl_lift_action_t plan_pddl_lift_action_t;

struct _plan_pddl_lift_actions_t {
    plan_pddl_lift_action_t *action;
    int size;
};
typedef struct _plan_pddl_lift_actions_t plan_pddl_lift_actions_t;

/**
 * Creates a list of lifted actions ready to be grounded.
 */
void planPDDLLiftActionsInit(plan_pddl_lift_actions_t *action,
                             const plan_pddl_actions_t *pddl_action,
                             const plan_pddl_type_obj_t *type_obj,
                             const plan_pddl_facts_t *func,
                             int obj_size, int eq_fact_id);

/**
 * Frees allocated resouces
 */
void planPDDLLiftActionsFree(plan_pddl_lift_actions_t *as);

/**
 * Prints lifted actions -- for debug
 */
void planPDDLLiftActionsPrint(const plan_pddl_lift_actions_t *a,
                              const plan_pddl_predicates_t *pred,
                              const plan_pddl_objs_t *obj,
                              FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_LIFT_ACTION_H__ */
