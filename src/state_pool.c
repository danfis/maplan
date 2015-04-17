/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#include <strings.h>
#include <boruvka/hfunc.h>
#include <boruvka/alloc.h>
#include "plan/state_pool.h"

struct _plan_state_packed_t {
    bor_list_t htable;        /*!< Connection into hash table */
    plan_state_id_t state_id; /*!< Corresponding state ID */
    char statebuf[];          /*!< Variable data containing packed state */
};
typedef struct _plan_state_packed_t plan_state_packed_t;

#define STATE_PACKED_STACK(name, pool) \
    plan_state_packed_t *name = (plan_state_packed_t *)alloca( \
            sizeof(plan_state_packed_t) + \
            planStatePackerBufSize(pool->packer))

#define STATE_FROM_HTABLE(list) \
    BOR_LIST_ENTRY(list, plan_state_packed_t, htable)

/** Returns state buffer from the struct */
_bor_inline void *stateBuf(const plan_state_packed_t *s);
/** Returns state structure corresponding to the state ID */
_bor_inline plan_state_packed_t *statePacked(const plan_state_pool_t *pool,
                                             plan_state_id_t sid);

/** Inserts state into hash table and returns ID under which it is stored. */
_bor_inline plan_state_id_t insertIntoHTable(plan_state_pool_t *pool,
                                             plan_state_packed_t *sp);

/** Callbacks for bor_htable_t */
static bor_htable_key_t htableHash(const bor_list_t *key, void *ud);
static int htableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud);

/** Initialization function for data array holding plan_state_packed_t */
static void statePackedInit(void *el, int id, const void *ud);

plan_state_pool_t *planStatePoolNew(const plan_var_t *var, int var_size)
{
    int state_size, size;
    plan_state_pool_t *pool;

    pool = BOR_ALLOC(plan_state_pool_t);
    pool->num_vars = var_size;

    pool->packer = planStatePackerNew(var, var_size);
    state_size = planStatePackerBufSize(pool->packer);

    pool->data = BOR_ALLOC_ARR(bor_extarr_t *, 2);

    size  = sizeof(plan_state_packed_t);
    size += state_size;
    pool->data[0] = borExtArrNew2(size, 128, 256, statePackedInit, pool);

    pool->data_size = 1;
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
        borExtArrDel(pool->data[i]);
    }
    BOR_FREE(pool->data);

    if (pool->packer)
        planStatePackerDel(pool->packer);

    BOR_FREE(pool);
}

plan_state_pool_t *planStatePoolClone(const plan_state_pool_t *sp)
{
    plan_state_pool_t *pool;
    plan_state_packed_t *state;
    int i;

    pool = BOR_ALLOC(plan_state_pool_t);
    memcpy(pool, sp, sizeof(*sp));
    pool->packer = planStatePackerClone(sp->packer);
    pool->data = BOR_ALLOC_ARR(bor_extarr_t *, sp->data_size);
    for (i = 0; i < sp->data_size; ++i)
        pool->data[i] = borExtArrClone(sp->data[i]);

    pool->htable = borHTableNew(htableHash, htableEq, (void *)pool);
    pool->num_states = 0;
    for (i = 0; i < sp->num_states; ++i){
        state = statePacked(pool, i);
        insertIntoHTable(pool, state);
    }

    return pool;
}

int planStatePoolDataReserve(plan_state_pool_t *pool,
                             size_t element_size,
                             bor_extarr_el_init_fn init_fn,
                             const void *init_data)
{
    int data_id;

    data_id = pool->data_size;
    ++pool->data_size;
    pool->data = BOR_REALLOC_ARR(pool->data, bor_extarr_t *,
                                 pool->data_size);
    pool->data[data_id] = borExtArrNew2(element_size, 128, 256,
                                        init_fn, init_data);
    return data_id;
}

void *planStatePoolData(plan_state_pool_t *pool,
                        int data_id,
                        plan_state_id_t state_id)
{
    if (data_id >= pool->data_size)
        return NULL;

    return borExtArrGet(pool->data[data_id], state_id);
}

