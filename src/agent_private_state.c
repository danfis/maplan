#include <stdio.h>
#include <boruvka/hfunc.h>
#include <plan/agent_private_state.h>

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
}

static bor_htable_key_t htableHash(const bor_list_t *key, void *ud)
{
    state_t *state = BOR_LIST_ENTRY(key, state_t, htable);
    plan_agent_private_state_t *aps = (plan_agent_private_state_t *)ud;
    return borCityHash_64(state->state, aps->state_size);
}

static int htableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud)
{
    state_t *s1 = BOR_LIST_ENTRY(k1, state_t, htable);
    state_t *s2 = BOR_LIST_ENTRY(k2, state_t, htable);
    plan_agent_private_state_t *aps = (plan_agent_private_state_t *)ud;

    return memcmp(s1->state, s2->state, aps->state_size) == 0;
}

void planAgentPrivateStateInit(plan_agent_private_state_t *aps,
                               int num_agents, int agent_id)
{
    size_t size;

    aps->num_agents = num_agents;
    aps->agent_id = agent_id;

    aps->state_size = sizeof(int) * (num_agents - 1);
    size = sizeof(state_t) + aps->state_size;
    aps->states = planDataArrNew(size, stateInit, NULL);
    aps->num_states = 0;

    aps->htable = borHTableNew(htableHash, htableEq, aps);
}

void planAgentPrivateStateFree(plan_agent_private_state_t *aps)
{
    if (aps->htable)
        borHTableDel(aps->htable);
    if (aps->states)
        planDataArrDel(aps->states);
}

int planAgentPrivateStateInsert(plan_agent_private_state_t *aps,
                                int *state_ids)
{
    state_t *state;
    bor_list_t *hfound;
    int *dst;
    int i, j;

    state = planDataArrGet(aps->states, aps->num_states);
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

void planAgentPrivateStateGet(const plan_agent_private_state_t *aps, int id,
                              int *state_ids)
{
    const state_t *state;
    const int *src;
    int i, j;

    state = planDataArrGet(aps->states, id);
    src = state->state;
    for (i = 0, j = 0; i < aps->num_agents; ++i){
        if (i == aps->agent_id){
            state_ids[i] = -1;
        }else{
            state_ids[i] = src[j++];
        }
    }
}
