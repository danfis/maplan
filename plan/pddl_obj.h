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

#ifndef __PLAN_PDDL_OBJ_H__
#define __PLAN_PDDL_OBJ_H__

#include <plan/pddl_type.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_obj_t {
    const char *name;
    int type;
    int is_constant;
};
typedef struct _plan_pddl_obj_t plan_pddl_obj_t;

struct _plan_pddl_objs_t {
    plan_pddl_obj_t *obj;
    int size;
};
typedef struct _plan_pddl_objs_t plan_pddl_objs_t;

/**
 * Parse :constants and :objects from domain and problem PDDLs.
 */
int planPDDLObjsParse(const plan_pddl_lisp_t *domain,
                      const plan_pddl_lisp_t *problem,
                      const plan_pddl_types_t *types,
                      plan_pddl_objs_t *objs);

/**
 * Frees allocated resources.
 */
void planPDDLObjsFree(plan_pddl_objs_t *objs);

/**
 * Returns ID of the object of the specified name.
 */
int planPDDLObjsGet(const plan_pddl_objs_t *objs, const char *name);

/**
 * Print formated objects.
 */
void planPDDLObjsPrint(const plan_pddl_objs_t *objs, FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_OBJ_H__ */
