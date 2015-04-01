#include <boruvka/alloc.h>
#include <plan/ma_state.h>

plan_ma_state_t *planMAStateNew(plan_state_pool_t *state_pool,
                                int num_agents, int agent_id)
{
    plan_ma_state_t *ma_state;

    ma_state = BOR_ALLOC(plan_ma_state_t);
    ma_state->state_pool = state_pool;
    ma_state->packer = state_pool->packer;
    ma_state->bufsize = planStatePackerBufSize(state_pool->packer);
    ma_state->num_agents = num_agents;
    ma_state->agent_id = agent_id;
    ma_state->ma_privacy = 0;

    if (state_pool->packer->ma_privacy){
        ma_state->ma_privacy = 1;
        planMAPrivateStateInit(&ma_state->private_state, num_agents, agent_id);
        // TODO: Add initial value
    }

    return ma_state;
}

void planMAStateDel(plan_ma_state_t *ma_state)
{
    if (ma_state->ma_privacy)
        planMAPrivateStateFree(&ma_state->private_state);
    BOR_FREE(ma_state);
}

int planMAStateSetMAMsg(const plan_ma_state_t *ma_state,
                        plan_state_id_t state_id,
                        plan_ma_msg_t *ma_msg)
{
    const void *buf;

    buf = planStatePoolGetPackedState(ma_state->state_pool, state_id);
    if (buf == NULL)
        return -1;

    // TODO: ma-privacy mode
    planMAMsgSetStateBuf(ma_msg, buf, ma_state->bufsize);
    planMAMsgSetStateId(ma_msg, state_id);
    return 0;
}

int planMAStateSetMAMsg2(const plan_ma_state_t *ma_state,
                         const plan_state_t *state,
                         plan_ma_msg_t *ma_msg)
{
    void *buf;

    // TODO: ma-privacy mode
    buf = BOR_ALLOC_ARR(char, ma_state->bufsize);
    planStatePackerPack(ma_state->packer, state, buf);
    planMAMsgSetStateBuf(ma_msg, buf, ma_state->bufsize);
    BOR_FREE(buf);
    return 0;
}

plan_state_id_t planMAStateInsertFromMAMsg(plan_ma_state_t *ma_state,
                                           const plan_ma_msg_t *ma_msg)
{
    plan_state_id_t state_id;
    const void *buf;

    // TODO: ma-privacy mode
    buf = planMAMsgStateBuf(ma_msg);
    state_id = planStatePoolInsertPacked(ma_state->state_pool, buf);
    return state_id;
}

void planMAStateGetFromMAMsg(const plan_ma_state_t *ma_state,
                             const plan_ma_msg_t *ma_msg,
                             plan_state_t *state)
{
    const void *buf;

    // TODO: ma-privacy mode
    buf = planMAMsgStateBuf(ma_msg);
    planStatePackerUnpack(ma_state->packer, buf, state);
}

int planMAStateMAMsgIsSet(const plan_ma_state_t *ma_state,
                          const plan_ma_msg_t *ma_msg)
{
    // TODO: ma-privacy mode
    return planMAMsgStateBufSize(ma_msg) == ma_state->bufsize;
}
