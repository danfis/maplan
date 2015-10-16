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

#ifndef __PLAN_PDDL_SAS_INV_FACT_H__
#define __PLAN_PDDL_SAS_INV_FACT_H__

#include <plan/pddl_ground.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_sas_inv_fact_t {
    int id;
    int fact_size;    /*!< Number of all facts */
    int *conflict;    /*!< True for the conflicting facts. */

    plan_pddl_ground_facts_t *edge;       /*!< List of edges (add->del) */
    int edge_size;                        /*!< Number of edges */
};
typedef struct _plan_pddl_sas_inv_fact_t plan_pddl_sas_inv_fact_t;

struct _plan_pddl_sas_inv_facts_t {
    plan_pddl_sas_inv_fact_t *fact;
    int fact_size;
};
typedef struct _plan_pddl_sas_inv_facts_t plan_pddl_sas_inv_facts_t;

/**
 * Initialize facts structure of given size.
 */
void planPDDLSasInvFactsInit(plan_pddl_sas_inv_facts_t *fs, int size);

/**
 * Frees allocated memory.
 */
void planPDDLSasInvFactsFree(plan_pddl_sas_inv_facts_t *fs);

/**
 * Adds edges between all facts in from and all facts in to.
 */
void planPDDLSasInvFactsAddEdges(plan_pddl_sas_inv_facts_t *fs,
                                 const plan_pddl_ground_facts_t *from,
                                 const plan_pddl_ground_facts_t *to);

/**
 * Adds conflicts between facts.
 */
void planPDDLSasInvFactsAddConflicts(plan_pddl_sas_inv_facts_t *fs,
                                     const plan_pddl_ground_facts_t *facts);

/**
 * Makes the fact to be in conflict with all other facts.
 */
void planPDDLSasInvFactsAddConflictsWithAll(plan_pddl_sas_inv_facts_t *fs,
                                            int fact_id);

/**
 * Returns true if the facts for an invariant.
 */
int planPDDLSasInvFactsCheckInvariant(const plan_pddl_sas_inv_facts_t *fs,
                                      const int *facts_bool);

/**
 * Checks merged invariant.
 */
int planPDDLSasInvFactsCheckInvariant2(const plan_pddl_sas_inv_facts_t *fs,
                                       const int *facts_bool1,
                                       const int *facts_bool2);

/**
 * Prints a fact -- for debugging.
 */
void planPDDLSasInvFactPrint(const plan_pddl_sas_inv_fact_t *fact, FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_SAS_INV_FACT_H__ */
