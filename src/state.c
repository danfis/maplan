#include <stdarg.h>
#include <strings.h>
#include <boruvka/hfunc.h>
#include <boruvka/alloc.h>
#include "plan/state.h"


#define DATA_STATE 0
#define DATA_HTABLE 1

#define HTABLE_STATE(list) \
    BOR_LIST_ENTRY(list, plan_state_htable_t, htable)

struct _plan_state_htable_t {
    bor_list_t htable;
    plan_state_id_t state_id;
};
typedef struct _plan_state_htable_t plan_state_htable_t;


struct _plan_state_packer_var_t {
    int bitlen;                    /*!< Number of bits required to store a value */
    plan_packer_word_t shift;      /*!< Left shift size during packing */
    plan_packer_word_t mask;       /*!< Mask for packed values */
    plan_packer_word_t clear_mask; /*!< Clear mask for packed values (i.e., ~mask) */
    int pos;                       /*!< Position of a word in buffer where
                                        values are stored. */
};
typedef struct _plan_state_packer_var_t plan_state_packer_var_t;

/** Sets variable to value in packed state buffer */
static void planStatePackerSet(plan_state_packer_t *packer,
                               plan_var_id_t var,
                               plan_val_t val,
                               void *buf);
/** Sets mask (~0) corresponding to the variable in packed state buffer */
static void planStatePackerSetMask(plan_state_packer_t *packer,
                                   plan_var_id_t var, void *buf);
/** Returns number of bits needed for storing all values in interval
 * [0,range). */
static int packerBitsNeeded(plan_val_t range);
/** Comparator for sorting variables by their bit length */
static int sortCmpVar(const void *a, const void *b);
/** Constructs mask for variable */
static plan_packer_word_t packerVarMask(int bitlen, int shift);
/** Set variable into packed buffer */
static void packerSetVar(const plan_state_packer_var_t *var,
                         plan_packer_word_t val,
                         void *buffer);
/** Retrieves a value of the variable from packed buffer */
static plan_val_t packerGetVar(const plan_state_packer_var_t *var,
                               const void *buffer);

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

static void printBin(const unsigned char *m, int size)
{
    int i, j;
    unsigned char c;

    for (i = 0; i < size; ++i){
        c = m[i];

        for (j = 0; j < 8; ++j){
            if (c & 0x80){
                fprintf(stderr, "1");
            }else{
                fprintf(stderr, "0");
            }
            c = c << 1;
        }
    }
}

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


