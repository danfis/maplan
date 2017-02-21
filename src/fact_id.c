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

#include "plan/fact_id.h"

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
    fid->fact1_size = id;
    fid->var_size = var_size;
}

static void extendVar2(plan_fact_id_t *f, const plan_var_t *var, int var_size)
{
    int var_id, val_id, fid, f1, fact1_size;

    if (var_size <= 1)
        return;

    fid = fact1_size = f->fact_size;
    f->fact_offset = BOR_ALLOC_ARR(int, f->var[var_size - 1][0]);
    for (f1 = 0, var_id = 0; var_id < var_size - 1; ++var_id){
        for (val_id = 0; val_id < var[var_id].range; ++val_id, ++f1){
            f->fact_offset[f1] = fid - f->var[var_id + 1][0];
            fid += fact1_size - f->var[var_id + 1][0];
        }
    }
    f->fact_size = fid;
}

static void factToVarMap(plan_fact_id_t *f,
                         const plan_var_t *var, int var_size)
{
    plan_var_id_t vi;
    plan_val_t val;
    int fact_id;

    f->fact_to_var = BOR_ALLOC_ARR(plan_var_id_t, f->fact1_size);
    f->fact_to_val = BOR_ALLOC_ARR(plan_val_t, f->fact1_size);
    for (vi = 0; vi < var_size; ++vi){
        if (var[vi].ma_privacy)
            continue;

        for (val = 0; val < var[vi].range; ++val){
            fact_id = planFactIdVar(f, vi, val);
            f->fact_to_var[fact_id] = vi;
            f->fact_to_val[fact_id] = val;
        }
    }
}

void planFactIdInit(plan_fact_id_t *fid, const plan_var_t *var, int var_size,
                    unsigned flags)
{
    int alloc;

    bzero(fid, sizeof(*fid));

    fid->flags = flags;
    initVar(fid, var, var_size);
    if (flags & PLAN_FACT_ID_H2)
        extendVar2(fid, var, var_size);

    if (flags & PLAN_FACT_ID_REVERSE_MAP)
        factToVarMap(fid, var, var_size);

    alloc = var_size;
    if (flags & PLAN_FACT_ID_H2)
        alloc = (alloc * (var_size + 1)) / 2;
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
    if (fid->fact_to_var)
        BOR_FREE(fid->fact_to_var);
    if (fid->fact_to_val)
        BOR_FREE(fid->fact_to_val);
    if (fid->state_buf != NULL)
        BOR_FREE(fid->state_buf);
    if (fid->part_state_buf != NULL)
        BOR_FREE(fid->part_state_buf);
}

void planFactIdFromFactId(const plan_fact_id_t *f, int fid,
                          int *fid1, int *fid2)
{
    int i, id;

    if (fid < f->fact1_size || fid >= f->fact_size){
        *fid1 = *fid2 = fid;
        return;
    }

    for (i = 0; i < f->fact1_size; ++i){
        id = fid - f->fact_offset[i];
        if (id < f->fact1_size){
            *fid1 = i;
            *fid2 = id;
            return;
        }
    }
}

static int __planFactIdState2(const plan_fact_id_t *f,
                              const plan_state_t *state, int *fact_id)
{
    int fid, i, j, ins;

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

static int __planFactIdPartState2(const plan_fact_id_t *f,
                                  const plan_part_state_t *part_state,
                                  int *fact_id)
{
    int fid, i, j, ins;

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

int __planFactIdState(const plan_fact_id_t *f, const plan_state_t *state)
{
    return __planFactIdState2(f, state, f->state_buf);
}

int __planFactIdPartState(const plan_fact_id_t *f,
                          const plan_part_state_t *part_state)
{
    return __planFactIdPartState2(f, part_state, f->part_state_buf);
}

int *planFactIdState2(const plan_fact_id_t *f, const plan_state_t *state,
                      int *size)
{
    int alloc, *arr;

    alloc = f->var_size;
    if (f->flags & PLAN_FACT_ID_H2)
        alloc = (alloc * (f->var_size + 1)) / 2;
    arr = BOR_ALLOC_ARR(int, alloc);
    *size = __planFactIdState2(f, state, arr);
    return arr;
}

int *planFactIdPartState2(const plan_fact_id_t *f,
                          const plan_part_state_t *part_state,
                          int *size)
{
    int alloc, *arr;

    alloc = part_state->vals_size;
    if (f->flags & PLAN_FACT_ID_H2)
        alloc = (alloc * (part_state->vals_size + 1)) / 2;
    arr = BOR_ALLOC_ARR(int, alloc);
    *size = __planFactIdPartState2(f, part_state, arr);
    return arr;
}
