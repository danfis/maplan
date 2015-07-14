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

#include <plan/pddl_ground.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


struct _plan_pddl_sas_fact_t {
    int id;
    plan_pddl_ground_facts_t conflict;
    plan_pddl_ground_facts_t single_edge;
    plan_pddl_ground_facts_t *multi_edge;
    int multi_edge_size;
    int in_rank;
};
typedef struct _plan_pddl_sas_fact_t plan_pddl_sas_fact_t;

struct _plan_pddl_sas_t {
    int *comp;  /*!< Array for tracking which facts are in the component */
    int *close; /*!< True if the corresponding fact is closed */

    plan_pddl_sas_fact_t *fact; /*!< Array of fact related data */
    int fact_size;        /*!< Number of facts */

    int *is_in_invariant;
    plan_pddl_ground_facts_t *invariant;
    int invariant_size;
};
typedef struct _plan_pddl_sas_t plan_pddl_sas_t;

void planPDDLSasInit(plan_pddl_sas_t *sas, const plan_pddl_ground_t *g);
void planPDDLSasFree(plan_pddl_sas_t *sas);

void planPDDLSas(plan_pddl_sas_t *sas);
void planPDDLSasPrintInvariant(const plan_pddl_sas_t *sas,
                               const plan_pddl_ground_t *g,
                               FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_SAS_H__ */
