#include <strings.h>
#include <boruvka/hfunc.h>
#include <boruvka/alloc.h>
#include "plan/state_pool.h"


#define DATA_STATE 0
#define DATA_HTABLE 1

#define HTABLE_STATE(list) \
    BOR_LIST_ENTRY(list, plan_state_htable_t, htable)

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

/** Performs bit operation c = a AND b. */
static void bitAnd(const void *a, const void *b, int size, void *c);
/** Performs bit operator c = (a AND ~m) OR b. */
static void bitApplyWithMask(const void *a, const void *m, const void *b,
                             int size, void *c);


/** Callbacks for bor_htable_t */
static bor_htable_key_t htableHash(const bor_list_t *key, void *ud);
static int htableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud);

plan_state_pool_t *planStatePoolNew(const plan_var_t *var, int var_size)
{
    int state_size;
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
    pool->data[0] = planDataArrNew(state_size, NULL, state_init);
    BOR_FREE(state_init);

    htable_init.state_id = PLAN_NO_STATE;
    pool->data[1] = planDataArrNew(sizeof(plan_state_htable_t),
                                   NULL, &htable_init);

    pool->data_size = 2;
    pool->htable = borHTableNew(htableHash, htableEq, (void *)pool);
    pool->num_states = 0;

    return pool;
}

void planStatePoolDel(plan_state_pool_t *pool)
{
    int i;

    if (pool->htable)
        borHTableDel(pool->htable);

    for (i = 0; i < pool->data_size; ++i){
        planDataArrDel(pool->data[i]);
    }
    BOR_FREE(pool->data);

    if (pool->packer)
        planStatePackerDel(pool->packer);

    BOR_FREE(pool);
}

int planStatePoolDataReserve(plan_state_pool_t *pool,
                             size_t element_size,
                             plan_data_arr_el_init_fn init_fn,
                             const void *init_data)
{
    int data_id;

    data_id = pool->data_size;
    ++pool->data_size;
    pool->data = BOR_REALLOC_ARR(pool->data, plan_data_arr_t *,
                                 pool->data_size);
    pool->data[data_id] = planDataArrNew(element_size,
                                         init_fn, init_data);
    return data_id;
}

void *planStatePoolData(plan_state_pool_t *pool,
                        int data_id,
                        plan_state_id_t state_id)
{
    if (data_id >= pool->data_size)
        return NULL;

    return planDataArrGet(pool->data[data_id], state_id);
}


_bor_inline plan_state_id_t insertIntoHTable(plan_state_pool_t *pool,
                                             plan_state_id_t sid)
{
    plan_state_htable_t *htable;
    bor_list_t *hstate;
    const plan_state_htable_t *hfound;

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

plan_state_id_t planStatePoolInsert(plan_state_pool_t *pool,
                                    const plan_state_t *state)
{
    plan_state_id_t sid;
    void *statebuf;

    // determine state ID
    sid = pool->num_states;

    // allocate a new state and initialize it with the given values
    statebuf = planDataArrGet(pool->data[DATA_STATE], sid);
    planStatePackerPack(pool->packer, state, statebuf);

    return insertIntoHTable(pool, sid);
}

plan_state_id_t planStatePoolInsertPacked(plan_state_pool_t *pool,
                                          const void *packed_state)
{
    plan_state_id_t sid;
    void *statebuf;

    // determine state ID
    sid = pool->num_states;

    // allocate a new state and initialize it with the given values
    statebuf = planDataArrGet(pool->data[DATA_STATE], sid);
    memcpy(statebuf, packed_state, planStatePackerBufSize(pool->packer));

    return insertIntoHTable(pool, sid);
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

const void *planStatePoolGetPackedState(const plan_state_pool_t*pool,
                                        plan_state_id_t sid)
{
    if (sid >= pool->num_states)
        return NULL;
    return planDataArrGet(pool->data[DATA_STATE], sid);
}

int planStatePoolPartStateIsSubset(const plan_state_pool_t *pool,
                                   const plan_part_state_t *part_state,
                                   plan_state_id_t sid)
{
    void *statebuf;
    void *masked_state;
    int size, cmp;

    if (sid >= pool->num_states)
        return 0;

    // prepare temporary buffer
    size = planStatePackerBufSize(pool->packer);
    masked_state = BOR_ALLOC_ARR(char, size);

    // get corresponding state
    statebuf = planDataArrGet(pool->data[DATA_STATE], sid);

    // mask out values we are not interested in
    bitAnd(statebuf, part_state->maskbuf, size, masked_state);

    // compare resulting buffers
    cmp = memcmp(masked_state, part_state->valbuf, size);

    // free temporary buffer
    BOR_FREE(masked_state);

    return cmp == 0;
}

plan_state_id_t planStatePoolApplyPartState2(plan_state_pool_t *pool,
                                             const void *maskbuf,
                                             const void *valbuf,
                                             plan_state_id_t sid)
{
    void *statebuf, *newstate;
    plan_state_id_t newid;
    plan_state_htable_t *hstate;
    bor_list_t *hfound;

    if (sid >= pool->num_states)
        return PLAN_NO_STATE;

    // get corresponding state
    statebuf = planDataArrGet(pool->data[DATA_STATE], sid);

    // remember ID of the new state (if it will be inserted)
    newid = pool->num_states;

    // get buffer of the new state
    newstate = planDataArrGet(pool->data[DATA_STATE], newid);

    // apply partial state to the buffer of the new state
    bitApplyWithMask(statebuf, maskbuf, valbuf,
                     planStatePackerBufSize(pool->packer), newstate);

    // hash table struct correspodning to the new state and set it up
    hstate = planDataArrGet(pool->data[DATA_HTABLE], newid);
    hstate->state_id = newid;

    // insert it into hash table
    hfound = borHTableInsertUnique(pool->htable, &hstate->htable);

    if (hfound == NULL){
        // The state was inserted -- return the new id
        ++pool->num_states;
        return newid;

    }else{
        // Found in state pool, return its ID and forget the new state (by
        // simply not increasing pool->num_states).
        hstate = HTABLE_STATE(hfound);
        return hstate->state_id;
    }
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

static void bitApplyWithMask(const void *a, const void *m, const void *b,
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
