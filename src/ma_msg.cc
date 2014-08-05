#include <boruvka/alloc.h>

#include "plan/ma_msg.h"
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

int planMAMsgIsSearchType(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    unsigned type = msg->type();

    if ((type & 0x200u) == 0x200u)
        return 1;
    return 0;
}

void planMAMsgSetPublicState(plan_ma_msg_t *_msg, int agent_id,
                             const void *state, size_t state_size,
                             int state_id,
                             int cost, int heuristic)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgPublicState *public_state;

    msg->set_type(PlanMAMsg::SEARCH_PUBLIC_STATE);
    public_state = msg->mutable_public_state();
    public_state->set_agent_id(agent_id);
    public_state->set_state(state, state_size);
    public_state->set_state_id(state_id);
    public_state->set_cost(cost);
    public_state->set_heuristic(heuristic);
}

int planMAMsgIsPublicState(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::SEARCH_PUBLIC_STATE;
}

int planMAMsgPublicStateAgent(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgPublicState &public_state = msg->public_state();
    return public_state.agent_id();
}

const void *planMAMsgPublicStateStateBuf(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgPublicState &public_state = msg->public_state();
    return public_state.state().data();
}

int planMAMsgPublicStateStateId(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgPublicState &public_state = msg->public_state();
    return public_state.state_id();
}

int planMAMsgPublicStateCost(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgPublicState &public_state = msg->public_state();
    return public_state.cost();
}

int planMAMsgPublicStateHeur(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgPublicState &public_state = msg->public_state();
    return public_state.heuristic();
}

void planMAMsgGetPublicState(const plan_ma_msg_t *_msg, int *agent_id,
                             void *state, size_t state_size,
                             int *state_id,
                             int *cost, int *heuristic)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    size_t st_size;

    const PlanMAMsgPublicState &public_state = msg->public_state();
    *agent_id = public_state.agent_id();

    st_size = BOR_MIN(state_size, public_state.state().size());
    memcpy(state, public_state.state().data(), st_size);
    *state_id = public_state.state_id();

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

void planMAMsgTracePathSetStateId(plan_ma_msg_t *_msg,
                                  int state_id)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgTracePath *trace_path;

    trace_path = msg->mutable_trace_path();
    trace_path->set_state_id(state_id);
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

int planMAMsgTracePathStateId(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgTracePath &trace_path = msg->trace_path();
    return trace_path.state_id();
}

int planMAMsgTracePathNumOperators(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgTracePath &trace_path = msg->trace_path();
    return trace_path.path_size();
}

const char *planMAMsgTracePathOperator(const plan_ma_msg_t *_msg, int i,
                                       int *cost)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgTracePath &trace_path = msg->trace_path();
    const PlanMAMsgPathOperator &op = trace_path.path(i);

    if (cost)
        *cost = op.cost();
    return op.name().c_str();
}

void planMAMsgSetHeurRequest(plan_ma_msg_t *_msg,
                             int agent_id,
                             long ref_id,
                             const int *state, int state_size,
                             int op_id)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgHeurRequest *req;

    msg->set_type(PlanMAMsg::HEUR_REQUEST);
    req = msg->mutable_heur_request();
    req->set_agent_id(agent_id);
    req->set_ref_id(ref_id);
    req->set_op_id(op_id);

    for (int i = 0; i < state_size; ++i)
        req->add_state(state[i]);
}

int planMAMsgIsHeurRequest(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::HEUR_REQUEST;
}

int planMAMsgHeurRequestAgentId(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurRequest &req = msg->heur_request();
    return req.agent_id();
}

int planMAMsgHeurRequestRefId(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurRequest &req = msg->heur_request();
    return req.ref_id();
}

int planMAMsgHeurRequestOpId(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurRequest &req = msg->heur_request();
    return req.op_id();
}

int planMAMsgHeurRequestState(const plan_ma_msg_t *_msg, int var)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurRequest &req = msg->heur_request();
    return req.state(var);
}


void planMAMsgSetHeurResponse(plan_ma_msg_t *_msg, long ref_id)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgHeurResponse *res;

    msg->set_type(PlanMAMsg::HEUR_RESPONSE);
    res = msg->mutable_heur_response();
    res->set_ref_id(ref_id);
}

void planMAMsgHeurResponseAddOp(plan_ma_msg_t *_msg, int op_id)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgHeurResponse *res = msg->mutable_heur_response();
    res->add_op(op_id);
}

void planMAMsgHeurResponseAddPeerOp(plan_ma_msg_t *_msg,
                                    int op_id, int owner)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgHeurResponse *res = msg->mutable_heur_response();
    PlanMAMsgHeurResponseOpOwner *op = res->add_peer_op();
    op->set_op_id(op_id);
    op->set_owner(owner);
}

int planMAMsgIsHeurResponse(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::HEUR_RESPONSE;
}

int planMAMsgHeurResponseRefId(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurResponse &res = msg->heur_response();
    return res.ref_id();
}

int planMAMsgHeurResponseOpSize(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurResponse &res = msg->heur_response();
    return res.op_size();
}

int planMAMsgHeurResponseOp(const plan_ma_msg_t *_msg, int i)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurResponse &res = msg->heur_response();
    return res.op(i);
}

int planMAMsgHeurResponsePeerOpSize(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurResponse &res = msg->heur_response();
    return res.peer_op_size();
}

int planMAMsgHeurResponsePeerOp(const plan_ma_msg_t *_msg, int i, int *owner)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurResponse &res = msg->heur_response();
    const PlanMAMsgHeurResponseOpOwner &op = res.peer_op(i);
    *owner = op.owner();
    return op.op_id();
}