plan_state_id_t planStatePoolInsert(plan_state_pool_t *pool,
                                    const plan_state_t *state)
{
    // TODO: Refactor with planStatePoolInsertPacked() and
    // planStatePoolFind()
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

plan_state_id_t planStatePoolInsertPacked(plan_state_pool_t *pool,
                                          const void *packed_state)
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
    memcpy(statebuf, packed_state, planStatePackerBufSize(pool->packer));

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

plan_state_id_t planStatePoolApplyPartState(plan_state_pool_t *pool,
                                            const plan_part_state_t *part_state,
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
    bitApplyWithMask(statebuf, part_state->maskbuf, part_state->valbuf,
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



/** State Packer **/
plan_state_packer_t *planStatePackerNew(const plan_var_t *var,
                                        int var_size)
{
    int i, is_set, bitlen, wordpos;
    plan_state_packer_t *p;
    plan_state_packer_var_t **pvars;


    p = BOR_ALLOC(plan_state_packer_t);
    p->num_vars = var_size;
    p->vars = BOR_ALLOC_ARR(plan_state_packer_var_t, p->num_vars);

    pvars = BOR_ALLOC_ARR(plan_state_packer_var_t *, p->num_vars);

    for (i = 0; i < var_size; ++i){
        p->vars[i].bitlen = packerBitsNeeded(var[i].range);
        p->vars[i].pos = -1;
        pvars[i] = p->vars + i;
    }
    qsort(pvars, var_size, sizeof(plan_state_packer_var_t *), sortCmpVar);

    is_set = 0;
    wordpos = -1;
    while (is_set < var_size){
        ++wordpos;

        bitlen = 0;
        for (i = 0; i < var_size; ++i){
            if (pvars[i]->pos == -1
                    && pvars[i]->bitlen + bitlen <= PLAN_PACKER_WORD_BITS){
                pvars[i]->pos = wordpos;
                bitlen += pvars[i]->bitlen;
                pvars[i]->shift = PLAN_PACKER_WORD_BITS - bitlen;
                pvars[i]->mask = packerVarMask(pvars[i]->bitlen,
                                               pvars[i]->shift);
                pvars[i]->clear_mask = ~pvars[i]->mask;
                ++is_set;
            }
        }
    }

    p->bufsize = sizeof(plan_packer_word_t) * (wordpos + 1);

    // free temporary array
    BOR_FREE(pvars);

    /*
    for (i = 0; i < var_size; ++i){
        fprintf(stderr, "%d %d %d %d\n", (int)PLAN_PACKER_WORD_BITS,
                p->vars[i].bitlen, p->vars[i].shift,
                p->vars[i].pos);
    }
    fprintf(stderr, "%lu\n", p->bufsize);
    */
    return p;
}

void planStatePackerDel(plan_state_packer_t *p)
{
    if (p->vars)
        BOR_FREE(p->vars);
    BOR_FREE(p);
}

void planStatePackerPack(const plan_state_packer_t *p,
                         const plan_state_t *state,
                         void *buffer)
{
    int i;
    for (i = 0; i < p->num_vars; ++i){
        packerSetVar(p->vars + i, planStateGet(state, i), buffer);
    }
}

void planStatePackerUnpack(const plan_state_packer_t *p,
                           const void *buffer,
                           plan_state_t *state)
{
    int i;
    for (i = 0; i < p->num_vars; ++i){
        planStateSet(state, i, packerGetVar(p->vars + i, buffer));
    }
}

static void planStatePackerSet(plan_state_packer_t *packer,
                               plan_var_id_t var,
                               plan_val_t val,
                               void *buf)
{
    packerSetVar(packer->vars + var, val, buf);
}

static void planStatePackerSetMask(plan_state_packer_t *packer,
                                   plan_var_id_t var, void *_buf)
{
    plan_packer_word_t *buf = (plan_packer_word_t *)_buf;
    buf[packer->vars[var].pos] |= packer->vars[var].mask;
}

static int packerBitsNeeded(plan_val_t range)
{
    plan_packer_word_t max_val = range - 1;
    int num = PLAN_PACKER_WORD_BITS;

    for (; !(max_val & PLAN_PACKER_WORD_SET_HI_BIT); --num, max_val <<= 1);
    return num;
}

static int sortCmpVar(const void *a, const void *b)
{
    const plan_state_packer_var_t *va = *(const plan_state_packer_var_t **)a;
    const plan_state_packer_var_t *vb = *(const plan_state_packer_var_t **)b;
    if (va->bitlen == vb->bitlen){
        return 0;
    }else if (va->bitlen < vb->bitlen){
        return 1;
    }else{
        return -1;
    }
}

static plan_packer_word_t packerVarMask(int bitlen, int shift)
{
    plan_packer_word_t mask1, mask2;

    mask1 = PLAN_PACKER_WORD_SET_ALL_BITS << shift;
    mask2 = PLAN_PACKER_WORD_SET_ALL_BITS
                >> (PLAN_PACKER_WORD_BITS - shift - bitlen);
    return mask1 & mask2;
}

static void packerSetVar(const plan_state_packer_var_t *var,
                         plan_packer_word_t val,
                         void *buffer)
{
    plan_packer_word_t *buf;
    val = (val << var->shift) & var->mask;
    buf = ((plan_packer_word_t *)buffer) + var->pos;
    *buf = (*buf & var->clear_mask) | val;
}

static plan_val_t packerGetVar(const plan_state_packer_var_t *var,
                               const void *buffer)
{
    plan_packer_word_t val;
    plan_packer_word_t *buf;
    buf = ((plan_packer_word_t *)buffer) + var->pos;
    val = *buf;
    val = (val & var->mask) >> var->shift;
    return val;
}







/** State **/
plan_state_t *planStateNew(const plan_state_pool_t *pool)
{
    plan_state_t *state;
    state = BOR_ALLOC(plan_state_t);
    state->val = BOR_ALLOC_ARR(plan_val_t, pool->num_vars);
    state->num_vars = pool->num_vars;
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
    for (i = 0; i < state->num_vars; ++i)
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





/** Partial State **/

plan_part_state_t *planPartStateNew(const plan_state_pool_t *pool)
{
    plan_part_state_t *ps;
    int i, size;

    ps = BOR_ALLOC(plan_part_state_t);
    ps->num_vars = pool->num_vars;
    ps->val    = BOR_ALLOC_ARR(plan_val_t, pool->num_vars);
    ps->is_set = BOR_ALLOC_ARR(int, pool->num_vars);

    for (i = 0; i < pool->num_vars; ++i){
        ps->val[i] = PLAN_VAL_UNDEFINED;
        ps->is_set[i] = 0;
    }

    size = planStatePackerBufSize(pool->packer);
    ps->valbuf  = BOR_ALLOC_ARR(char, size);
    ps->maskbuf = BOR_ALLOC_ARR(char, size);

    bzero(ps->valbuf, size);
    bzero(ps->maskbuf, size);

    ps->vals = NULL;
    ps->vals_size = 0;

    return ps;

}

void planPartStateDel(plan_part_state_t *part_state)
{
    BOR_FREE(part_state->val);
    BOR_FREE(part_state->is_set);
    BOR_FREE(part_state->valbuf);
    BOR_FREE(part_state->maskbuf);
    BOR_FREE(part_state->vals);
    BOR_FREE(part_state);
}

static int valsCmp(const void *_a, const void *_b)
{
    const plan_part_state_pair_t *a = _a;
    const plan_part_state_pair_t *b = _b;

    if (a->var == b->var)
        return 0;
    if (a->var < b->var)
        return -1;
    return 1;
}

void planPartStateSet(plan_state_pool_t *pool,
                      plan_part_state_t *state,
                      plan_var_id_t var,
                      plan_val_t val)
{
    state->val[var] = val;
    state->is_set[var] = 1;

    planStatePackerSet(pool->packer, var, val, state->valbuf);
    planStatePackerSetMask(pool->packer, var, state->maskbuf);

    ++state->vals_size;
    state->vals = BOR_REALLOC_ARR(state->vals,
                                  plan_part_state_pair_t,
                                  state->vals_size);
    state->vals[state->vals_size - 1].var = var;
    state->vals[state->vals_size - 1].val = val;
    qsort(state->vals, state->vals_size, sizeof(plan_part_state_pair_t), valsCmp);
}

void planPartStateToState(const plan_part_state_t *part_state,
                          plan_state_t *state)
{
    memcpy(state->val, part_state->val,
           sizeof(plan_val_t) * part_state->num_vars);
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
