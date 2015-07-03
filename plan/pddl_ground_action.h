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

#ifndef __PLAN_PDDL_GROUND_ACTION_H__
#define __PLAN_PDDL_GROUND_ACTION_H__

#include <plan/pddl.h>
#include <plan/pddl_fact_pool.h>
#include <plan/pddl_lift_action.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /*
struct _plan_pddl_ground_facts_t {
    int *fact;
    int size;
};
typedef struct _plan_pddl_ground_facts_t plan_pddl_ground_facts_t;

struct _plan_pddl_ground_cond_eff_t {
    plan_pddl_ground_facts_t pre;
    plan_pddl_ground_facts_t eff;
};
typedef struct _plan_pddl_ground_cond_eff_t plan_pddl_ground_cond_eff_t;

struct _plan_pddl_ground_cond_effs_t {
    plan_pddl_ground_cond_eff_t *cond_eff;
    int size;
};
typedef struct _plan_pddl_ground_cond_effs_t plan_pddl_ground_cond_effs_t;

struct _plan_pddl_ground_action_t {
    char *name;
    int cost;
    plan_pddl_ground_facts_t pre;
    plan_pddl_ground_facts_t eff;
    plan_pddl_ground_cond_effs_t cond_eff;
};
typedef struct _plan_pddl_ground_action_t plan_pddl_ground_action_t;
*/

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

struct _plan_pddl_ground_action_pool_t {
    bor_htable_t *htable;
    bor_extarr_t *action;
    int size;
    const plan_pddl_objs_t *objs;
};
typedef struct _plan_pddl_ground_action_pool_t plan_pddl_ground_action_pool_t;

/**
 * Initializes action pool.
 */
void planPDDLGroundActionPoolInit(plan_pddl_ground_action_pool_t *ga,
                                  const plan_pddl_objs_t *objs);

/**
 * Frees allocated resources.
 */
void planPDDLGroundActionPoolFree(plan_pddl_ground_action_pool_t *ga);

/**
 * Adds action grounded from the lifted form and adds its effects to the
 * fact-pool.
 */
int planPDDLGroundActionPoolAdd(plan_pddl_ground_action_pool_t *ga,
                                const plan_pddl_lift_action_t *lift_action,
                                const int *bound_arg,
                                plan_pddl_fact_pool_t *fact_pool);

/**
 * Returns i'th action from the pool.
 */
plan_pddl_ground_action_t *planPDDLGroundActionPoolGet(
            const plan_pddl_ground_action_pool_t *pool, int i);

void planPDDLGroundActionFree(plan_pddl_ground_action_t *ga);
void planPDDLGroundActionsFree(plan_pddl_ground_actions_t *ga);
void planPDDLGroundActionCopy(plan_pddl_ground_action_t *dst,
                              const plan_pddl_ground_action_t *src);
void planPDDLGroundActionPrint(const plan_pddl_ground_action_t *a,
                               const plan_pddl_predicates_t *preds,
                               const plan_pddl_objs_t *objs,
                               FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_GROUND_ACTION_H__ */
