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

#include <boruvka/alloc.h>
#include "plan/problem_2.h"

void planFactId2Init(plan_fact_id_2_t *f, const plan_var_t *var, int var_size)
{
    int var_id, val_id, fid, f1;

    bzero(f, sizeof(*f));
    f->var = BOR_ALLOC_ARR(int *, var_size);
    f->var_size = var_size;

    for (fid = 0, var_id = 0; var_id < var_size; ++var_id){
        f->var[var_id] = BOR_ALLOC_ARR(int, var[var_id].range);
        for (val_id = 0; val_id < var[var_id].range; ++val_id, ++fid)
            f->var[var_id][val_id] = fid;
    }
    f->fact1_size = fid;

    if (var_size > 1){
        f->fact2 = BOR_ALLOC_ARR(int, f->var[var_size - 1][0]);
        fprintf(stderr, "fact2-size: %d\n", f->var[var_size - 1][0]);
        for (f1 = 0, var_id = 0; var_id < var_size - 1; ++var_id){
            for (val_id = 0; val_id < var[var_id].range; ++val_id, ++f1){
                f->fact2[f1] = fid - f->var[var_id + 1][0];
                fid += f->fact1_size - f->var[var_id + 1][0];
            }
        }
    }

    f->fact_size = fid;
}

void planFactId2Free(plan_fact_id_2_t *f)
{
    int i;

    for (i = 0; i < f->var_size; ++i){
        if (f->var[i] != NULL)
            BOR_FREE(f->var[i]);
    }
    if (f->var != NULL)
        BOR_FREE(f->var);
    if (f->fact2 != NULL)
        BOR_FREE(f->fact2);
}

int planFactId2Var(const plan_fact_id_2_t *f,
                   plan_var_id_t var, plan_val_t val)
{
    return f->var[var][val];
}

int planFactId2Var2(const plan_fact_id_2_t *f,
                    plan_var_id_t var1, plan_val_t val1,
                    plan_var_id_t var2, plan_val_t val2)
{
    if (var1 == var2 && val1 == val2)
        return planFactId2Var(f, var1, val1);
    return planFactId2Fact2(f, planFactId2Var(f, var1, val1),
                               planFactId2Var(f, var2, val2));
}

int planFactId2Fact2(const plan_fact_id_2_t *f, int fid1, int fid2)
{
    if (fid1 < fid2){
        return fid2 + f->fact2[fid1];
    }else{
        return fid1 + f->fact2[fid2];
    }
}

int *planFactId2State(const plan_fact_id_2_t *f, const plan_state_t *state,
                      int *size)
{
    int *fact_id, i, j, ins;

    *size = (state->size * (state->size + 1)) / 2;
    fact_id = BOR_ALLOC_ARR(int, *size);
    for (ins = 0, i = 0; i < state->size; ++i)
        fact_id[ins++] = planFactId2Var(f, i, planStateGet(state, i));

    for (i = 0; i < state->size; ++i){
        for (j = i + 1; j < state->size; ++j){
            fact_id[ins++] = planFactId2Var2(f, i, planStateGet(state, i),
                                                j, planStateGet(state, j));
        }
    }

    return fact_id;
}

int *planFactId2PartState(const plan_fact_id_2_t *f,
                          const plan_part_state_t *part_state, int *size)
{
    int *fact_id, i, j, ins;

    *size = part_state->vals_size;
    *size *= part_state->vals_size + 1;
    *size /= 2;
    fact_id = BOR_ALLOC_ARR(int, *size);

    for (ins = 0, i = 0; i < part_state->vals_size; ++i)
        fact_id[ins++] = planFactId2Var(f, part_state->vals[i].var,
                                           part_state->vals[i].val);

    for (i = 0; i < part_state->vals_size; ++i){
        for (j = i + 1; j < part_state->vals_size; ++j){
            fact_id[ins++] = planFactId2Var2(f, part_state->vals[i].var,
                                                part_state->vals[i].val,
                                                part_state->vals[j].var,
                                                part_state->vals[j].val);
        }
    }

    return fact_id;
}
