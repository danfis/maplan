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
#include <plan/mutex.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PLAN_FA_MUTEX_ONLY_GOAL 0x1u

void planFAMutexFind(const plan_problem_t *p, const plan_state_t *state,
                     plan_mutex_group_set_t *ms, unsigned flags);


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_FA_MUTEX_H__ */
