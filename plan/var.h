#ifndef __PLAN_VAR_H__
#define __PLAN_VAR_H__

#include <stdlib.h>
#include <stdio.h>

struct _plan_var_t {
    char *name;
    int range;
    char **fact_name;
    int axiom_layer;
};
typedef struct _plan_var_t plan_var_t;

void planVarInit(plan_var_t *plan);
void planVarFree(plan_var_t *plan);

#endif /* __PLAN_VAR_H__ */

