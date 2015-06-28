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

#ifndef __PLAN_PDDL_TYPE_H__
#define __PLAN_PDDL_TYPE_H__

#include <plan/pddl_lisp.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_type_t {
    const char *name;
    int parent;
};
typedef struct _plan_pddl_type_t plan_pddl_type_t;

struct _plan_pddl_types_t {
    plan_pddl_type_t *type;
    int size;
};
typedef struct _plan_pddl_types_t plan_pddl_types_t;

/**
 * Parses :types into type array.
 */
int planPDDLTypesParse(const plan_pddl_lisp_t *domain,
                       plan_pddl_types_t *types);

/**
 * Frees allocated resources.
 */
void planPDDLTypesFree(plan_pddl_types_t *types);

/**
 * Returns ID of the type corresponding to the name.
 */
int planPDDLTypesGet(const plan_pddl_types_t *t, const char *name);

/**
 * Prints list of types to the specified output.
 */
void planPDDLTypesPrint(const plan_pddl_types_t *t, FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_TYPE_H__ */
