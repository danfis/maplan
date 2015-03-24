#ifndef __PLAN_VAR_H__
#define __PLAN_VAR_H__

#include <stdlib.h>
#include <stdio.h>

#include <plan/common.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_var_t {
    char *name;          /*!< Name of the variable */
    plan_val_t range;    /*!< Number of values in variable */
    int *is_val_private; /*!< Flag for each value signal whether it is
                              private */
    int is_private;      /*!< True if all values are private */
};
typedef struct _plan_var_t plan_var_t;

/**
 * Initializes a variable structure.
 */
void planVarInit(plan_var_t *var, const char *name, plan_val_t range);

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

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_VAR_H__ */
