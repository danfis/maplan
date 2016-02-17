/***
 * maplan
 * -------
 * Copyright (c)2016 Daniel Fiser <danfis@danfis.cz>,
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

#ifndef __PLAN_PDDL_INV_H__
#define __PLAN_PDDL_INV_H__

#include <boruvka/list.h>
#include <plan/pddl_ground.h>
#include <plan/lp.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_inv_t {
    int *fact;
    int size;
    bor_list_t list;
};
typedef struct _plan_pddl_inv_t plan_pddl_inv_t;

struct _plan_pddl_inv_finder_t {
    bor_list_t inv; /*!< List of invariants */
    int inv_size;   /*!< Number of inferred invariants */
    plan_lp_t *lp;  /*!< Linear program for inferring invariants */
    int fact_size;  /*!< Number of facts */

    const plan_pddl_ground_t *g; /*!< Reference to grounded problem */
};
typedef struct _plan_pddl_inv_finder_t plan_pddl_inv_finder_t;


/**
 * Initializes invariant finder structure.
 */
void planPDDLInvFinderInit(plan_pddl_inv_finder_t *invf,
                           const plan_pddl_ground_t *g);

/**
 * Frees allocated memory.
 */
void planPDDLInvFinderFree(plan_pddl_inv_finder_t *invf);

/**
 * Finds and stores invariants.
 */
void planPDDLInvFinder(plan_pddl_inv_finder_t *invf);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_INV_H__ */
