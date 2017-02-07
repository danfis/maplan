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

#include <string.h>
#include <boruvka/alloc.h>
#include "plan/var.h"

void planVarInit(plan_var_t *var, const char *name, plan_val_t range)
{
    var->name = BOR_STRDUP(name);
    var->range = range;
    var->is_private = 0;
    var->ma_privacy = 0;

    var->val = BOR_CALLOC_ARR(plan_var_val_t, var->range);
}

void planVarInitMAPrivacy(plan_var_t *var)
{
    var->name = NULL;
    var->range = INT_MAX;
    var->val = NULL;
    var->is_private = 0;
    var->ma_privacy = 1;
}

void planVarFree(plan_var_t *var)
{
    int i;

    if (var->name)
        BOR_FREE(var->name);

    if (var->val){
        for (i = 0; i < var->range; ++i){
            if (var->val[i].name)
                BOR_FREE(var->val[i].name);
        }
        BOR_FREE(var->val);
    }
}

void planVarCopy(plan_var_t *dst, const plan_var_t *src)
{
    int i;

    dst->name = BOR_STRDUP(src->name);
    dst->range = src->range;
    dst->is_private = src->is_private;
    dst->ma_privacy = src->ma_privacy;

    dst->val = NULL;
    if (src->val){
        dst->val = BOR_CALLOC_ARR(plan_var_val_t, dst->range);
        memcpy(dst->val, src->val, sizeof(plan_var_val_t) * dst->range);
        for (i = 0; i < dst->range; ++i){
            if (src->val[i].name)
                dst->val[i].name = BOR_STRDUP(src->val[i].name);
        }
    }
}

static int _isAllPrivate(plan_var_t *var)
{
    plan_val_t i;
    for (i = 0; i < var->range; ++i){
        if (!var->val[i].is_private)
            return 0;
    }
    return 1;
}

void planVarSetPrivateVal(plan_var_t *var, plan_val_t val)
{
    var->val[val].is_private = 1;
    if (_isAllPrivate(var))
        var->is_private = 1;
}

void planVarSetPrivate(plan_var_t *var)
{
    plan_val_t i;
    for (i = 0; i < var->range; ++i)
        var->val[i].is_private = 1;
    var->is_private = 1;
}

void planVarSetValName(plan_var_t *var, plan_val_t val, const char *name)
{
    if (var->val[val].name != NULL)
        BOR_FREE(var->val[val].name);
    var->val[val].name = BOR_STRDUP(name);
}

void planVarSetValUsedBy(plan_var_t *var, plan_val_t val, int used_by)
{
    uint64_t set;
    set = (1u << used_by);
    var->val[val].used_by |= set;
}

_bor_inline int popcount(uint64_t v)
{
    uint64_t w = v;
    int cnt = 0;

    while (w != 0){
        cnt += (int)(w & 1u);
        w >>= 1u;
    }

    return cnt;
}

static int setValPrivateFromUsedBy(plan_var_val_t *val)
{
    if (popcount(val->used_by) <= 1){
        val->is_private = 1;
        return 1;
    }
    return 0;
}

void planVarSetPrivateFromUsedBy(plan_var_t *var)
{
    int i, num_private;

    num_private = 0;
    for (i = 0; i < var->range; ++i)
        num_private += setValPrivateFromUsedBy(var->val + i);

    if (num_private == var->range)
        var->is_private = 1;
}
