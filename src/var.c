#include <string.h>
#include <boruvka/alloc.h>
#include "plan/var.h"

void planVarInit(plan_var_t *var, const char *name, plan_val_t range)
{
    var->name = BOR_STRDUP(name);
    var->range = range;
    var->is_val_private = BOR_CALLOC_ARR(int, var->range);
    var->is_private = 0;
    var->val_name = BOR_CALLOC_ARR(char *, var->range);
}

void planVarFree(plan_var_t *var)
{
    int i;

    if (var->name)
        BOR_FREE(var->name);
    if (var->is_val_private)
        BOR_FREE(var->is_val_private);
    if (var->val_name){
        for (i = 0; i < var->range; ++i){
            if (var->val_name[i])
                BOR_FREE(var->val_name[i]);
        }
        BOR_FREE(var->val_name);
    }
}

void planVarCopy(plan_var_t *dst, const plan_var_t *src)
{
    int i;

    dst->name = BOR_STRDUP(src->name);
    dst->range = src->range;
    dst->is_private = src->is_private;

    dst->is_val_private = NULL;
    if (src->is_val_private){
        dst->is_val_private = BOR_ALLOC_ARR(int, dst->range);
        memcpy(dst->is_val_private, src->is_val_private,
               sizeof(int) * dst->range);
    }

    dst->val_name = NULL;
    if (src->val_name){
        dst->val_name = BOR_CALLOC_ARR(char *, dst->range);
        for (i = 0; i < dst->range; ++i){
            if (src->val_name[i])
                dst->val_name[i] = BOR_STRDUP(src->val_name[i]);
        }
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

void planVarSetPrivate(plan_var_t *var)
{
    plan_val_t i;
    for (i = 0; i < var->range; ++i)
        var->is_val_private[i] = 1;
    var->is_private = 1;
}

void planVarSetValName(plan_var_t *var, plan_val_t val, const char *name)
{
    if (var->val_name[val] != NULL)
        BOR_FREE(var->val_name[val]);
    var->val_name[val] = BOR_STRDUP(name);
}
