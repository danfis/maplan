#include <boruvka/alloc.h>

#include "plan/ma_msg.h"
#include "ma_msg.pb.h"

#define PROTO(msg) ((PlanMAMsg *)(msg))

static int snapshot_token_counter = 0;

static int stateId(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->state_id();
}

void planShutdownProtobuf(void)
{
    google::protobuf::ShutdownProtobufLibrary();
}

plan_ma_msg_t *planMAMsgNew(int type, int subtype, int agent_id)
{
    int proto_type;
    PlanMAMsg *protobuf;

    protobuf = new PlanMAMsg;
    proto_type = (subtype << 4) | type;
    protobuf->set_type(proto_type);
    protobuf->set_agent_id(agent_id);

    if (type == PLAN_MA_MSG_TRACE_PATH){
        protobuf->set_initiator_agent_id(agent_id);

    }else if (type == PLAN_MA_MSG_SNAPSHOT
                && subtype == PLAN_MA_MSG_SNAPSHOT_INIT){
        uint64_t token = __sync_fetch_and_add(&snapshot_token_counter, 1);
        token = token << 32;
        token = token | (uint32_t)agent_id;
        protobuf->set_snapshot_token(token);
    }

    return (plan_ma_msg_t *)protobuf;
}

void planMAMsgDel(plan_ma_msg_t *msg)
{
    PlanMAMsg *proto = PROTO(msg);
    delete proto;
}

plan_ma_msg_t *planMAMsgClone(const plan_ma_msg_t *msg_in)
{
    const PlanMAMsg *proto_in = PROTO(msg_in);
    PlanMAMsg *proto;
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(planMAMsgType(msg_in), planMAMsgSubType(msg_in),
                       planMAMsgAgent(msg_in));
    proto = PROTO(msg);
    *proto = *proto_in;
    return msg;
}

int planMAMsgType(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->type() & 0xf;
}

int planMAMsgSubType(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->type() >> 4;
}

int planMAMsgAgent(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->agent_id();
}

void *planMAMsgPacked(const plan_ma_msg_t *msg, size_t *size)
{
    const PlanMAMsg *proto = PROTO(msg);
    void *buf;

    *size = proto->ByteSize();
    buf = BOR_ALLOC_ARR(char, *size);
    proto->SerializeToArray(buf, *size);
    return buf;
}

plan_ma_msg_t *planMAMsgUnpacked(void *buf, size_t size)
{
    PlanMAMsg *proto = new PlanMAMsg;
    proto->ParseFromArray(buf, size);
    return (plan_ma_msg_t *)proto;
}


void planMAMsgTerminateSetAgent(plan_ma_msg_t *msg, int agent_id)
{
    PlanMAMsg *proto = PROTO(msg);
    proto->set_terminate_agent_id(agent_id);
}

int planMAMsgTerminateAgent(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->terminate_agent_id();
}

void planMAMsgPublicStateSetState(plan_ma_msg_t *msg,
                                  const void *statebuf,
                                  size_t statebuf_size,
                                  int state_id, int cost, int heur)
{
    PlanMAMsg *proto = PROTO(msg);
    proto->set_state(statebuf, statebuf_size);
    proto->set_state_id(state_id);
    proto->set_cost(cost);
    proto->set_heur(heur);
}

const void *planMAMsgPublicStateStateBuf(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->state().data();
}

int planMAMsgPublicStateStateId(const plan_ma_msg_t *msg)
{
    return stateId(msg);
}

int planMAMsgPublicStateCost(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->cost();
}

int planMAMsgPublicStateHeur(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->heur();
}

void planMAMsgTracePathSetStateId(plan_ma_msg_t *msg, int state_id)
{
    PlanMAMsg *proto = PROTO(msg);
    proto->set_state_id(state_id);
}

void planMAMsgTracePathAddPath(plan_ma_msg_t *msg, const plan_path_t *path)
{
    PlanMAMsg *proto = PROTO(msg);
    bor_list_t *item;
    const plan_path_op_t *p;

    for (item = path->prev; item != path; item = item->prev){
        p = BOR_LIST_ENTRY(item, plan_path_op_t, path);

        PlanMAMsgOp *op = proto->add_op();
        op->set_name(p->name);
        op->set_cost(p->cost);
    }
}

