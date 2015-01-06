#include <string.h>
#include <boruvka/alloc.h>
#include "plan/var.h"

void planVarInit(plan_var_t *var, const char *name, plan_val_t range)
{
    var->name = strdup(name);
    var->range = range;
    var->is_private = BOR_CALLOC_ARR(int, var->range);
}

void planVarFree(plan_var_t *plan)
{
    if (plan->name)
        BOR_FREE(plan->name);
    if (plan->is_private)
        BOR_FREE(plan->is_private);
}

void planVarCopy(plan_var_t *dst, const plan_var_t *src)
{
    dst->name = strdup(src->name);
    dst->range = src->range;

    dst->is_private = NULL;
    if (src->is_private){
        dst->is_private = BOR_ALLOC_ARR(int, dst->range);
        memcpy(dst->is_private, src->is_private, sizeof(int) * dst->range);
    }
}
