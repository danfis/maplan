#include <strings.h>
#include <boruvka/alloc.h>
#include "plan/state.h"


plan_state_pool_t *planStatePoolNew(const plan_var_t *var, size_t var_size)
{
    plan_state_pool_t *pool;

    pool = BOR_ALLOC(plan_state_pool_t);
    pool->num_vars = var_size;
    pool->states = borSegmArrNew(pool->num_vars * sizeof(unsigned), 8196);
    pool->allocated = 0;

    return pool;
}

void planStatePoolDel(plan_state_pool_t *pool)
{
    if (pool->states)
        borSegmArrDel(pool->states);
    BOR_FREE(pool);
}

plan_state_t planStatePoolNewState(plan_state_pool_t *pool)
{
    plan_state_t state;

    state.val = borSegmArrGet(pool->states, pool->allocated);
    ++pool->allocated;

    // initialize state
    bzero(state.val, sizeof(unsigned) * pool->num_vars);

    return state;
}

plan_part_state_t planStatePoolNewPartState(plan_state_pool_t *pool)
{
    plan_part_state_t state;

    state.val  = borSegmArrGet(pool->states, pool->allocated++);
    state.mask = borSegmArrGet(pool->states, pool->allocated++);

    // initialize state
    bzero(state.val, sizeof(unsigned) * pool->num_vars);
    bzero(state.mask, sizeof(unsigned) * pool->num_vars);

    return state;
}



int planStateGet(const plan_state_t *state, unsigned var)
{
    return state->val[var];
}

void planStateSet(plan_state_t *state, unsigned var, unsigned val)
{
    state->val[var] = val;
}



int planPartStateGet(const plan_part_state_t *state, unsigned var)
{
    return state->val[var];
}

void planPartStateSet(plan_part_state_t *state, unsigned var, unsigned val)
{
    state->val[var] = val;
    state->mask[var] = ~0u;
}