int planMAMsgTracePathStateId(const plan_ma_msg_t *msg)
{
    return stateId(msg);
}

void planMAMsgTracePathExtractPath(const plan_ma_msg_t *msg,
                                   plan_path_t *path)
{
    const PlanMAMsg *proto = PROTO(msg);
    int size;

    size = proto->op_size();
    for (int i = 0; i < size; ++i){
        const PlanMAMsgOp &op = proto->op(i);
        planPathPrepend2(path, op.name().c_str(), op.cost());
    }
}

int planMAMsgTracePathInitAgent(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->initiator_agent_id();
}


void planMAMsgSnapshotSetType(plan_ma_msg_t *msg, int type)
{
    PlanMAMsg *proto = PROTO(msg);
    proto->set_snapshot_type(type);
}

int planMAMsgSnapshotType(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->snapshot_type();
}

long planMAMsgSnapshotToken(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->snapshot_token();
}

plan_ma_msg_t *planMAMsgSnapshotNewMark(const plan_ma_msg_t *snapshot_init,
                                        int agent_id)
{
    plan_ma_msg_t *msg = planMAMsgNew(PLAN_MA_MSG_SNAPSHOT,
                                      PLAN_MA_MSG_SNAPSHOT_MARK, agent_id);
    PlanMAMsg *proto = PROTO(msg);
    proto->set_snapshot_token(planMAMsgSnapshotToken(snapshot_init));
    planMAMsgSnapshotSetType(msg, planMAMsgSnapshotType(snapshot_init));
    return msg;
}

plan_ma_msg_t *planMAMsgSnapshotNewResponse(const plan_ma_msg_t *sshot_init,
                                            int agent_id)
{
    plan_ma_msg_t *msg = planMAMsgNew(PLAN_MA_MSG_SNAPSHOT,
                                      PLAN_MA_MSG_SNAPSHOT_RESPONSE, agent_id);
    PlanMAMsg *proto = PROTO(msg);
    proto->set_snapshot_token(planMAMsgSnapshotToken(sshot_init));
    planMAMsgSnapshotSetType(msg, planMAMsgSnapshotType(sshot_init));
    return msg;
}

void planMAMsgSnapshotSetAck(plan_ma_msg_t *msg, int ack)
{
    PlanMAMsg *proto = PROTO(msg);
    proto->set_snapshot_ack(ack);
}

int planMAMsgSnapshotAck(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->snapshot_ack();
}


int planMAMsgHeurType(const plan_ma_msg_t *msg)
{
    int type = planMAMsgType(msg);
    int subtype = planMAMsgSubType(msg);

    if (type != PLAN_MA_MSG_HEUR)
        return PLAN_MA_MSG_HEUR_NONE;

    if ((subtype & 0x0f) == subtype)
        return PLAN_MA_MSG_HEUR_REQUEST;

    if ((subtype & 0xf0) == subtype)
        return PLAN_MA_MSG_HEUR_UPDATE;

    return PLAN_MA_MSG_HEUR_NONE;
}


void planMAMsgHeurFFSetRequest(plan_ma_msg_t *msg,
                               const int *init_state, int init_state_size,
                               int goal_op_id)
{
    PlanMAMsg *proto = PROTO(msg);
    planMAMsgSetStateFull(msg, init_state, init_state_size);
    proto->set_goal_op_id(goal_op_id);
}

void planMAMsgHeurFFSetResponse(plan_ma_msg_t *msg, int goal_op_id)
{
    PlanMAMsg *proto = PROTO(msg);
    proto->set_goal_op_id(goal_op_id);
}

int planMAMsgHeurFFOpId(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->goal_op_id();
}






void planMAMsgSetStateFull(plan_ma_msg_t *msg, const int *state, int size)
{
    PlanMAMsg *proto = PROTO(msg);

    for (int i = 0; i < size; ++i)
        proto->add_state_full(state[i]);
}