plan_state_id_t planStatePoolInsert(plan_state_pool_t *pool,
                                    const plan_state_t *state)
{
    plan_state_id_t sid;
    plan_state_packed_t *sp;

    // determine state ID
    sid = pool->num_states;

    // allocate a new state and initialize it with the given values
    sp = statePacked(pool, sid);
    planStatePackerPack(pool->packer, state, stateBuf(sp));

    return insertIntoHTable(pool, sp);
}

plan_state_id_t planStatePoolInsertPacked(plan_state_pool_t *pool,
                                          const void *packed_state)
{
    plan_state_id_t sid;
    plan_state_packed_t *sp;

    // determine state ID
    sid = pool->num_states;

    // allocate a new state and initialize it with the given values
    sp = statePacked(pool, sid);
    memcpy(stateBuf(sp), packed_state, planStatePackerBufSize(pool->packer));

    return insertIntoHTable(pool, sp);
}

plan_state_id_t planStatePoolFind(const plan_state_pool_t *pool,
                                  const plan_state_t *state)
{
    STATE_PACKED_STACK(sp, pool);
    bor_list_t *hstate;

    memset(stateBuf(sp), 0, planStatePackerBufSize(pool->packer));
    planStatePackerPack(pool->packer, state, stateBuf(sp));
    hstate = borHTableFind(pool->htable, &sp->htable);

    if (hstate == NULL){
        return PLAN_NO_STATE;
    }

    sp = STATE_FROM_HTABLE(hstate);
    return sp->state_id;
}

void planStatePoolGetState(const plan_state_pool_t *pool,
                           plan_state_id_t sid,
                           plan_state_t *state)
{
    plan_state_packed_t *sp;

    if (sid >= pool->num_states)
        return;

    sp = statePacked(pool, sid);
    planStatePackerUnpack(pool->packer, stateBuf(sp), state);
    state->state_id = sid;
}

const void *planStatePoolGetPackedState(const plan_state_pool_t *pool,
                                        plan_state_id_t sid)
{
    if (sid >= pool->num_states)
        return NULL;
    return stateBuf(statePacked(pool, sid));
}


_bor_inline int isSubsetPacked(const plan_state_pool_t *pool,
                               const plan_part_state_t *part_state,
                               plan_state_id_t sid)
{
    plan_state_packed_t *sp;

    // get corresponding state
    sp = statePacked(pool, sid);
    return planPartStateIsSubsetPackedState(part_state, stateBuf(sp));
}

_bor_inline int isSubset(const plan_state_pool_t *pool,
                         const plan_part_state_t *part_state,
                         plan_state_id_t sid)
{
    PLAN_STATE_STACK(state, pool->num_vars);

    planStatePoolGetState(pool, sid, &state);
    return planPartStateIsSubsetState(part_state, &state);
}

int planStatePoolPartStateIsSubset(const plan_state_pool_t *pool,
                                   const plan_part_state_t *part_state,
                                   plan_state_id_t sid)
{
    if (sid >= pool->num_states)
        return 0;

    if (part_state->bufsize > 0)
        return isSubsetPacked(pool, part_state, sid);
    return isSubset(pool, part_state, sid);
}

_bor_inline plan_state_id_t applyPartStatePacked(plan_state_pool_t *pool,
                                                 const plan_part_state_t *ps,
                                                 plan_state_id_t sid)
{
    plan_state_packed_t *sp, *newsp;
    plan_state_id_t newid;

    // get corresponding state
    sp = statePacked(pool, sid);

    // remember ID of the new state (if it will be inserted)
    newid = pool->num_states;

    // get buffer of the new state
    newsp = statePacked(pool, newid);

    // apply partial state to the buffer of the new state
    planPartStateCreatePackedState(ps, stateBuf(sp), stateBuf(newsp));

    return insertIntoHTable(pool, newsp);
}

_bor_inline plan_state_id_t applyPartState(plan_state_pool_t *pool,
                                           const plan_part_state_t *ps,
                                           plan_state_id_t sid)
{
    PLAN_STATE_STACK(state, pool->num_vars);

    planStatePoolGetState(pool, sid, &state);
    planPartStateUpdateState(ps, &state);
    return planStatePoolInsert(pool, &state);
}

