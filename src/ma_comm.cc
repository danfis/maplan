#include <boruvka/alloc.h>

#include "plan/ma_comm.h"
#include "ma_msg.pb.h"

plan_ma_msg_t *planMAMsgNew(void)
{
    PlanMAMsg *msg;
    msg = new PlanMAMsg;
    return msg;
}

void planMAMsgDel(plan_ma_msg_t *_msg)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    delete msg;
}

void *planMAMsgPacked(const plan_ma_msg_t *_msg, size_t *size)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    void *buf;

    *size = msg->ByteSize();
    buf = BOR_ALLOC_ARR(char, *size);
    msg->SerializeToArray(buf, *size);
    return buf;
}

plan_ma_msg_t *planMAMsgUnpacked(void *buf, size_t size)
{
    PlanMAMsg *msg = new PlanMAMsg;
    msg->ParseFromArray(buf, size);
    return msg;
}

void planMAMsgSetPublicState(plan_ma_msg_t *_msg,
                             const char *agent_name,
                             void *state, size_t state_size,
                             int cost,
                             int heuristic)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    msg->set_agent_name(agent_name);
    msg->set_state(state, state_size);
    msg->set_cost(cost);
    msg->set_heuristic(heuristic);
}

int planMAMsgIsPublicState(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::PUBLIC_STATE;
}

void planMAMsgGetPublicState(const plan_ma_msg_t *_msg,
                             char *agent_name, size_t agent_name_size,
                             void *state, size_t state_size,
                             int *cost,
                             int *heuristic)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    size_t st_size;

    strncpy(agent_name, msg->agent_name().c_str(), agent_name_size);

    st_size = BOR_MIN(state_size, msg->state().size());
    memcpy(state, msg->state().data(), st_size);

    *cost      = msg->cost();
    *heuristic = msg->heuristic();
}