void planMAMsgStateFull(const plan_ma_msg_t *msg, plan_state_t *state)
{
    const PlanMAMsg *proto = PROTO(msg);
    int len = proto->state_full_size();
    for (int i = 0; i < len; ++i)
        planStateSet(state, i, proto->state_full(i));
}

plan_val_t planMAMsgStateFullVal(const plan_ma_msg_t *msg, plan_var_id_t var)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->state_full(var);
}

void planMAMsgAddOp(plan_ma_msg_t *msg, int op_id, plan_cost_t cost,
                    int owner, plan_cost_t value)
{
    PlanMAMsg *proto = PROTO(msg);
    PlanMAMsgOp *op = proto->add_op();
    op->set_op_id(op_id);

    if (cost != PLAN_COST_INVALID)
        op->set_cost(cost);
    if (owner >= 0)
        op->set_owner(owner);
    if (value != PLAN_COST_INVALID)
        op->set_value(value);
}

int planMAMsgOpSize(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->op_size();
}

int planMAMsgOp(const plan_ma_msg_t *msg, int i,
                plan_cost_t *cost, int *owner, plan_cost_t *value)
{
    const PlanMAMsg *proto = PROTO(msg);
    const PlanMAMsgOp &op = proto->op(i);

    if (cost)
        *cost = op.cost();
    if (owner)
        *owner = op.owner();
    if (value)
        *value = op.value();
    return op.op_id();
}

#if 0
static void setPublicState(PlanMAMsgPublicState *public_state, int agent_id,
                           const void *state, size_t state_size,
                           int state_id,
                           int cost, int heuristic)
{
    public_state->set_agent_id(agent_id);
    public_state->set_state(state, state_size);
    public_state->set_state_id(state_id);
    public_state->set_cost(cost);
    public_state->set_heuristic(heuristic);
}

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

int planMAMsgIsType(const plan_ma_msg_t *msg, int msg_type)
{
    int type = planMAMsgType(msg);
    msg_type = msg_type << 8;
    if (type & msg_type)
        return 1;
    return 0;
}

int planMAMsgType(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type();
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
    setPublicState(public_state, agent_id, state, state_size,
                   state_id, cost, heuristic);
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

int planMAMsgIsTerminateType(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::TERMINATE
            || msg->type() == PlanMAMsg::TERMINATE_REQUEST;
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

void planMAMsgSetTerminateRequest(plan_ma_msg_t *_msg, int agent_id)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgTerminateRequest *req = msg->mutable_terminate_request();
    msg->set_type(PlanMAMsg::TERMINATE_REQUEST);
    req->set_agent_id(agent_id);
}

int planMAMsgIsTerminateRequest(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::TERMINATE_REQUEST;
}

int planMAMsgTerminateRequestAgent(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->terminate_request().agent_id();
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

int planMAMsgIsHeurRequest(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    int type = msg->type();
    return (type & 0x0810) == 0x0810;
}

int planMAMsgIsHeurResponse(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    int type = msg->type();
    return (type & 0x0820) == 0x0820;
}

void planMAMsgSetHeurFFRequest(plan_ma_msg_t *_msg,
                               int agent_id,
                               const int *state, int state_size,
                               int op_id)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgHeurFFRequest *req;

    msg->set_type(PlanMAMsg::HEUR_FF_REQUEST);
    req = msg->mutable_heur_ff_request();
    req->set_agent_id(agent_id);
    req->set_op_id(op_id);

    for (int i = 0; i < state_size; ++i)
        req->add_state(state[i]);
}

int planMAMsgIsHeurFFRequest(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::HEUR_FF_REQUEST;
}

int planMAMsgHeurFFRequestAgentId(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurFFRequest &req = msg->heur_ff_request();
    return req.agent_id();
}

int planMAMsgHeurFFRequestOpId(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurFFRequest &req = msg->heur_ff_request();
    return req.op_id();
}

int planMAMsgHeurFFRequestState(const plan_ma_msg_t *_msg, int var)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurFFRequest &req = msg->heur_ff_request();
    return req.state(var);
}


