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

#include <boruvka/alloc.h>

#include "fact_id.h"

void planFactIdInit(plan_fact_id_t *fid, const plan_var_t *var, int var_size)
{
    int i, j, id, range;

    // allocate the array corresponding to the variables
    fid->fact_id = BOR_ALLOC_ARR(int *, var_size);

    for (id = 0, i = 0; i < var_size; ++i){
        range = var[i].range;
        if (var[i].ma_privacy){
            // Ignore ma-privacy variables
            fid->fact_id[i] = NULL;
            continue;
        }

        // allocate array for variable's values
        fid->fact_id[i] = BOR_ALLOC_ARR(int, range);

        // fill values with IDs
        for (j = 0; j < range; ++j)
            fid->fact_id[i][j] = id++;
    }

    // remember number of values
    fid->fact_size = id;
    fid->var_size = var_size;
}

void planFactIdFree(plan_fact_id_t *fid)
{
    int i;
    for (i = 0; i < fid->var_size; ++i)
        BOR_FREE(fid->fact_id[i]);
    BOR_FREE(fid->fact_id);
}
