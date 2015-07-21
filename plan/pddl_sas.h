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

#ifndef __PLAN_PDDL_SAS_H__
#define __PLAN_PDDL_SAS_H__

#include <plan/state.h>
#include <plan/pddl_ground.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Disable causal graph simplification.
 */
#define PLAN_PDDL_SAS_NO_CG 0x1u

struct _plan_pddl_sas_fact_t {
    int id;
    plan_pddl_ground_facts_t conflict;
    plan_pddl_ground_facts_t single_edge;
    plan_pddl_ground_facts_t *multi_edge;
    int multi_edge_size;
    int in_rank;
    plan_var_id_t var;
    plan_val_t val;
};
typedef struct _plan_pddl_sas_fact_t plan_pddl_sas_fact_t;

struct _plan_pddl_sas_t {
    const plan_pddl_ground_t *ground;

    int *comp;  /*!< Array for tracking which facts are in the component */
    int *close; /*!< True if the corresponding fact is closed */

    plan_pddl_sas_fact_t *fact; /*!< Array of fact related data */
    int fact_size;        /*!< Number of facts */

    bor_extarr_t *inv_pool;
    bor_htable_t *inv_htable;
    int inv_size;
    plan_val_t *var_range;
    plan_var_id_t *var_order;
    int var_size;
    plan_pddl_ground_facts_t init;
    plan_pddl_ground_facts_t goal;
};
typedef struct _plan_pddl_sas_t plan_pddl_sas_t;

void planPDDLSasInit(plan_pddl_sas_t *sas, const plan_pddl_ground_t *g);
void planPDDLSasFree(plan_pddl_sas_t *sas);

void planPDDLSas(plan_pddl_sas_t *sas, unsigned flags);
void planPDDLSasPrintInvariant(const plan_pddl_sas_t *sas,
                               const plan_pddl_ground_t *g,
                               FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_SAS_H__ */
