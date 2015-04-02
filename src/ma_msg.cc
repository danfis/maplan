#include <boruvka/alloc.h>

#include "plan/ma_msg.h"
#include "ma_msg.pb.h"

#define PROTO(msg) ((PlanMAMsg *)(msg))

#define SETVAL(msg, name, value) \
    PROTO(msg)->set_##name(value)
#define GETVAL(msg, name) \
    PROTO(msg)->name()
#define ADDVAL(msg, name, value) \
    PROTO(msg)->add_##name(value)
#define GETARRVAL(msg, name, i) \
    PROTO(msg)->name(i)
#define ARRSIZE(msg, name) \
    PROTO(msg)->name##_size()

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

void planMAMsgSetStateBuf(plan_ma_msg_t *msg, const void *buf, size_t bufsize)
{
    PlanMAMsg *proto = PROTO(msg);
    proto->set_state_buf(buf, bufsize);
}

void planMAMsgSetStatePrivateIds(plan_ma_msg_t *msg, const int *ids, int size)
{
    PlanMAMsg *m = PROTO(msg);
    for (int i = 0; i < size; ++i)
        m->add_state_private_id(ids[i]);
}

void planMAMsgSetStateId(plan_ma_msg_t *msg, plan_state_id_t state_id)
{
    SETVAL(msg, state_id, state_id);
}

void planMAMsgSetStateCost(plan_ma_msg_t *msg, int cost)
{
    SETVAL(msg, state_cost, cost);
}

void planMAMsgSetStateHeur(plan_ma_msg_t *msg, int heur)
{
    SETVAL(msg, state_heur, heur);
}

const void *planMAMsgStateBuf(const plan_ma_msg_t *msg)
{
    return GETVAL(msg, state_buf).data();
}

int planMAMsgStateBufSize(const plan_ma_msg_t *msg)
{
    return GETVAL(msg, state_buf).size();
}

int planMAMsgStatePrivateIdsSize(const plan_ma_msg_t *msg)
{
    return GETVAL(msg, state_private_id_size);
}

void planMAMsgStatePrivateIds(const plan_ma_msg_t *msg, int *ids)
{
    const PlanMAMsg *m = PROTO(msg);
    int size = m->state_private_id_size();

    for (int i = 0; i < size; ++i)
        ids[i] = m->state_private_id(i);
}

int planMAMsgStateId(const plan_ma_msg_t *msg)
{
    return GETVAL(msg, state_id);
}

int planMAMsgStateCost(const plan_ma_msg_t *msg)
{
    return GETVAL(msg, state_cost);
}

int planMAMsgStateHeur(const plan_ma_msg_t *msg)
{
    return GETVAL(msg, state_heur);
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

    if ((subtype & 0x00ff) == subtype)
        return PLAN_MA_MSG_HEUR_REQUEST;

    if ((subtype & 0xff00) == subtype)
        return PLAN_MA_MSG_HEUR_UPDATE;

    return PLAN_MA_MSG_HEUR_NONE;
}


void planMAMsgSetGoalOpId(plan_ma_msg_t *msg, int goal_op_id)
{
    SETVAL(msg, goal_op_id, goal_op_id);
}

int planMAMsgGoalOpId(const plan_ma_msg_t *msg)
{
    return GETVAL(msg, goal_op_id);
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

void planMAMsgSetMinCutCost(plan_ma_msg_t *msg, plan_cost_t cost)
{
    PlanMAMsg *proto = PROTO(msg);
    proto->set_min_cut_cost(cost);
}

plan_cost_t planMAMsgMinCutCost(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    return proto->min_cut_cost();
}

void planMAMsgSetHeurToken(plan_ma_msg_t *msg, int token)
{
    SETVAL(msg, heur_token, token);
}

int planMAMsgHeurToken(const plan_ma_msg_t *msg)
{
    return GETVAL(msg, heur_token);
}

void planMAMsgAddHeurRequestedAgent(plan_ma_msg_t *msg, int agent_id)
{
    ADDVAL(msg, heur_requested_agent, agent_id);
}

int planMAMsgHeurRequestedAgentSize(const plan_ma_msg_t *msg)
{
    return ARRSIZE(msg, heur_requested_agent);
}

int planMAMsgHeurRequestedAgent(const plan_ma_msg_t *msg, int i)
{
    return GETARRVAL(msg, heur_requested_agent, i);
}

void planMAMsgSetHeurCost(plan_ma_msg_t *msg, int cost)
{
    SETVAL(msg, heur_cost, cost);
}

int planMAMsgHeurCost(const plan_ma_msg_t *msg)
{
    return GETVAL(msg, heur_cost);
}

void planMAMsgSetDTGReq(plan_ma_msg_t *msg, int var, int from, int to)
{
    PlanMAMsg *proto = PROTO(msg);
    PlanMAMsgDTGReq *req = proto->mutable_dtg_req();
    req->set_var(var);
    req->set_val_from(from);
    req->set_val_to(to);
}

void planMAMsgAddDTGReqReachable(plan_ma_msg_t *msg, int val)
{
    PlanMAMsg *proto = PROTO(msg);
    PlanMAMsgDTGReq *req = proto->mutable_dtg_req();
    req->add_reachable(val);
}

void planMAMsgDTGReq(const plan_ma_msg_t *msg, int *var, int *from, int *to)
{
    const PlanMAMsg *proto = PROTO(msg);
    const PlanMAMsgDTGReq &req = proto->dtg_req();
    *var = req.var();
    *from = req.val_from();
    *to = req.val_to();
}

int planMAMsgDTGReqReachableSize(const plan_ma_msg_t *msg)
{
    const PlanMAMsg *proto = PROTO(msg);
    const PlanMAMsgDTGReq &req = proto->dtg_req();
    return req.reachable_size();
}

int planMAMsgDTGReqReachable(const plan_ma_msg_t *msg, int i)
{
    const PlanMAMsg *proto = PROTO(msg);
    const PlanMAMsgDTGReq &req = proto->dtg_req();
    return req.reachable(i);
}

void planMAMsgCopyDTGReqReachable(plan_ma_msg_t *dst, const plan_ma_msg_t *src)
{
    PlanMAMsg *pdst = PROTO(dst);
    const PlanMAMsg *psrc = PROTO(src);
    PlanMAMsgDTGReq *dreq = pdst->mutable_dtg_req();
    const PlanMAMsgDTGReq &sreq = psrc->dtg_req();
    int i, size;

    dreq->clear_reachable();
    size = sreq.reachable_size();
    for (i = 0; i < size; ++i)
        dreq->add_reachable(sreq.reachable(i));
}

void planMAMsgSetInitAgent(plan_ma_msg_t *msg, int agent_id)
{
    SETVAL(msg, initiator_agent_id, agent_id);
}

int planMAMsgInitAgent(const plan_ma_msg_t *msg)
{
    return GETVAL(msg, initiator_agent_id);
}
