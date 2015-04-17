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

#include <boruvka/hfunc.h>
#include <plan/ma_private_state.h>

struct _state_t {
    int id;
    bor_list_t htable;
    int state[];
};
typedef struct _state_t state_t;

static void stateInit(void *el, int id, const void *ud)
{
    state_t *state = (state_t *)el;
    state->id = id;
    // No need to zeroize .state[] array because it is always fully
    // rewritten before inserting.
}

static bor_htable_key_t htableHash(const bor_list_t *key, void *ud)
{
    state_t *state = BOR_LIST_ENTRY(key, state_t, htable);
    plan_ma_private_state_t *aps = (plan_ma_private_state_t *)ud;
    return borCityHash_64(state->state, aps->state_size);
}

static int htableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud)
{
    state_t *s1 = BOR_LIST_ENTRY(k1, state_t, htable);
    state_t *s2 = BOR_LIST_ENTRY(k2, state_t, htable);
    plan_ma_private_state_t *aps = (plan_ma_private_state_t *)ud;

    return memcmp(s1->state, s2->state, aps->state_size) == 0;
}

void planMAPrivateStateInit(plan_ma_private_state_t *aps,
                            int num_agents, int agent_id)
{
    size_t size;

    aps->num_agents = num_agents;
    aps->agent_id = agent_id;

    aps->state_size = sizeof(int) * (num_agents - 1);
    size = sizeof(state_t) + aps->state_size;
    aps->states = borExtArrNew2(size, 32, 128, stateInit, NULL);
    aps->num_states = 0;

    aps->htable = borHTableNew(htableHash, htableEq, aps);
}

void planMAPrivateStateFree(plan_ma_private_state_t *aps)
{
    if (aps->htable)
        borHTableDel(aps->htable);
    if (aps->states)
        borExtArrDel(aps->states);
}

int planMAPrivateStateInsert(plan_ma_private_state_t *aps, int *state_ids)
{
    state_t *state;
    bor_list_t *hfound;
    int *dst;
    int i, j;

    state = borExtArrGet(aps->states, aps->num_states);
    dst = state->state;
    for (i = 0, j = 0; i < aps->num_agents; ++i){
        if (i != aps->agent_id)
            dst[j++] = state_ids[i];
    }

    hfound = borHTableInsertUnique(aps->htable, &state->htable);
    if (hfound == NULL){
        aps->num_states++;
        return state->id;
    }else{
        state = BOR_LIST_ENTRY(hfound, state_t, htable);
        return state->id;
    }
}

void planMAPrivateStateGet(const plan_ma_private_state_t *aps, int id,
                           int *state_ids)
{
    const state_t *state;
    const int *src;
    int i, j;

    state = borExtArrGet(aps->states, id);
    src = state->state;
    for (i = 0, j = 0; i < aps->num_agents; ++i){
        if (i == aps->agent_id){
            state_ids[i] = -1;
        }else{
            state_ids[i] = src[j++];
        }
    }
}
