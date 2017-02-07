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

#ifndef __PLAN_VAR_H__
#define __PLAN_VAR_H__

#include <stdlib.h>
#include <stdio.h>

#include <plan/common.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_var_val_t {
    char *name;       /*!< Name of the value (i.e., corresponding fact) */
    int is_private;   /*!< True if the value is private */
    uint64_t used_by; /*!< Bit array recording agents that use this value */
};
typedef struct _plan_var_val_t plan_var_val_t;

struct _plan_var_t {
    char *name;          /*!< Name of the variable */
    plan_val_t range;    /*!< Number of values in variable */
    int is_private;      /*!< True if all values are private */
    int ma_privacy;      /*!< True if the variable is used for state
                              privacy preserving in ma mode */

    plan_var_val_t *val; /*!< Info about each value within variable */
};
typedef struct _plan_var_t plan_var_t;

/**
 * Initializes a variable structure.
 */
void planVarInit(plan_var_t *var, const char *name, plan_val_t range);

/**
 * Initialize an artificial variable
 */
void planVarInitMAPrivacy(plan_var_t *var);

/**
 * Frees allocated resources.
 */
void planVarFree(plan_var_t *plan);

/**
 * Copies variable from src to dst.
 */
void planVarCopy(plan_var_t *dst, const plan_var_t *src);

/**
 * Sets the specified value as private.
 */
void planVarSetPrivateVal(plan_var_t *var, plan_val_t val);

/**
 * Sets whole variable as private
 */
void planVarSetPrivate(plan_var_t *var);

/**
 * Sets name of the value.
 */
void planVarSetValName(plan_var_t *var, plan_val_t val, const char *name);

/**
 * Sets value as used-by the specified agent.
 */
void planVarSetValUsedBy(plan_var_t *var, plan_val_t val, int used_by);

/**
 * Sets variable's values as private if their .used_by contains at most one
 * bit set to 1. Then it sets the whole variable as private if all values
 * are private.
 */
void planVarSetPrivateFromUsedBy(plan_var_t *var);

/**
 * Returns true if the value is used by the specified agent.
 */
_bor_inline int planVarValIsUsedBy(const plan_var_t *var, plan_val_t val,
                                   int used_by);


/**** INLINES: ****/
_bor_inline int planVarValIsUsedBy(const plan_var_t *var, plan_val_t val,
                                   int used_by)
{
    uint64_t mask = (1u << used_by);
    return (var->val[val].used_by & mask);
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_VAR_H__ */
