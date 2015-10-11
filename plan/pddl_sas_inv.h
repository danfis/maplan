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
#include <plan/pddl_ground.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_sas_inv_fact_t {
    int id;
    int fact_size;    /*!< Number of all facts */
    int *conflict;    /*!< True for the fact that is in conflict */

    plan_pddl_ground_facts_t *edge;       /*!< List of edges (add->del) */
    int edge_size;                        /*!< Number of edges */
};
typedef struct _plan_pddl_sas_inv_fact_t plan_pddl_sas_inv_fact_t;

struct _plan_pddl_sas_inv_edge_t {
    int size;
    int *node;
};
typedef struct _plan_pddl_sas_inv_edge_t plan_pddl_sas_inv_edge_t;

struct _plan_pddl_sas_inv_node_t {
    int id;
    int node_size;    /*!< Number of all nodes */
    int fact_size;    /*!< Number of all facts */
    int *inv;         /*!< True for facts that are invariant candidates */
    int *conflict;    /*!< True for the node that is in conflict */
    int *must;        /*!< True for the node that must be in invariant with
                           this node. */

    plan_pddl_sas_inv_edge_t *edge;
    int edge_size;
    int conflict_node_id;
};
typedef struct _plan_pddl_sas_inv_node_t plan_pddl_sas_inv_node_t;

struct _plan_pddl_sas_inv_t {
    int *fact;
    int size;
    bor_list_t list;
};
typedef struct _plan_pddl_sas_inv_t plan_pddl_sas_inv_t;

struct _plan_pddl_sas_inv_finder_t {
    plan_pddl_sas_inv_fact_t *fact;
    int fact_size;
    plan_pddl_ground_facts_t fact_init;

    plan_pddl_sas_inv_node_t *node;
    int node_size;

    bor_list_t inv;
    int inv_size;
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
