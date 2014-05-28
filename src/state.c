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


/** Returns true if the two given states are equal. */
_bor_inline int planStateEq(const plan_state_pool_t *pool,
                            plan_state_id_t s1,
                            plan_state_id_t s2);

/** Returns hash value of the specified state. */
_bor_inline bor_htable_key_t planStateHash(const plan_state_pool_t *pool,
                                           plan_state_id_t sid);

/** Callbacks for bor_htable_t */
static bor_htable_key_t htableHash(const bor_list_t *key, void *ud);
static int htableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud);

plan_state_pool_t *planStatePoolNew(const plan_var_t *var, size_t var_size)
{
    size_t state_size;
    plan_state_pool_t *pool;
    void *state_init;
    plan_state_htable_t htable_init;

    pool = BOR_ALLOC(plan_state_pool_t);
    pool->num_vars = var_size;

    pool->packer = planStatePackerNew(var, var_size);
    state_size = planStatePackerBufSize(pool->packer);

    pool->data = BOR_ALLOC_ARR(plan_data_arr_t *, 2);

    state_init = BOR_ALLOC_ARR(char, state_size);
    memset(state_init, 0, state_size);
    pool->data[0] = planDataArrNew(state_size, 8196, state_init);
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

    if (pool->packer)
        planStatePackerDel(pool->packer);

    BOR_FREE(pool);
}

plan_state_id_t planStatePoolInsert(plan_state_pool_t *pool,
                                    const plan_state_t *state)
{
    plan_state_id_t sid;
    plan_state_htable_t *htable;
    bor_list_t *hstate;
    const plan_state_htable_t *hfound;
    void *statebuf;

    // determine state ID
    sid = pool->num_states;

    // allocate a new state and initialize it with the given values
    statebuf = planDataArrGet(pool->data[DATA_STATE], sid);
    planStatePackerPack(pool->packer, state, statebuf);

    // allocate and initialize hash table element
    htable = planDataArrGet(pool->data[DATA_HTABLE], sid);
    htable->state_id = sid;

    // insert it into hash table
    hstate = borHTableInsertUnique(pool->htable, &htable->htable);

    if (hstate == NULL){
        // NULL is returned if the element was inserted into table, so
        // increase number of elements in the pool
        ++pool->num_states;
        return sid;

    }else{
        // If the non-NULL was returned, it means that the same state was
        // already in hash table.
        hfound = HTABLE_STATE(hstate);
        return hfound->state_id;
    }
}

plan_state_id_t planStatePoolFind(plan_state_pool_t *pool,
                                  const plan_state_t *state)
{
    plan_state_id_t sid;
    plan_state_htable_t *htable;
    const plan_state_htable_t *hfound;
    bor_list_t *hstate;
    void *statebuf;

    // determine state ID
    sid = pool->num_states;

    // allocate a new state and initialize it with the given values
    statebuf = planDataArrGet(pool->data[DATA_STATE], sid);
    planStatePackerPack(pool->packer, state, statebuf);

    // allocate and initialize hash table element
    htable = planDataArrGet(pool->data[DATA_HTABLE], sid);
    htable->state_id = sid;

    // insert it into hash table
    hstate = borHTableFind(pool->htable, &htable->htable);

    if (hstate == NULL){
        return PLAN_NO_STATE;
    }

    hfound = HTABLE_STATE(hstate);
    return hfound->state_id;
}

void planStatePoolGetState(const plan_state_pool_t *pool,
                           plan_state_id_t sid,
                           plan_state_t *state)
{
    void *statebuf;

    if (sid >= pool->num_states)
        return;

    statebuf = planDataArrGet(pool->data[DATA_STATE], sid);
    planStatePackerUnpack(pool->packer, statebuf, state);
}





/** State Packer **/
plan_state_packer_t *planStatePackerNew(const plan_var_t *var,
                                        size_t var_size)
{
    plan_state_packer_t *p;
    p = BOR_ALLOC(plan_state_packer_t);
    p->num_vars = var_size;
    p->bufsize = sizeof(unsigned) * var_size;
    return p;
}

void planStatePackerDel(plan_state_packer_t *p)
{
    BOR_FREE(p);
}

void planStatePackerPack(const plan_state_packer_t *p,
                         const plan_state_t *state,
                         void *buffer)
{
    size_t i;
    unsigned *buf = (unsigned *)buffer;

    for (i = 0; i < p->num_vars; ++i){
        buf[i] = planStateGet(state, i);
    }
}

void planStatePackerUnpack(const plan_state_packer_t *p,
                           const void *buffer,
                           plan_state_t *state)
{
    size_t i;
    const unsigned *buf = (const unsigned *)buffer;

    for (i = 0; i < p->num_vars; ++i){
        planStateSet(state, i, buf[i]);
    }
}







/** State **/
plan_state_t *planStateNew(plan_state_pool_t *pool)
{
    plan_state_t *state;
    state = BOR_ALLOC(plan_state_t);
    state->val = BOR_ALLOC_ARR(unsigned, pool->num_vars);
    state->num_vars = pool->num_vars;
    return state;
}

void planStateDel(plan_state_pool_t *pool, plan_state_t *state)
{
    BOR_FREE(state->val);
    BOR_FREE(state);
}

void planStateZeroize(plan_state_t *state)
{
    size_t i;
    for (i = 0; i < state->num_vars; ++i)
        state->val[i] = 0;
}






/** Partial State **/

plan_part_state_t *planPartStateNew(plan_state_pool_t *pool)
{
    plan_part_state_t *ps;

    ps = BOR_ALLOC(plan_part_state_t);
    ps->val  = BOR_ALLOC_ARR(unsigned, pool->num_vars);
    ps->mask = BOR_ALLOC_ARR(unsigned, pool->num_vars);

    bzero(ps->val, sizeof(unsigned) * pool->num_vars);
    bzero(ps->mask, sizeof(unsigned) * pool->num_vars);

    return ps;

}

void planPartStateDel(plan_state_pool_t *pool,
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
    void *s1 = planDataArrGet(pool->data[DATA_STATE], s1id);
    void *s2 = planDataArrGet(pool->data[DATA_STATE], s2id);
    size_t size = planStatePackerBufSize(pool->packer);
    return memcmp(s1, s2, size) == 0;
}

_bor_inline bor_htable_key_t planStateHash(const plan_state_pool_t *pool,
                                           plan_state_id_t sid)
{
    void *buf = planDataArrGet(pool->data[DATA_STATE], sid);
    return borCityHash_64(buf, planStatePackerBufSize(pool->packer));
}

static bor_htable_key_t htableHash(const bor_list_t *key, void *ud)
{
    const plan_state_htable_t *s = HTABLE_STATE(key);
    plan_state_pool_t *pool = (plan_state_pool_t *)ud;

    return planStateHash(pool, s->state_id);
}

static int htableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud)
{
    const plan_state_htable_t *s1 = HTABLE_STATE(k1);
    const plan_state_htable_t *s2 = HTABLE_STATE(k2);
    plan_state_pool_t *pool = (plan_state_pool_t *)ud;

    return planStateEq(pool, s1->state_id, s2->state_id);
}
