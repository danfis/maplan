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

#include <boruvka/extarr.h>
#include <boruvka/htable.h>
#include <plan/pddl.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_ground_fact_pool_t {
    bor_htable_t *htable;
    bor_extarr_t **fact;
    int *fact_size;
    int pred_size;
    int size;
};
typedef struct _plan_pddl_ground_fact_pool_t plan_pddl_ground_fact_pool_t;

struct _plan_pddl_ground_action_pool_t {
    bor_htable_t *htable;
    bor_extarr_t *action;
    int size;
    const plan_pddl_objs_t *objs;
};
typedef struct _plan_pddl_ground_action_pool_t plan_pddl_ground_action_pool_t;

struct _plan_pddl_ground_t {
    const plan_pddl_t *pddl;
    plan_pddl_ground_fact_pool_t fact_pool;
    plan_pddl_ground_action_pool_t action_pool;
};
typedef struct _plan_pddl_ground_t plan_pddl_ground_t;

/**
 * Initialize the main structure.
 */
void planPDDLGroundInit(plan_pddl_ground_t *g, const plan_pddl_t *pddl);

/**
 * Frees allocated resources.
 */
void planPDDLGroundFree(plan_pddl_ground_t *g);

/**
 * Grounds pddl problem.
 */
void planPDDLGround(plan_pddl_ground_t *g);

void planPDDLGroundPrint(const plan_pddl_ground_t *g, FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_GROUND_H__ */
