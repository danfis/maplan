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
#include <plan/pddl_sas_inv.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * If set, causal graph will be used for fine tuning problem definition.
 */
#define PLAN_PDDL_SAS_USE_CG 0x1u

struct _plan_pddl_sas_fact_t {
    plan_var_id_t var; /*!< Assigned variable ID */
    plan_val_t val;    /*!< Assigned value */
};
typedef struct _plan_pddl_sas_fact_t plan_pddl_sas_fact_t;

struct _plan_pddl_sas_t {
    plan_pddl_sas_inv_finder_t inv_finder;

    plan_pddl_sas_fact_t *fact; /*!< Array of fact related data */
    int fact_size;              /*!< Number of facts */

    plan_val_t *var_range;    /*!< Range for each variable */
    plan_var_id_t *var_order; /*!< Order of variables as suggested by
                                   causal-graph analysis. */
    int var_size;             /*!< Number of variables */
};
typedef struct _plan_pddl_sas_t plan_pddl_sas_t;

void planPDDLSasFree(plan_pddl_sas_t *sas);

void planPDDLSas(plan_pddl_sas_t *sas, const plan_pddl_ground_t *g,
                 unsigned flags);
void planPDDLSasPrintInvariant(const plan_pddl_sas_t *sas,
                               const plan_pddl_ground_t *g,
                               const plan_pddl_t *pddl,
                               FILE *fout);
void planPDDLSasPrintFacts(const plan_pddl_sas_t *sas,
                           const plan_pddl_ground_t *g,
                           const plan_pddl_t *pddl,
                           FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_SAS_H__ */