plan_state_id_t planStatePoolApplyPartState(plan_state_pool_t *pool,
                                            const plan_part_state_t *ps,
                                            plan_state_id_t sid)
{
    if (sid >= pool->num_states)
        return PLAN_NO_STATE;

    if (ps->bufsize > 0){
        return applyPartStatePacked(pool, ps, sid);
    }else{
        return applyPartState(pool, ps, sid);
    }
}


_bor_inline plan_state_id_t applyPartStatesPacked(plan_state_pool_t *pool,
                                                  const plan_part_state_t **ps,
                                                  int ps_len,
                                                  plan_state_id_t sid)
{
    plan_state_packed_t *sp, *newsp;
    plan_state_id_t newid;
    int i;

    // get corresponding state
    sp = statePacked(pool, sid);

    // remember ID of the new state (if it will be inserted)
    newid = pool->num_states;

    // get buffer of the new state
    newsp = statePacked(pool, newid);

    // apply partial state to the buffer of the new state
    planPartStateCreatePackedState(ps[0], stateBuf(sp), stateBuf(newsp));
    for (i = 1; i < ps_len; ++i){
        planPartStateUpdatePackedState(ps[i], stateBuf(newsp));
    }

    return insertIntoHTable(pool, newsp);
}

_bor_inline plan_state_id_t applyPartStates(plan_state_pool_t *pool,
                                            const plan_part_state_t **ps,
                                            int ps_len,
                                            plan_state_id_t sid)
{
    PLAN_STATE_STACK(state, pool->num_vars);
    int i;

    planStatePoolGetState(pool, sid, &state);
    for (i = 0; i < ps_len; ++i)
        planPartStateUpdateState(ps[i], &state);
    return planStatePoolInsert(pool, &state);
}

plan_state_id_t planStatePoolApplyPartStates(plan_state_pool_t *pool,
                                             const plan_part_state_t **part_states,
                                             int part_states_len,
                                             plan_state_id_t sid)
{
    if (sid >= pool->num_states || part_states_len <= 0)
        return PLAN_NO_STATE;

    if (part_states[0]->bufsize > 0)
        return applyPartStatesPacked(pool, part_states, part_states_len, sid);
    return applyPartStates(pool, part_states, part_states_len, sid);
}



_bor_inline void *stateBuf(const plan_state_packed_t *s)
{
    return (void *)s->statebuf;
}

_bor_inline plan_state_packed_t *statePacked(const plan_state_pool_t *pool,
                                             plan_state_id_t sid)
{
    return (plan_state_packed_t *)borExtArrGet(pool->data[0], sid);
}

_bor_inline plan_state_id_t insertIntoHTable(plan_state_pool_t *pool,
                                             plan_state_packed_t *sp)
{
    bor_list_t *hstate;

    // insert it into hash table
    hstate = borHTableInsertUnique(pool->htable, &sp->htable);

    if (hstate == NULL){
        // NULL is returned if the element was inserted into table, so
        // increase number of elements in the pool
        ++pool->num_states;
        return sp->state_id;

    }else{
        // If the non-NULL was returned, it means that the same state was
        // already in hash table.
        sp = STATE_FROM_HTABLE(hstate);
        return sp->state_id;
    }
}

static bor_htable_key_t htableHash(const bor_list_t *key, void *ud)
{
    const plan_state_packed_t *sp = STATE_FROM_HTABLE(key);
    plan_state_pool_t *pool = (plan_state_pool_t *)ud;
    return borCityHash_64(stateBuf(sp), planStatePackerBufSize(pool->packer));
}

static int htableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud)
{
    const plan_state_packed_t *s1 = STATE_FROM_HTABLE(k1);
    const plan_state_packed_t *s2 = STATE_FROM_HTABLE(k2);
    plan_state_pool_t *pool = (plan_state_pool_t *)ud;
    size_t size = planStatePackerBufSize(pool->packer);
    return memcmp(stateBuf(s1), stateBuf(s2), size) == 0;
}

static void statePackedInit(void *el, int id, const void *ud)
{
    plan_state_packed_t *sp = (plan_state_packed_t *)el;
    plan_state_pool_t *pool = (plan_state_pool_t *)ud;
    size_t size = planStatePackerBufSize(pool->packer);

    sp->state_id = id;
    memset(stateBuf(sp), 0, size);
}
