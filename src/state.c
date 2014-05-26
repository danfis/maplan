#include <strings.h>
#include <boruvka/hfunc.h>
#include <boruvka/alloc.h>
#include "plan/state.h"


bor_htable_key_t htableHash(const bor_list_t *key, void *ud)
{
    const plan_state_t *s = BOR_LIST_ENTRY(key, const plan_state_t, htable);
    plan_state_pool_t *pool = (plan_state_pool_t *)ud;
    void *arr = ((char *)s) + sizeof(*s);

    return borCityHash_64(arr, sizeof(unsigned) * pool->num_vars);
}

int htableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud)
{
    const plan_state_t *s1 = BOR_LIST_ENTRY(k1, const plan_state_t, htable);
    const plan_state_t *s2 = BOR_LIST_ENTRY(k1, const plan_state_t, htable);
    plan_state_pool_t *pool = (plan_state_pool_t *)ud;

    return planStateEq(pool, s1, s2);
}

plan_state_pool_t *planStatePoolNew(const plan_var_t *var, size_t var_size)
{
    plan_state_pool_t *pool;

    pool = BOR_ALLOC(plan_state_pool_t);
    pool->num_vars = var_size;
    pool->state_size  = sizeof(plan_state_t);
    pool->state_size += pool->num_vars * sizeof(unsigned);
    pool->states = borSegmArrNew(pool->state_size, 8196);
    pool->num_states = 0;
    pool->htable = borHTableNew(htableHash, htableEq, (void *)pool);

    return pool;
}

void planStatePoolDel(plan_state_pool_t *pool)
{
    if (pool->htable)
        borHTableDel(pool->htable);
    if (pool->states)
        borSegmArrDel(pool->states);

    BOR_FREE(pool);
}

const plan_state_t *planStatePoolFind(plan_state_pool_t *pool,
                                      const plan_state_t *state)
{
    bor_list_t *sref;

    sref = borHTableFind(pool->htable, &state->htable);
    if (sref == NULL)
        return NULL;
    return BOR_LIST_ENTRY(sref, const plan_state_t, htable);
}

const plan_state_t *planStatePoolInsert(plan_state_pool_t *pool,
                                        const plan_state_t *state)
{
    size_t bucket;
    bor_list_t *sref;
    plan_state_t *news;

    // compute bucket in advance because we may need it during insertion
    bucket = borHTableBucket(pool->htable, &state->htable);

    // Try to find a state within the computed bucket and if it is find
    // just return it.
    sref = borHTableFindBucket(pool->htable, bucket, &state->htable);
    if (sref != NULL)
        return BOR_LIST_ENTRY(sref, const plan_state_t, htable);

    // create a new state in segmented array
    news = (plan_state_t *)borSegmArrGet(pool->states, pool->num_states);
    ++pool->num_states;

    // set variable values
    memcpy(news, state, pool->state_size);
    borListInit(&news->htable);

    // and insert it into hash table
    borHTableInsertBucket(pool->htable, bucket, &news->htable);

    return news;
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
