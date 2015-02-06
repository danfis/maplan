#include <stdarg.h>
#include <boruvka/alloc.h>
#include "plan/state.h"

plan_state_t *planStateNew(int size)
{
    plan_state_t *state;
    state = BOR_ALLOC(plan_state_t);
    state->val = BOR_ALLOC_ARR(plan_val_t, size);
    state->size = size;
    return state;
}

void planStateDel(plan_state_t *state)
{
    BOR_FREE(state->val);
    BOR_FREE(state);
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
