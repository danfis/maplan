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

#include <string.h>
#include <boruvka/alloc.h>
#include "plan/part_state.h"

/** Performs bit operation c = a AND b. */
_bor_inline void bitAnd(const void *a, const void *b, int size, void *c);
/** Performs bit operator c = (a AND ~m) OR b. */
_bor_inline void bitApplyWithMask(const void *a, const void *m, const void *b,
                                  int size, void *c);

plan_part_state_t *planPartStateNew(int size)
{
    plan_part_state_t *ps;
    ps = BOR_ALLOC(plan_part_state_t);
    planPartStateInit(ps, size);
    return ps;
}

void planPartStateDel(plan_part_state_t *part_state)
{
    planPartStateFree(part_state);
    BOR_FREE(part_state);
}

plan_part_state_t *planPartStateClone(const plan_part_state_t *p)
{
    plan_part_state_t *ps;
    ps = BOR_ALLOC(plan_part_state_t);
    planPartStateInit(ps, p->size);
    planPartStateCopy(ps, p);
    return ps;
}

void planPartStateInit(plan_part_state_t *ps, int size)
{
    ps->size = size;

    ps->valbuf = NULL;
    ps->maskbuf = NULL;
    ps->bufsize = 0;
    ps->vals = NULL;
    ps->vals_size = 0;
}

void planPartStateFree(plan_part_state_t *part_state)
{
    if (part_state->valbuf)
        BOR_FREE(part_state->valbuf);
    if (part_state->maskbuf)
        BOR_FREE(part_state->maskbuf);
    if (part_state->vals)
        BOR_FREE(part_state->vals);
}

void planPartStateCopy(plan_part_state_t *dst, const plan_part_state_t *src)
{
    plan_var_id_t var;
    plan_val_t val;
    int i;

    PLAN_PART_STATE_FOR_EACH(src, i, var, val){
        planPartStateSet(dst, var, val);
    }
}

static int valsCmp(const void *_a, const void *_b)
{
    const plan_part_state_pair_t *a = _a;
    const plan_part_state_pair_t *b = _b;
    return a->var - b->var;
}

void planPartStateSet(plan_part_state_t *state,
                      plan_var_id_t var,
                      plan_val_t val)
{
    if (state->valbuf){
        BOR_FREE(state->valbuf);
        BOR_FREE(state->maskbuf);
        state->valbuf = state->maskbuf = NULL;
        state->bufsize = 0;
    }

    ++state->vals_size;
    state->vals = BOR_REALLOC_ARR(state->vals,
                                  plan_part_state_pair_t,
                                  state->vals_size);
    state->vals[state->vals_size - 1].var = var;
    state->vals[state->vals_size - 1].val = val;
    qsort(state->vals, state->vals_size, sizeof(plan_part_state_pair_t), valsCmp);
}

void planPartStateUnset(plan_part_state_t *state, plan_var_id_t var)
{
    int i;

    if (state->valbuf){
        BOR_FREE(state->valbuf);
        BOR_FREE(state->maskbuf);
        state->valbuf = state->maskbuf = NULL;
        state->bufsize = 0;
    }

    for (i = 0; i < state->vals_size; ++i){
        if (state->vals[i].var == var){
            state->vals[i] = state->vals[state->vals_size - 1];
            --state->vals_size;
            break;
        }
    }

    if (state->vals_size == 0 && state->vals){
        BOR_FREE(state->vals);
        state->vals = NULL;
    }else if (state->vals){
        state->vals = BOR_REALLOC_ARR(state->vals,
                                      plan_part_state_pair_t,
                                      state->vals_size);
        qsort(state->vals, state->vals_size,
              sizeof(plan_part_state_pair_t), valsCmp);
    }
}

void planPartStateToState(const plan_part_state_t *part_state,
                          plan_state_t *state)
{
    int i;

    for (i = 0; i < part_state->size; ++i)
        state->val[i] = PLAN_VAL_UNDEFINED;
    for (i = 0; i < part_state->vals_size; ++i)
        state->val[part_state->vals[i].var] = part_state->vals[i].val;
}

