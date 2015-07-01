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

#ifndef __PLAN_PDDL_ACTION_H__
#define __PLAN_PDDL_ACTION_H__

#include <plan/pddl_lisp.h>
#include <plan/pddl_obj.h>
#include <plan/pddl_fact.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_cond_eff_t {
    plan_pddl_facts_t pre;
    plan_pddl_facts_t eff;
};
typedef struct _plan_pddl_cond_eff_t plan_pddl_cond_eff_t;

struct _plan_pddl_cond_effs_t {
    plan_pddl_cond_eff_t *cond_eff;
    int size;
};
typedef struct _plan_pddl_cond_effs_t plan_pddl_cond_effs_t;

struct _plan_pddl_action_t {
    const char *name;
    plan_pddl_objs_t param;
    plan_pddl_facts_t pre;
    plan_pddl_facts_t eff;
    plan_pddl_facts_t cost;
    plan_pddl_cond_effs_t cond_eff;
};
typedef struct _plan_pddl_action_t plan_pddl_action_t;

struct _plan_pddl_actions_t {
    plan_pddl_action_t *action;
    int size;
};
typedef struct _plan_pddl_actions_t plan_pddl_actions_t;

/**
 * Parses actions from domain PDDL.
 */
int planPDDLActionsParse(const plan_pddl_lisp_t *domain,
                         const plan_pddl_types_t *types,
                         const plan_pddl_objs_t *objs,
                         const plan_pddl_type_obj_t *type_obj,
                         const plan_pddl_predicates_t *predicates,
                         const plan_pddl_predicates_t *functions,
                         unsigned require,
                         plan_pddl_actions_t *actions);

void planPDDLActionsFree(plan_pddl_actions_t *actions);

plan_pddl_action_t *planPDDLActionsAdd(plan_pddl_actions_t *as);

void planPDDLActionsPrint(const plan_pddl_actions_t *actions,
                          const plan_pddl_objs_t *objs,
                          const plan_pddl_predicates_t *predicates,
                          const plan_pddl_predicates_t *functions,
                          FILE *fout);
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_ACTION_H__ */
