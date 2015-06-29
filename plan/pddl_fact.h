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

#ifndef __PLAN_PDDL_FACT_H__
#define __PLAN_PDDL_FACT_H__

#include <plan/pddl_lisp.h>
#include <plan/pddl_obj.h>
#include <plan/pddl_predicate.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_fact_t {
    int pred;     /*!< Predicate ID */
    int *arg;     /*!< Object IDs are arguments */
    int arg_size; /*!< Number of arguments */
    int neg;      /*!< True if it is negated form */
    int func_val; /*!< Assigned value in case of function */
};
typedef struct _plan_pddl_fact_t plan_pddl_fact_t;

struct _plan_pddl_facts_t {
    plan_pddl_fact_t *fact;
    int size;
};
typedef struct _plan_pddl_facts_t plan_pddl_facts_t;

/**
 * Parses :init into list of instantiated predicates and instantiated
 * functions.
 */
int planPDDLFactsParseInit(const plan_pddl_lisp_t *problem,
                           const plan_pddl_predicates_t *predicates,
                           const plan_pddl_predicates_t *functions,
                           const plan_pddl_objs_t *objs,
                           plan_pddl_facts_t *init_fact,
                           plan_pddl_facts_t *init_func);

/**
 * Free allocated resources.
 */
void planPDDLFactsFree(plan_pddl_facts_t *fs);

/**
 * Adds another fact to array.
 */
plan_pddl_fact_t *planPDDLFactsAdd(plan_pddl_facts_t *fs);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_FACT_H__ */