void planMAMsgSetHeurFFResponse(plan_ma_msg_t *_msg, int op_id)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgHeurFFResponse *res;

    msg->set_type(PlanMAMsg::HEUR_FF_RESPONSE);
    res = msg->mutable_heur_ff_response();
    res->set_op_id(op_id);
}

void planMAMsgHeurFFResponseAddOp(plan_ma_msg_t *_msg, int op_id, int cost)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgHeurFFResponse *res = msg->mutable_heur_ff_response();
    PlanMAMsgHeurFFResponseOp *op = res->add_op();
    op->set_op_id(op_id);
    op->set_cost(cost);
}

void planMAMsgHeurFFResponseAddPeerOp(plan_ma_msg_t *_msg,
                                      int op_id, int cost, int owner)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgHeurFFResponse *res = msg->mutable_heur_ff_response();
    PlanMAMsgHeurFFResponseOp *op = res->add_peer_op();
    op->set_op_id(op_id);
    op->set_cost(cost);
    op->set_owner(owner);
}

int planMAMsgIsHeurFFResponse(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::HEUR_FF_RESPONSE;
}

int planMAMsgHeurFFResponseOpId(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurFFResponse &res = msg->heur_ff_response();
    return res.op_id();
}

int planMAMsgHeurFFResponseOpSize(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurFFResponse &res = msg->heur_ff_response();
    return res.op_size();
}

int planMAMsgHeurFFResponseOp(const plan_ma_msg_t *_msg, int i, int *cost)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurFFResponse &res = msg->heur_ff_response();
    const PlanMAMsgHeurFFResponseOp &op = res.op(i);
    *cost = op.cost();
    return op.op_id();
}

int planMAMsgHeurFFResponsePeerOpSize(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurFFResponse &res = msg->heur_ff_response();
    return res.peer_op_size();
}

int planMAMsgHeurFFResponsePeerOp(const plan_ma_msg_t *_msg, int i,
                                  int *cost, int *owner)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurFFResponse &res = msg->heur_ff_response();
    const PlanMAMsgHeurFFResponseOp &op = res.peer_op(i);
    *cost = op.cost();
    *owner = op.owner();
    return op.op_id();
}



void planMAMsgSetHeurMaxRequest(plan_ma_msg_t *_msg, int agent_id, 
                                const int *state, int state_size)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgHeurMaxRequest *req;

    msg->set_type(PlanMAMsg::HEUR_MAX_REQUEST);
    req = msg->mutable_heur_max_request();
    req->set_agent_id(agent_id);
    for (int i = 0; i < state_size; ++i)
        req->add_state(state[i]);
}

void planMAMsgHeurMaxRequestAddOp(plan_ma_msg_t *_msg, int op_id, int val)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgHeurMaxRequest *req = msg->mutable_heur_max_request();
    PlanMAMsgHeurMaxOp *op;
    op = req->add_op();
    op->set_op_id(op_id);
    op->set_value(val);
}

int planMAMsgIsHeurMaxRequest(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::HEUR_MAX_REQUEST;
}

int planMAMsgHeurMaxRequestAgent(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurMaxRequest &req = msg->heur_max_request();
    return req.agent_id();
}

int planMAMsgHeurMaxRequestState(const plan_ma_msg_t *_msg, int var)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurMaxRequest &req = msg->heur_max_request();
    return req.state(var);
}

int planMAMsgHeurMaxRequestOpSize(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurMaxRequest &req = msg->heur_max_request();
    return req.op_size();
}

int planMAMsgHeurMaxRequestOp(const plan_ma_msg_t *_msg, int i, int *value)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurMaxRequest &req = msg->heur_max_request();
    const PlanMAMsgHeurMaxOp &op = req.op(i);
    *value = op.value();
    return op.op_id();
}


void planMAMsgSetHeurMaxResponse(plan_ma_msg_t *_msg, int agent_id)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgHeurMaxResponse *res = msg->mutable_heur_max_response();
    msg->set_type(PlanMAMsg::HEUR_MAX_RESPONSE);
    res->set_agent_id(agent_id);
}

