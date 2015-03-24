#include <string.h>
#include <boruvka/alloc.h>
#include "plan/var.h"

void planVarInit(plan_var_t *var, const char *name, plan_val_t range)
{
    var->name = strdup(name);
    var->range = range;
    var->is_val_private = BOR_CALLOC_ARR(int, var->range);
    var->is_private = 0;
}

void planVarFree(plan_var_t *plan)
{
    if (plan->name)
        BOR_FREE(plan->name);
    if (plan->is_val_private)
        BOR_FREE(plan->is_val_private);
}

void planVarCopy(plan_var_t *dst, const plan_var_t *src)
{
    dst->name = strdup(src->name);
    dst->range = src->range;
    dst->is_private = src->is_private;

    dst->is_val_private = NULL;
    if (src->is_val_private){
        dst->is_val_private = BOR_ALLOC_ARR(int, dst->range);
        memcpy(dst->is_val_private, src->is_val_private,
               sizeof(int) * dst->range);
    }
}

static int _isAllPrivate(plan_var_t *var)
{
    plan_val_t i;
    for (i = 0; i < var->range; ++i){
        if (!var->is_val_private[i])
            return 0;
    }
    return 1;
}

void planVarSetPrivateVal(plan_var_t *var, plan_val_t val)
{
    var->is_val_private[val] = 1;
    if (_isAllPrivate(var))
        var->is_private = 1;
}

