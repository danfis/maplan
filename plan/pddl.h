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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Requirements
 */
#define PLAN_PDDL_REQUIRE_STRIPS                0x000001u
#define PLAN_PDDL_REQUIRE_TYPING                0x000002u
#define PLAN_PDDL_REQUIRE_NEGATIVE_PRE          0x000004u
#define PLAN_PDDL_REQUIRE_DISJUNCTIVE_PRE       0x000008u
#define PLAN_PDDL_REQUIRE_EQUALITY              0x000010u
#define PLAN_PDDL_REQUIRE_EXISTENTIAL_PRE       0x000020u
#define PLAN_PDDL_REQUIRE_UNIVERSAL_PRE         0x000040u
#define PLAN_PDDL_REQUIRE_CONDITIONAL_EFF       0x000080u
#define PLAN_PDDL_REQUIRE_NUMERIC_FLUENT        0x000100u
#define PLAN_PDDL_REQUIRE_OBJECT_FLUENT         0x000200u
#define PLAN_PDDL_REQUIRE_DURATIVE_ACTION       0x000400u
#define PLAN_PDDL_REQUIRE_DURATION_INEQUALITY   0x000800u
#define PLAN_PDDL_REQUIRE_CONTINUOUS_EFF        0x001000u
#define PLAN_PDDL_REQUIRE_DERIVED_PRED          0x002000u
#define PLAN_PDDL_REQUIRE_TIMED_INITIAL_LITERAL 0x004000u
#define PLAN_PDDL_REQUIRE_PREFERENCE            0x008000u
#define PLAN_PDDL_REQUIRE_CONSTRAINT            0x010000u
#define PLAN_PDDL_REQUIRE_ACTION_COST           0x020000u
#define PLAN_PDDL_REQUIRE_MULTI_AGENT           0x040000u
#define PLAN_PDDL_REQUIRE_UNFACTORED_PRIVACY    0x080000u
#define PLAN_PDDL_REQUIRE_FACTORED_PRIVACY      0x100000u


#define PLAN_PDDL_COND_NONE     0
#define PLAN_PDDL_COND_AND      1
#define PLAN_PDDL_COND_OR       2
#define PLAN_PDDL_COND_NOT      3
#define PLAN_PDDL_COND_IMPLY    4
#define PLAN_PDDL_COND_EXISTS   5
#define PLAN_PDDL_COND_FORALL   6
#define PLAN_PDDL_COND_WHEN     7
#define PLAN_PDDL_COND_INCREASE 50
#define PLAN_PDDL_COND_PRED     100
#define PLAN_PDDL_COND_CONST    101
#define PLAN_PDDL_COND_VAR      102
#define PLAN_PDDL_COND_INT      103


struct _plan_pddl_type_t {
    const char *name;
    int parent;
};
typedef struct _plan_pddl_type_t plan_pddl_type_t;

struct _plan_pddl_obj_t {
    const char *name;
    int type;
    int is_constant;
};
typedef struct _plan_pddl_obj_t plan_pddl_obj_t;

struct _plan_pddl_predicate_t {
    const char *name;
    int *param;
    int param_size;
};
typedef struct _plan_pddl_predicate_t plan_pddl_predicate_t;

typedef struct _plan_pddl_cond_t plan_pddl_cond_t;
struct _plan_pddl_cond_t {
    int type;
    int val;
    plan_pddl_cond_t *arg;
    int arg_size;
};

struct _plan_pddl_action_t {
    const char *name;
    plan_pddl_obj_t *param;
    int param_size;
    plan_pddl_cond_t pre;
    plan_pddl_cond_t eff;
};
typedef struct _plan_pddl_action_t plan_pddl_action_t;

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
    plan_pddl_type_t *type;
    int type_size;
    plan_pddl_obj_t *obj;
    int obj_size;
    plan_pddl_predicate_t *predicate;
    int predicate_size;
    plan_pddl_action_t *action;
    int action_size;
    plan_pddl_cond_t goal;

    plan_pddl_obj_arr_t *type_obj_map;

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
