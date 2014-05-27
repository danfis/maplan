#include <strings.h>
#include <boruvka/hfunc.h>
#include <boruvka/alloc.h>
#include "plan/state.h"


#define DATA_STATE 0
#define DATA_HTABLE 1

#define HTABLE_STATE(list) \
    BOR_LIST_ENTRY(list, const plan_state_htable_t, htable)

struct _plan_state_htable_t {
    bor_list_t htable;
    plan_state_id_t state_id;
};
typedef struct _plan_state_htable_t plan_state_htable_t;

typedef unsigned plan_state_t;


_bor_inline int planStateEq(const plan_state_pool_t *pool,
                            plan_state_id_t s1,
                            plan_state_id_t s2);

_bor_inline const plan_state_t *planStatePoolGet(const plan_state_pool_t *p,
                                                 plan_state_id_t id);

bor_htable_key_t htableHash(const bor_list_t *key, void *ud)
{
    const plan_state_htable_t *s = HTABLE_STATE(key);
    plan_state_pool_t *pool = (plan_state_pool_t *)ud;
    const plan_state_t *state = planStatePoolGet(pool, s->state_id);

    return borCityHash_64(state, pool->state_size);
}

int htableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud)
{
    const plan_state_htable_t *s1 = HTABLE_STATE(k1);
    const plan_state_htable_t *s2 = HTABLE_STATE(k2);
    plan_state_pool_t *pool = (plan_state_pool_t *)ud;

    return planStateEq(pool, s1->state_id, s2->state_id);
}

plan_state_pool_t *planStatePoolNew(const plan_var_t *var, size_t var_size)
{
    plan_state_pool_t *pool;
    void *state_init;
    plan_state_htable_t htable_init;

    pool = BOR_ALLOC(plan_state_pool_t);
    pool->num_vars = var_size;
    pool->state_size  = sizeof(plan_state_t);
    pool->state_size += pool->num_vars * sizeof(unsigned);

    pool->data = BOR_ALLOC_ARR(plan_data_arr_t *, 2);

    state_init = BOR_ALLOC_ARR(char, pool->state_size);
    memset(state_init, 0, pool->state_size);
    pool->data[0] = planDataArrNew(pool->state_size, 8196, state_init);
    BOR_FREE(state_init);

    borListInit(&htable_init.htable);
    htable_init.state_id = -1;
    pool->data[1] = planDataArrNew(sizeof(plan_state_htable_t), 8196,
                                   &htable_init);

    pool->data_size = 2;
    pool->htable = borHTableNew(htableHash, htableEq, (void *)pool);
    pool->num_states = 0;

    return pool;
}

void planStatePoolDel(plan_state_pool_t *pool)
{
    size_t i;

    if (pool->htable)
        borHTableDel(pool->htable);

    for (i = 0; i < pool->data_size; ++i){
        planDataArrDel(pool->data[i]);
    }

    BOR_FREE(pool);
}

plan_state_id_t planStatePoolCreate(plan_state_pool_t *pool,
                                    unsigned *values)
{
    plan_state_id_t sid;
    plan_state_t *state;
    plan_state_htable_t *htable;
    size_t i;

    // determine state ID
    sid = pool->num_states;

    // allocate a new state and initialize it with the given values
    state = planDataArrGet(pool->data[DATA_STATE], sid);
    for (i = 0; i < pool->num_vars; ++i){
        state[i] = values[i];
    }

    // allocate and initialize hash table element
    htable = planDataArrGet(pool->data[DATA_HTABLE], sid);
    htable->state_id = sid;

    // insert hash table element into hash table
    borHTableInsert(pool->htable, &htable->htable);

    // increase number of state currently stored in data pool
    ++pool->num_states;

    return sid;
}

unsigned planStatePoolStateVal(const plan_state_pool_t *pool,
                               plan_state_id_t id,
                               unsigned var)
{
    plan_state_t *state;
    state = planDataArrGet(pool->data[DATA_STATE], id);
    return state[var];
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



_bor_inline int planStateEq(const plan_state_pool_t *pool,
                            plan_state_id_t s1id,
                            plan_state_id_t s2id)
{
    const plan_state_t *s1 = planStatePoolGet(pool, s1id);
    const plan_state_t *s2 = planStatePoolGet(pool, s2id);
    return memcmp(s1, s2, pool->state_size) == 0;
}

_bor_inline const plan_state_t *planStatePoolGet(const plan_state_pool_t *p,
                                                 plan_state_id_t id)
{
    return planDataArrGet(p->data[DATA_STATE], id);
}

