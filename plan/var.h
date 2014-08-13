#ifndef __PLAN_VAR_H__
#define __PLAN_VAR_H__

#include <stdlib.h>
#include <stdio.h>

#include <plan/common.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_var_t {
    char *name;
    plan_val_t range;
    char **fact_name;
    int axiom_layer;
};
typedef struct _plan_var_t plan_var_t;

void planVarInit(plan_var_t *plan);
void planVarFree(plan_var_t *plan);
void planVarCopy(plan_var_t *dst, const plan_var_t *src);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_VAR_H__ */
