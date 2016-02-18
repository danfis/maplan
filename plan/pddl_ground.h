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
#include <plan/pddl_fact_pool.h>
#include <plan/pddl_lift_action.h>
#include <plan/pddl_ground_action.h>
#include <plan/ma_comm.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_ground_t {
    plan_pddl_fact_pool_t fact_pool;
    plan_pddl_ground_action_pool_t action_pool;
    plan_pddl_lift_actions_t lift_action;
    plan_pddl_ground_facts_t init; /*!< List of facts in initial state */
    plan_pddl_ground_facts_t goal; /*!< List of facts in goal */

    int agent_size;    /*!< Number of agents defined in PDDL */
    int *agent_to_obj; /*!< Mapping between agent ID and the
                            corresponding object ID */
    int *obj_to_agent; /*!< Mapping between object ID (owner ID) and
                            agent ID */

    int *glob_to_obj;  /*!< Mapping from global obj ID to local ID */
    int *obj_to_glob;  /*!< Inverse mapping to .glob_to_obj[] */
    int *glob_to_pred; /*!< Mapping from global predicate ID to local ID */
    int *pred_to_glob; /*!< Inverse mapping to .glob_to_pred[] */
};
typedef struct _plan_pddl_ground_t plan_pddl_ground_t;

/**
 * Grounds pddl problem.
 */
void planPDDLGround(plan_pddl_ground_t *g, const plan_pddl_t *pddl);

/**
 * Grounds factored pddl problems.
 */
int planPDDLGroundFactor(plan_pddl_ground_t *g, const plan_pddl_t *pddl,
                         plan_ma_comm_t *comm);

/**
 * Frees allocated resources.
 */
void planPDDLGroundFree(plan_pddl_ground_t *g);

void planPDDLGroundPrint(const plan_pddl_ground_t *g,
                         const plan_pddl_t *pddl, FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_GROUND_H__ */
