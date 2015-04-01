#include <boruvka/alloc.h>
#include <plan/ma_state.h>

plan_ma_state_t *planMAStateNew(plan_state_pool_t *state_pool,
                                int num_agents, int agent_id)
{
    plan_ma_state_t *ma_state;

    ma_state = BOR_ALLOC(plan_ma_state_t);
    ma_state->state_pool = state_pool;
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

int planMAStateSetMAMsg(plan_ma_state_t *ma_state,
                        plan_state_id_t state_id,
                        plan_ma_msg_t *ma_msg)
{
    const void *buf;
    int buf_size;

    buf = planStatePoolGetPackedState(ma_state->state_pool, state_id);
    buf_size = planStatePackerBufSize(ma_state->state_pool->packer);
    if (buf == NULL)
        return -1;

    // TODO: ma-privacy mode
    planMAMsgSetStateBuf(ma_msg, buf, buf_size);
    planMAMsgSetStateId(ma_msg, state_id);
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
