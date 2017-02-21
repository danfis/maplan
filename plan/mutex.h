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

#ifndef __PLAN_MUTEX_H__
#define __PLAN_MUTEX_H__

#include <plan/problem.h>
#include <plan/arr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_mutex_fact_t {
    plan_var_id_t var;
    plan_val_t val;
};
typedef struct _plan_mutex_fact_t plan_mutex_fact_t;

struct _plan_mutex_group_t {
    plan_mutex_fact_t *fact;
    int fact_alloc;
    int fact_size;
    int is_goal; /*!< True if the mutex group contains a fact from goal */
    int is_fa;   /*!< True if the mutex group is fact-alternating */
};
typedef struct _plan_mutex_group_t plan_mutex_group_t;

void planMutexGroupInit(plan_mutex_group_t *m);
void planMutexGroupFree(plan_mutex_group_t *m);
void planMutexGroupAdd(plan_mutex_group_t *m,
                       plan_var_id_t var, plan_val_t val);
void planMutexGroupCopy(plan_mutex_group_t *dst,
                        const plan_mutex_group_t *src);

struct _plan_mutex_group_set_t {
    plan_mutex_group_t *group;
    int group_alloc;
    int group_size;
};
typedef struct _plan_mutex_group_set_t plan_mutex_group_set_t;

void planMutexGroupSetInit(plan_mutex_group_set_t *ms);
void planMutexGroupSetFree(plan_mutex_group_set_t *ms);
plan_mutex_group_t *planMutexGroupSetAdd(plan_mutex_group_set_t *ms);
void planMutexGroupSetCopy(plan_mutex_group_set_t *dst,
                           const plan_mutex_group_set_t *src);
void planMutexGroupSetSort(plan_mutex_group_set_t *ms);


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MUTEX_H__ */