void planMAMsgHeurMaxResponseAddOp(plan_ma_msg_t *_msg, int op_id, int val)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgHeurMaxResponse *res = msg->mutable_heur_max_response();
    PlanMAMsgHeurMaxOp *op = res->add_op();
    op->set_op_id(op_id);
    op->set_value(val);
}

int planMAMsgIsHeurMaxResponse(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::HEUR_MAX_RESPONSE;
}

int planMAMsgHeurMaxResponseAgent(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurMaxResponse &res = msg->heur_max_response();
    return res.agent_id();
}

int planMAMsgHeurMaxResponseOpSize(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurMaxResponse &res = msg->heur_max_response();
    return res.op_size();
}

int planMAMsgHeurMaxResponseOp(const plan_ma_msg_t *_msg, int i, int *value)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgHeurMaxResponse &res = msg->heur_max_response();
    const PlanMAMsgHeurMaxOp &op = res.op(i);
    *value = op.value();
    return op.op_id();
}




void planMAMsgSetSolution(plan_ma_msg_t *_msg, int agent_id,
                          const void *state, size_t state_size,
                          int state_id,
                          int cost, int heuristic,
                          int token)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    PlanMAMsgSolution *sol = msg->mutable_solution();
    msg->set_type(PlanMAMsg::SOLUTION);
    PlanMAMsgPublicState *pstate = sol->mutable_state();
    setPublicState(pstate, agent_id, state, state_size,
                   state_id, cost, heuristic);
    sol->set_token(token);
}

int planMAMsgIsSolution(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::SOLUTION;
}

plan_ma_msg_t *planMAMsgSolutionPublicState(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgSolution &sol = msg->solution();
    PlanMAMsg *public_state = new PlanMAMsg();
    public_state->set_type(PlanMAMsg::SEARCH_PUBLIC_STATE);
    *public_state->mutable_public_state() = sol.state();
    return public_state;
}

int planMAMsgSolutionToken(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgSolution &sol = msg->solution();
    return sol.token();
}

int planMAMsgSolutionCost(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgSolution &sol = msg->solution();
    return sol.state().cost();
}

int planMAMsgSolutionAgent(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgSolution &sol = msg->solution();
    return sol.state().agent_id();
}

int planMAMsgSolutionStateId(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgSolution &sol = msg->solution();
    return sol.state().state_id();
}


void planMAMsgSetSolutionAck(plan_ma_msg_t *_msg, int agent_id,
                             int ack, int token)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    msg->set_type(PlanMAMsg::SOLUTION_ACK);
    PlanMAMsgSolutionAck *sol = msg->mutable_solution_ack();
    sol->set_agent_id(agent_id);
    sol->set_ack(ack);
    sol->set_token(token);
}

int planMAMsgIsSolutionAck(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::SOLUTION_ACK;
}

int planMAMsgSolutionAck(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgSolutionAck &res = msg->solution_ack();
    return res.ack();
}

int planMAMsgSolutionAckAgent(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgSolutionAck &res = msg->solution_ack();
    return res.agent_id();
}

int planMAMsgSolutionAckToken(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgSolutionAck &res = msg->solution_ack();
    return res.token();
}



void planMAMsgSetSolutionMark(plan_ma_msg_t *_msg, int agent_id, int token)
{
    PlanMAMsg *msg = static_cast<PlanMAMsg *>(_msg);
    msg->set_type(PlanMAMsg::SOLUTION_MARK);
    PlanMAMsgSolutionMark *sol = msg->mutable_solution_mark();
    sol->set_agent_id(agent_id);
    sol->set_token(token);
}

int planMAMsgIsSolutionMark(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    return msg->type() == PlanMAMsg::SOLUTION_MARK;
}

int planMAMsgSolutionMarkAgent(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgSolutionMark &res = msg->solution_mark();
    return res.agent_id();
}

int planMAMsgSolutionMarkToken(const plan_ma_msg_t *_msg)
{
    const PlanMAMsg *msg = static_cast<const PlanMAMsg *>(_msg);
    const PlanMAMsgSolutionMark &res = msg->solution_mark();
    return res.token();
}
#endif
