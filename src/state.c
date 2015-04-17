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

#include <stdarg.h>
#include <boruvka/alloc.h>
#include "plan/state.h"

plan_state_t *planStateNew(int size)
{
    plan_state_t *state;
    state = BOR_ALLOC(plan_state_t);
    planStateInit(state, size);
    return state;
}

void planStateDel(plan_state_t *state)
{
    planStateFree(state);
    BOR_FREE(state);
}

void planStateInit(plan_state_t *state, int size)
{
    state->val = BOR_ALLOC_ARR(plan_val_t, size);
    state->size = size;
    state->state_id = PLAN_NO_STATE;
}

void planStateFree(plan_state_t *state)
{
    if (state->val)
        BOR_FREE(state->val);
}

void planStateZeroize(plan_state_t *state)
{
    int i;
    for (i = 0; i < state->size; ++i)
        state->val[i] = 0;
}

void planStateSet2(plan_state_t *state, int n, ...)
{
    va_list ap;
    int i, val;

    va_start(ap, n);
    for (i = 0; i < n; ++i){
        val = va_arg(ap, int);
        state->val[i] = val;
    }
    va_end(ap);
}

void planStatePrint(const plan_state_t *state, FILE *fout)
{
    int i;

    fprintf(fout, "State: [");
    for (i = 0; i < state->size; ++i){
        fprintf(fout, " %d", (int)state->val[i]);
    }
    fprintf(fout, "]\n");
}

void planStateCopy(plan_state_t *dst, const plan_state_t *src)
{
    int i;

    for (i = 0; i < src->size; ++i)
        dst->val[i] = src->val[i];
}
