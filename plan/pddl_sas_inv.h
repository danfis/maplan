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

#ifndef __PLAN_PDDL_SAS_INV_H__
#define __PLAN_PDDL_SAS_INV_H__

#include <boruvka/list.h>
#include <boruvka/htable.h>
#include <plan/pddl_ground.h>
#include <plan/pddl_sas_inv_fact.h>
#include <plan/pddl_sas_inv_node.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_sas_inv_t {
    int *fact;
    int size;
    bor_list_t list;

    bor_list_t table;
    bor_htable_key_t table_key;
};
typedef struct _plan_pddl_sas_inv_t plan_pddl_sas_inv_t;

struct _plan_pddl_sas_inv_finder_t {
    plan_pddl_sas_inv_facts_t facts;
    plan_pddl_ground_facts_t fact_init;

    bor_htable_t *inv_table;
    bor_list_t inv;
    int inv_size;
    const plan_pddl_ground_t *g;
};
typedef struct _plan_pddl_sas_inv_finder_t plan_pddl_sas_inv_finder_t;


/**
 * Initializes invariants structure.
 */
void planPDDLSasInvFinderInit(plan_pddl_sas_inv_finder_t *invf,
                              const plan_pddl_ground_t *g);

/**
 * Frees allocated memory.
 */
void planPDDLSasInvFinderFree(plan_pddl_sas_inv_finder_t *invf);

/**
 * Finds and stores invariants.
 */
void planPDDLSasInvFinder(plan_pddl_sas_inv_finder_t *invf);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_SAS_INV_H__ */
