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

#ifndef __PLAN_PDDL_H__
#define __PLAN_PDDL_H__

#include <plan/pddl_lisp.h>
#include <plan/pddl_require.h>
#include <plan/pddl_type.h>
#include <plan/pddl_obj.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_predicate_t {
    const char *name;
    int *param;
    int param_size;
};
typedef struct _plan_pddl_predicate_t plan_pddl_predicate_t;

struct _plan_pddl_fact_t {
    int pred;     /*!< Predicate ID */
    int *arg;     /*!< Object IDs are arguments */
    int arg_size; /*!< Number of arguments */
    int neg;      /*!< True if it is negated form */
};
typedef struct _plan_pddl_fact_t plan_pddl_fact_t;

struct _plan_pddl_inst_func_t {
    int func; /*!< Function ID */
    int val;  /*!< Assigned value */
    int *arg; /*!< Object IDs are arguments */
    int arg_size;
};
typedef struct _plan_pddl_inst_func_t plan_pddl_inst_func_t;

typedef struct _plan_pddl_action_t plan_pddl_action_t;
struct _plan_pddl_action_t {
    const char *name;
    plan_pddl_obj_t *param;
    int param_size;
    plan_pddl_fact_t *pre;
    int pre_size;
    plan_pddl_fact_t *eff;
    int eff_size;
    plan_pddl_action_t *cond_eff;
    int cond_eff_size;
    plan_pddl_inst_func_t *cost;
    int cost_size;
};

struct _plan_pddl_obj_arr_t {
    int *obj;
    int size;
};
typedef struct _plan_pddl_obj_arr_t plan_pddl_obj_arr_t;

struct _plan_pddl_t {
    plan_pddl_lisp_t *domain_lisp;
    plan_pddl_lisp_t *problem_lisp;
    const char *domain_name;
    const char *problem_name;
    unsigned require;
    plan_pddl_types_t type;
    plan_pddl_objs_t obj;
    plan_pddl_type_obj_t type_obj;
    plan_pddl_predicate_t *predicate;
    int predicate_size;
    int eq_pred_id;
    plan_pddl_predicate_t *function;
    int function_size;
    plan_pddl_action_t *action;
    int action_size;
    plan_pddl_fact_t *goal;
    int goal_size;
    plan_pddl_fact_t *init_fact;
    int init_fact_size;
    plan_pddl_inst_func_t *init_func;
    int init_func_size;
    int metric;

    struct {
        plan_pddl_obj_t *param;
        int *param_bind;
        int param_size;
    } forall;
};
typedef struct _plan_pddl_t plan_pddl_t;

plan_pddl_t *planPDDLNew(const char *domain_fn, const char *problem_fn);
void planPDDLDel(plan_pddl_t *pddl);
void planPDDLDump(const plan_pddl_t *pddl, FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_H__ */
