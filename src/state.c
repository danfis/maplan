#include <strings.h>
#include <boruvka/alloc.h>
#include "plan/state.h"


plan_state_pool_t *planStatePoolNew(const plan_var_t *var, size_t var_size)
{
    plan_state_pool_t *pool;

    pool = BOR_ALLOC(plan_state_pool_t);
    pool->num_vars = var_size;
    pool->state_size  = sizeof(plan_state_t);
    pool->state_size += pool->num_vars * sizeof(unsigned);
    pool->states = borSegmArrNew(pool->state_size, 8196);
    pool->num_states = 0;

    return pool;
}

void planStatePoolDel(plan_state_pool_t *pool)
{
    if (pool->states)
        borSegmArrDel(pool->states);
    BOR_FREE(pool);
}

ssize_t planStatePoolFind(plan_state_pool_t *pool,
                          const plan_state_t *state)
{
    const plan_state_t *s;
    size_t i;

    for (i = 0; i < pool->num_states; ++i){
        s = (plan_state_t *)borSegmArrGet(pool->states, i);
        if (planStateEq(pool, s, state))
            return i;
    }
    return -1;
}

size_t planStatePoolInsert(plan_state_pool_t *pool,
                           const plan_state_t *state)
{
    ssize_t id;
    plan_state_t *news;

    id = planStatePoolFind(pool, state);
    if (id >= 0)
        return id;

    news = (plan_state_t *)borSegmArrGet(pool->states, pool->num_states);
    ++pool->num_states;

    memcpy(news, state, pool->state_size);
    borListInit(&news->htable);

    return pool->num_states - 1;
}

const plan_state_t *planStatePoolGet(const plan_state_pool_t *pool, size_t id)
{
    if (id >= pool->num_states)
        return NULL;

    return (plan_state_t *)borSegmArrGet(pool->states, id);
}

plan_state_t *planStatePoolCreateState(plan_state_pool_t *pool)
{
    plan_state_t *state;

    state = (plan_state_t *)BOR_ALLOC_ARR(char, pool->state_size);
    bzero(state, pool->state_size);
    borListInit(&state->htable);

    return state;
}

void planStatePoolDestroyState(plan_state_pool_t *pool,
                               plan_state_t *state)
{
    BOR_FREE(state);
}

plan_part_state_t *planStatePoolCreatePartState(plan_state_pool_t *pool)
{
    plan_part_state_t *ps;

    ps = BOR_ALLOC(plan_part_state_t);
    ps->val  = BOR_ALLOC_ARR(unsigned, pool->num_vars);
    ps->mask = BOR_ALLOC_ARR(unsigned, pool->num_vars);

    bzero(ps->val, sizeof(unsigned) * pool->num_vars);
    bzero(ps->mask, sizeof(unsigned) * pool->num_vars);

    return ps;

}

void planStatePoolDestroyPartState(plan_state_pool_t *pool,
                                   plan_part_state_t *part_state)
{
    BOR_FREE(part_state->val);
    BOR_FREE(part_state->mask);
    BOR_FREE(part_state);
}



int planStateGet(const plan_state_t *state, unsigned var)
{
    unsigned *arr = (unsigned *)(((char *)state) + sizeof(*state));
    return arr[var];
}

void planStateSet(plan_state_t *state, unsigned var, unsigned val)
{
    unsigned *arr = (unsigned *)(((char *)state) + sizeof(*state));
    arr[var] = val;
}

void planStateZeroize(const plan_state_pool_t *pool, plan_state_t *state)
{
    unsigned *arr = (unsigned *)(((char *)state) + sizeof(*state));
    size_t i;

    for (i = 0; i < pool->num_vars; ++i)
        arr[i] = 0;
}

int planStateEq(const plan_state_pool_t *pool,
                const plan_state_t *s1, const plan_state_t *s2)
{
    void *arr1 = ((char *)s1) + sizeof(*s1);
    void *arr2 = ((char *)s2) + sizeof(*s2);
    size_t size;

    size = sizeof(unsigned) * pool->num_vars;

    if (memcmp(arr1, arr2, size) == 0)
        return 1;
    return 0;
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

int planPartStateIsSet(const plan_part_state_t *state, unsigned var)
{
    return state->mask[var] == ~0u;
}
