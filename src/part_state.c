#include <string.h>
#include <boruvka/alloc.h>
#include "plan/part_state.h"

/** Performs bit operation c = a AND b. */
static void bitAnd(const void *a, const void *b, int size, void *c);

plan_part_state_t *planPartStateNew(int size)
{
    plan_part_state_t *ps;
    int i;

    ps = BOR_ALLOC(plan_part_state_t);
    ps->size = size;
    ps->val    = BOR_ALLOC_ARR(plan_val_t, size);
    ps->is_set = BOR_ALLOC_ARR(int, size);

    for (i = 0; i < size; ++i){
        ps->val[i] = PLAN_VAL_UNDEFINED;
        ps->is_set[i] = 0;
    }

    ps->valbuf = NULL;
    ps->maskbuf = NULL;
    ps->bufsize = 0;
    ps->vals = NULL;
    ps->vals_size = 0;

    return ps;
}

void planPartStateDel(plan_part_state_t *part_state)
{
    if (part_state->val)
        BOR_FREE(part_state->val);
    if (part_state->is_set)
        BOR_FREE(part_state->is_set);
    if (part_state->valbuf)
        BOR_FREE(part_state->valbuf);
    if (part_state->maskbuf)
        BOR_FREE(part_state->maskbuf);
    if (part_state->vals)
        BOR_FREE(part_state->vals);
    BOR_FREE(part_state);
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
    state->val[var] = val;
    state->is_set[var] = 1;

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

    state->val[var] = PLAN_VAL_UNDEFINED;
    state->is_set[var] = 0;

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
    memcpy(state->val, part_state->val,
           sizeof(plan_val_t) * part_state->size);
}

int planPartStateIsSubset(const plan_part_state_t *ps1,
                          const plan_part_state_t *ps2)
{
    char buf[ps1->bufsize];

    // TODO: Alternative version for unpacked part-states
    // TODO: Check ps1->valbuf and ps2->valbuf?
    if (!ps1->valbuf || !ps2->valbuf){
        fprintf(stderr, "Fatal Error: Part-state is not packed!"
                        " (planPartStateIsSubset)\n");
        exit(-1);
    }

    bitAnd(ps1->maskbuf, ps2->maskbuf, ps1->bufsize, buf);
    if (memcmp(buf, ps1->maskbuf, ps1->bufsize) != 0)
        return 0;

    bitAnd(ps1->valbuf, ps2->valbuf, ps1->bufsize, buf);
    if (memcmp(buf, ps1->valbuf, ps1->bufsize) != 0)
        return 0;

    return 1;
}

int planPartStateEq(const plan_part_state_t *ps1,
                    const plan_part_state_t *ps2)
{
    // TODO: Alternative version for unpacked part-states
    // TODO: Check ps1->valbuf and ps2->valbuf?
    if (!ps1->valbuf || !ps2->valbuf){
        fprintf(stderr, "Fatal Error: Part-state is not packed!"
                        " (planPartStateEq)\n");
        exit(-1);
    }

    if (memcmp(ps1->maskbuf, ps2->maskbuf, ps1->bufsize) == 0
            && memcmp(ps1->valbuf, ps2->valbuf, ps1->bufsize) == 0)
        return 1;
    return 0;
}




static void bitAnd(const void *a, const void *b, int size, void *c)
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
