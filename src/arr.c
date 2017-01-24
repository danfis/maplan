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

#include <stdlib.h>
#include "plan/arr.h"

static int intCmp(const void *a, const void *b)
{
    return *(int *)a - *(int *)b;
}

void planArrIntSort(plan_arr_int_t *arr)
{
    qsort(arr->arr, arr->size, sizeof(int), intCmp);
}

void planArrIntUniq(plan_arr_int_t *arr)
{
    int i, ins;

    if (arr->size <= 1)
        return;

    for (i = ins = 1; i < arr->size; ++i){
        if (arr->arr[i] != arr->arr[i - 1])
            arr->arr[ins++] = arr->arr[i];
    }
    arr->size = ins;
}
