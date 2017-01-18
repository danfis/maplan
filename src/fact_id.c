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

static void initVar(plan_fact_id_t *fid, const plan_var_t *var, int var_size)
{
    int i, j, id, range;

    // allocate the array corresponding to the variables
    fid->var = BOR_ALLOC_ARR(int *, var_size);

    for (id = 0, i = 0; i < var_size; ++i){
        range = var[i].range;
        if (var[i].ma_privacy){
            // Ignore ma-privacy variables
            fid->var[i] = NULL;
            continue;
        }

        // allocate array for variable's values
        fid->var[i] = BOR_ALLOC_ARR(int, range);

        // fill values with IDs
        for (j = 0; j < range; ++j)
            fid->var[i][j] = id++;
    }

    // remember number of values
    fid->fact_size = id;
    fid->var_size = var_size;
}

void planFactIdInit(plan_fact_id_t *fid, const plan_var_t *var, int var_size,
                    unsigned flags)
{
    int alloc;

    bzero(fid, sizeof(*fid));

    fid->flags = flags;
    initVar(fid, var, var_size);

    alloc = var_size;
    if (flags & PLAN_FACT_ID_H2)
        alloc = (alloc * (var_size - 1)) / 2;
    fid->state_buf = BOR_ALLOC_ARR(int, alloc);
    fid->part_state_buf = BOR_ALLOC_ARR(int, alloc);
}

void planFactIdFree(plan_fact_id_t *fid)
{
    int i;
    for (i = 0; i < fid->var_size; ++i)
        BOR_FREE(fid->var[i]);
    if (fid->var != NULL)
        BOR_FREE(fid->var);
    if (fid->fact_offset != NULL)
        BOR_FREE(fid->fact_offset);
    if (fid->state_buf != NULL)
        BOR_FREE(fid->state_buf);
    if (fid->part_state_buf != NULL)
        BOR_FREE(fid->part_state_buf);
}

int __planFactIdState(const plan_fact_id_t *f, const plan_state_t *state)
{
    int *fact_id, fid, i, j, ins;

    fact_id = f->state_buf;
    for (ins = 0, i = 0; i < state->size; ++i){
        if ((fid = planFactIdVar(f, i, planStateGet(state, i))) != -1)
            fact_id[ins++] = fid;
    }

    if (f->flags & PLAN_FACT_ID_H2){
        for (i = 0; i < state->size; ++i){
            for (j = i + 1; j < state->size; ++j){
                fid = planFactIdVar2(f, i, planStateGet(state, i),
                                        j, planStateGet(state, j));
                if (fid != -1)
                    fact_id[ins++] = fid;
            }
        }
    }

    return ins;
}

int __planFactIdPartState(const plan_fact_id_t *f,
                          const plan_part_state_t *part_state)
{
    int *fact_id, fid, i, j, ins;

    fact_id = f->part_state_buf;
    for (ins = 0, i = 0; i < part_state->vals_size; ++i){
        fid = planFactIdVar(f, part_state->vals[i].var,
                               part_state->vals[i].val);
        if (fid != -1)
            fact_id[ins++] = fid;
    }

    if (f->flags & PLAN_FACT_ID_H2){
        for (i = 0; i < part_state->vals_size; ++i){
            for (j = i + 1; j < part_state->vals_size; ++j){
                fid = planFactIdVar2(f, part_state->vals[i].var,
                                        part_state->vals[i].val,
                                        part_state->vals[j].var,
                                        part_state->vals[j].val);
                if (fid != -1)
                    fact_id[ins++] = fid;
            }
        }
    }

    return ins;
}
