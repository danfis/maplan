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

#ifndef __PLAN_PDDL_PRED_H__
#define __PLAN_PDDL_PRED_H__

#include <plan/pddl_require.h>
#include <plan/pddl_type.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_predicate_t {
    const char *name;
    int *param;
    int param_size;
};
typedef struct _plan_pddl_predicate_t plan_pddl_predicate_t;

struct _plan_pddl_predicates_t {
    plan_pddl_predicate_t *pred;
    int size;
    int eq_pred;
};
typedef struct _plan_pddl_predicates_t plan_pddl_predicates_t;

/**
 * Parse :predicates from domain PDDL.
 */
int planPDDLPredicatesParse(const plan_pddl_lisp_t *domain,
                            unsigned require,
                            const plan_pddl_types_t *types,
                            plan_pddl_predicates_t *ps);

/**
 * Parse :functions from domain PDDL.
 */
int planPDDLFunctionsParse(const plan_pddl_lisp_t *domain,
                           const plan_pddl_types_t *types,
                           plan_pddl_predicates_t *ps);

/**
 * Frees allocated resources.
 */
void planPDDLPredicatesFree(plan_pddl_predicates_t *ps);

/**
 * Returns ID of the predicate with the specified name.
 */
int planPDDLPredicatesGet(const plan_pddl_predicates_t *ps, const char *name);

void planPDDLPredicatesPrint(const plan_pddl_predicates_t *ps,
                             const char *title, FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_PRED_H__ */
