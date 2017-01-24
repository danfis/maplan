/***
 * maplan
 * -------
 * Copyright (c)2017 Daniel Fiser <danfis@danfis.cz>,
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

#ifndef __PLAN_ARR_H__
#define __PLAN_ARR_H__

#include <boruvka/varr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BOR_VARR_DECL(int, plan_arr_int_t, planArrInt)

/**
 * Sorts the array.
 */
void planArrIntSort(plan_arr_int_t *arr);

/**
 * Same as uniq(1)
 */
void planArrIntUniq(plan_arr_int_t *arr);

#define PLAN_ARR_INT_FOR_EACH(ARR, VAL) \
    for (int ___i = 0, ___size = (ARR)->size, *___a = (ARR)->arr; \
         ___i < ___size && ((VAL) = ___a[___i], 1); ++___i)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_ARR_H__ */