int planPartStateIsSubsetPackedState(const plan_part_state_t *part_state,
                                     const void *bufstate)
{
    void *masked_state;
    int size, cmp;

    // prepare temporary buffer
    size = part_state->bufsize;
    masked_state = BOR_ALLOC_ARR(char, size);

    // mask out values we are not interested in
    bitAnd(bufstate, part_state->maskbuf, size, masked_state);

    // compare resulting buffers
    cmp = memcmp(masked_state, part_state->valbuf, size);

    // free temporary buffer
    BOR_FREE(masked_state);

    return cmp == 0;
}

int planPartStateIsSubsetState(const plan_part_state_t *part_state,
                               const plan_state_t *state)
{
    plan_var_id_t var;
    plan_val_t val;
    int i;

    // TODO
    PLAN_PART_STATE_FOR_EACH(part_state, i, var, val){
        if (planStateGet(state, var) != val)
            return 0;
    }

    return 1;
}

int planPartStateEq(const plan_part_state_t *ps1,
                    const plan_part_state_t *ps2)
{
    int i;

    if (ps1->vals_size != ps2->vals_size)
        return 0;

    for (i = 0; i < ps1->vals_size; ++i){
        if (ps1->vals[i].var != ps2->vals[i].var
                || ps1->vals[i].val != ps2->vals[i].val){
            return 0;
        }
    }
    return 1;
}

void planPartStateUpdateState(const plan_part_state_t *part_state,
                              plan_state_t *state)
{
    plan_var_id_t var;
    plan_val_t val;
    int i;

    PLAN_PART_STATE_FOR_EACH(part_state, i, var, val){
        planStateSet(state, var, val);
    }
}

void planPartStateUpdatePackedState(const plan_part_state_t *ps,
                                    void *statebuf)
{
    bitApplyWithMask(statebuf, ps->maskbuf, ps->valbuf,
                     ps->bufsize, statebuf);
}

void planPartStateCreatePackedState(const plan_part_state_t *ps,
                                    const void *src_statebuf,
                                    void *dst_statebuf)
{
    bitApplyWithMask(src_statebuf, ps->maskbuf, ps->valbuf,
                     ps->bufsize, dst_statebuf);
}

_bor_inline void bitAnd(const void *a, const void *b, int size, void *c)
{
    const uint32_t *a32, *b32;
    uint32_t *c32;
    const uint8_t *a8, *b8;
    uint8_t *c8;
    int size32, size8;

    size32 = size / 4;
    a32 = a;
    b32 = b;
    c32 = c;
    for (; size32 != 0; --size32, ++a32, ++b32, ++c32){
        *c32 = *a32 & *b32;
    }

    size8 = size % 4;
    a8 = (uint8_t *)a32;
    b8 = (uint8_t *)b32;
    c8 = (uint8_t *)c32;
    for (; size8 != 0; --size8, ++a8, ++b8, ++c8){
        *c8 = *a8 & *b8;
    }
}

_bor_inline void bitApplyWithMask(const void *a, const void *m, const void *b,
                                  int size, void *c)
{
    const uint32_t *a32, *b32, *m32;
    uint32_t *c32;
    const uint8_t *a8, *b8, *m8;
    uint8_t *c8;
    int size32, size8;

    size32 = size / 4;
    a32 = a;
    b32 = b;
    m32 = m;
    c32 = c;
    for (; size32 != 0; --size32, ++a32, ++b32, ++c32, ++m32){
        *c32 = (*a32 & ~*m32) | *b32;
    }

    size8 = size % 4;
    a8 = (uint8_t *)a32;
    b8 = (uint8_t *)b32;
    m8 = (uint8_t *)m32;
    c8 = (uint8_t *)c32;
    for (; size8 != 0; --size8, ++a8, ++b8, ++c8, ++m8){
        *c8 = (*a8 & ~*m8) | *b8;
    }
}
