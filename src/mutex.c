/***
 * maplan
 * -------
 * Copyright (c)2017 Daniel Fiser <danfis@danfis.cz>,
 * AI Center, Department of Computer Science,
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

#include <boruvka/alloc.h>
#include "plan/mutex.h"

void planMutexGroupInit(plan_mutex_group_t *m)
{
    bzero(m, sizeof(*m));
}

void planMutexGroupFree(plan_mutex_group_t *m)
{
    if (m->fact != NULL)
        BOR_FREE(m->fact);
}

static int cmpMutexFact(const void *a, const void *b)
{
    const plan_mutex_fact_t *f1 = a;
    const plan_mutex_fact_t *f2 = b;

    return f1->var - f2->var || f1->val - f2->val;
}

static void planMutexGroupSort(plan_mutex_group_t *m)
{
    qsort(m->fact, m->fact_size, sizeof(plan_mutex_fact_t), cmpMutexFact);
}

void planMutexGroupAdd(plan_mutex_group_t *m,
                       plan_var_id_t var, plan_val_t val)
{
    plan_mutex_fact_t *f;

    if (m->fact_size == m->fact_alloc){
        if (m->fact_alloc == 0)
            m->fact_alloc = 2;
        m->fact_alloc *= 2;
        m->fact = BOR_REALLOC_ARR(m->fact, plan_mutex_fact_t, m->fact_alloc);
    }

    f = m->fact + m->fact_size++;
    f->var = var;
    f->val = val;

    if (m->fact_size >= 2
            && (m->fact[m->fact_size - 2].var > var
                    || (m->fact[m->fact_size - 2].var == var
                            && m->fact[m->fact_size - 2].val > val))){
        planMutexGroupSort(m);
    }
}

void planMutexGroupCopy(plan_mutex_group_t *dst,
                        const plan_mutex_group_t *src)
{
    *dst = *src;
    dst->fact_alloc = dst->fact_size = src->fact_size;
    dst->fact = BOR_ALLOC_ARR(plan_mutex_fact_t, dst->fact_alloc);
    memcpy(dst->fact, src->fact, sizeof(plan_mutex_fact_t) * src->fact_size);
}


void planMutexGroupSetInit(plan_mutex_group_set_t *ms)
{
    bzero(ms, sizeof(*ms));
}

void planMutexGroupSetFree(plan_mutex_group_set_t *ms)
{
    int i;

    for (i = 0; i < ms->group_size; ++i)
        planMutexGroupFree(ms->group + i);
    if (ms->group != NULL)
        BOR_FREE(ms->group);
}

plan_mutex_group_t *planMutexGroupSetAdd(plan_mutex_group_set_t *ms)
{
    if (ms->group_size == ms->group_alloc){
        if (ms->group_alloc == 0)
            ms->group_alloc = 2;
        ms->group_alloc *= 2;
        ms->group = BOR_REALLOC_ARR(ms->group, plan_mutex_group_t,
                                    ms->group_alloc);
    }

    planMutexGroupInit(ms->group + ms->group_size);
    return ms->group + ms->group_size++;
}

void planMutexGroupSetCopy(plan_mutex_group_set_t *dst,
                           const plan_mutex_group_set_t *src)
{
    plan_mutex_group_t *m;
    int i;

    for (i = 0; i < src->group_size; ++i){
        m = planMutexGroupSetAdd(dst);
        planMutexGroupCopy(m, src->group + i);
    }
}

static int cmpGroup(const void *a, const void *b)
{
    const plan_mutex_group_t *g1 = a;
    const plan_mutex_group_t *g2 = b;
    int i;

    for (i = 0; i < g1->fact_size && i < g2->fact_size; ++i){
        if (g1->fact[i].var != g2->fact[i].var)
            return g1->fact[i].var - g2->fact[i].var;
        if (g1->fact[i].val != g2->fact[i].val)
            return g1->fact[i].val - g2->fact[i].val;
    }
    return g1->fact_size - g2->fact_size;
}

void planMutexGroupSetSort(plan_mutex_group_set_t *ms)
{
    qsort(ms->group, ms->group_size, sizeof(plan_mutex_group_t), cmpGroup);
}
