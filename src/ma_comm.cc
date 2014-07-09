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

int planMAMsgType(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type();
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

void planMAMsgSetPublicState(plan_ma_msg_t *_msg, int agent_id,
                             const void *state, size_t state_size,
                             int cost, int heuristic)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgPublicState *public_state;

    msg->set_type(PlanMAMsg::PUBLIC_STATE);
    public_state = msg->mutable_public_state();
    public_state->set_agent_id(agent_id);
    public_state->set_state(state, state_size);
    public_state->set_cost(cost);
    public_state->set_heuristic(heuristic);
}

int planMAMsgIsPublicState(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::PUBLIC_STATE;
}

void planMAMsgGetPublicState(const plan_ma_msg_t *_msg, int *agent_id,
                             void *state, size_t state_size,
                             int *cost, int *heuristic)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    size_t st_size;

    const PlanMAMsgPublicState &public_state = msg->public_state();
    *agent_id = public_state.agent_id();

    st_size = BOR_MIN(state_size, public_state.state().size());
    memcpy(state, public_state.state().data(), st_size);

    *cost      = public_state.cost();
    *heuristic = public_state.heuristic();
}

void planMAMsgSetTerminate(plan_ma_msg_t *_msg)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    msg->set_type(PlanMAMsg::TERMINATE);
}

int planMAMsgIsTerminate(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::TERMINATE;
}

void planMAMsgSetTerminateRequest(plan_ma_msg_t *_msg)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    msg->set_type(PlanMAMsg::TERMINATE_REQUEST);
}

int planMAMsgIsTerminateRequest(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::TERMINATE_REQUEST;
}

void planMAMsgSetTerminateAck(plan_ma_msg_t *_msg)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    msg->set_type(PlanMAMsg::TERMINATE_ACK);
}

int planMAMsgIsTerminateAck(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::TERMINATE_ACK;
}

int planMAMsgIsTerminateType(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    unsigned type = msg->type();

    if ((type & 0x100u) == 0x100u)
        return 1;
    return 0;
}

void planMAMsgSetTracePath(plan_ma_msg_t *_msg, int origin_agent_id)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgTracePath *trace_path;

    msg->set_type(PlanMAMsg::TRACE_PATH);
    trace_path = msg->mutable_trace_path();
    trace_path->set_origin_agent_id(origin_agent_id);
    trace_path->set_done(false);
}

void planMAMsgTracePathSetState(plan_ma_msg_t *_msg,
                                void *state, size_t state_size)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgTracePath *trace_path;

    trace_path = msg->mutable_trace_path();
    trace_path->set_state(state, state_size);
}

void planMAMsgTracePathAddOperator(plan_ma_msg_t *_msg,
                                   const char *name,
                                   int cost)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgTracePath *trace_path;
    PlanMAMsgPathOperator *op;

    trace_path = msg->mutable_trace_path();
    op = trace_path->add_path();
    op->set_name(name);
    op->set_cost(cost);
}

void planMAMsgTracePathSetDone(plan_ma_msg_t *_msg)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgTracePath *trace_path;

    trace_path = msg->mutable_trace_path();
    trace_path->set_done(true);
}

int planMAMsgIsTracePath(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::TRACE_PATH;
}

int planMAMsgTracePathIsDone(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgTracePath &trace_path = msg->trace_path();
    return trace_path.done();
}

int planMAMsgTracePathOriginAgent(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgTracePath &trace_path = msg->trace_path();
    return trace_path.origin_agent_id();
}

const void *planMAMsgTracePathState(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgTracePath &trace_path = msg->trace_path();
    return trace_path.state().data();
}
