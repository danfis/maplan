#include <string.h>
#include <boruvka/alloc.h>
#include "plan/var.h"

void planVarInit(plan_var_t *plan)
{
    plan->name = NULL;
    plan->range = -1;
}

void planVarFree(plan_var_t *plan)
{
    if (plan->name)
        BOR_FREE(plan->name);
}

void planVarCopy(plan_var_t *dst, const plan_var_t *src)
{
    dst->name = strdup(src->name);
    dst->range = src->range;
}
