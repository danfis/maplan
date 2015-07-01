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

#ifndef __PLAN_PDDL_GROUND_H__
#define __PLAN_PDDL_GROUND_H__

#include <plan/pddl.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_ground_action_t {
    char *name;
    int *arg;
    int arg_size;
    int cost;
    plan_pddl_facts_t pre;
    plan_pddl_facts_t eff;
    plan_pddl_cond_effs_t cond_eff;
};
typedef struct _plan_pddl_ground_action_t plan_pddl_ground_action_t;

struct _plan_pddl_ground_actions_t {
    plan_pddl_ground_action_t *action;
    int size;
};
typedef struct _plan_pddl_ground_actions_t plan_pddl_ground_actions_t;

struct _plan_pddl_ground_t {
    const plan_pddl_t *pddl;
    plan_pddl_facts_t fact;
    plan_pddl_ground_actions_t action;
};
typedef struct _plan_pddl_ground_t plan_pddl_ground_t;

/**
 * Grounds pddl object.
 */
void planPDDLGround(const plan_pddl_t *pddl, plan_pddl_ground_t *g);

/**
 * Frees allocated resources.
 */
void planPDDLGroundFree(plan_pddl_ground_t *g);

void planPDDLGroundPrint(const plan_pddl_ground_t *g, FILE *fout);

void planPDDLGroundActionFree(plan_pddl_ground_action_t *ga);
void planPDDLGroundActionsFree(plan_pddl_ground_actions_t *ga);
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_GROUND_H__ */
