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

#ifndef __PLAN_FA_MUTEX_H__
#define __PLAN_FA_MUTEX_H__

#include <plan/problem.h>
#include <plan/arr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_fa_mutex_set_t {
    plan_arr_int_t *fa_mutex;
    int alloc;
    int size;
};
typedef struct _plan_fa_mutex_set_t plan_fa_mutex_set_t;

void planFAMutexSetInit(plan_fa_mutex_set_t *ms);
void planFAMutexSetFree(plan_fa_mutex_set_t *ms);
void planFAMutexSetSort(plan_fa_mutex_set_t *ms);
void planFAMutexSetClone(plan_fa_mutex_set_t *dst,
                         const plan_fa_mutex_set_t *src);
void planFAMutexAddFromVars(plan_fa_mutex_set_t *ms,
                            const plan_var_t *var, int var_size);

void planFAMutexFind(const plan_problem_t *p, const plan_state_t *state,
                     plan_fa_mutex_set_t *ms);


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_FA_MUTEX_H__ */
