#include <strings.h>
#include <boruvka/alloc.h>
#include "plan/state.h"

#define INIT_ALLOC_SIZE 32


plan_state_pool_t *planStatePoolNew(size_t num_vars)
{
    plan_state_pool_t *pool;

    pool = BOR_ALLOC(plan_state_pool_t);
    pool->num_vars = num_vars;
    pool->states = borSegmArrNew(pool->num_vars * sizeof(unsigned), 8196);
    pool->num_states = 0;

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

    state.val = borSegmArrGet(pool->states, pool->num_states);
    ++pool->num_states;

    // initialize state
    bzero(state.val, sizeof(unsigned) * pool->num_vars);

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
