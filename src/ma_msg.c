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

#include <boruvka/alloc.h>

#include "plan/ma_msg.h"
#include "plan/msg_schema.h"

struct _plan_ma_msg_op_t {
    uint32_t header;
    int32_t op_id;
    int8_t *name;
    int name_size;
    int32_t cost;
    int32_t owner;
    int32_t value;
};

struct _plan_ma_msg_dtg_req_t {
    uint32_t header;
    int32_t var;
    int32_t val_from;
    int32_t val_to;
    int32_t *reachable;
    int reachable_size;
};

struct _plan_ma_msg_t {
    uint32_t header;
    int32_t type;
    int32_t agent_id;

    int32_t terminate_agent_id;

    int8_t *state_buf;
    int state_buf_size;
    int32_t *state_private_id;
    int state_private_id_size;
    int32_t state_id;
    int32_t state_cost;
    int32_t state_heur;

    int32_t initiator_agent_id;
    plan_ma_msg_op_t *op;
    int op_size;
    int64_t snapshot_token;
    int32_t snapshot_type;
    int32_t snapshot_ack;
    int32_t goal_op_id;
    int32_t min_cut_cost;
    int32_t heur_token;
    int32_t *heur_requested_agent;
    int heur_requested_agent_size;
    int32_t heur_cost;
    plan_ma_msg_dtg_req_t dtg_req;
    int32_t search_res;
};


PLAN_MSG_SCHEMA_BEGIN(schema_op)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_op_t, op_id, INT32)
PLAN_MSG_SCHEMA_ADD_ARR(plan_ma_msg_op_t, name, name_size, INT8)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_op_t, cost, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_op_t, owner, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_op_t, value, INT32)
PLAN_MSG_SCHEMA_END(schema_op, plan_ma_msg_op_t, header)
#define M_op_id 0x01u
#define M_name  0x02u
#define M_cost  0x04u
#define M_owner 0x08u
#define M_value 0x10u


PLAN_MSG_SCHEMA_BEGIN(schema_dtg_req)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_dtg_req_t, var, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_dtg_req_t, val_from, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_dtg_req_t, val_to, INT32)
PLAN_MSG_SCHEMA_ADD_ARR(plan_ma_msg_dtg_req_t, reachable, reachable_size, INT32)
PLAN_MSG_SCHEMA_END(schema_dtg_req, plan_ma_msg_dtg_req_t, header)
#define M_var       0x1u
#define M_val_from  0x2u
#define M_val_to    0x4u
#define M_reachable 0x8u


PLAN_MSG_SCHEMA_BEGIN(schema_msg)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, type, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, agent_id, INT32)

PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, terminate_agent_id, INT32)

PLAN_MSG_SCHEMA_ADD_ARR(plan_ma_msg_t, state_buf, state_buf_size, INT8)
PLAN_MSG_SCHEMA_ADD_ARR(plan_ma_msg_t, state_private_id, state_private_id_size, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, state_id, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, state_cost, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, state_heur, INT32)

PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, initiator_agent_id, INT32)
PLAN_MSG_SCHEMA_ADD_MSG_ARR(plan_ma_msg_t, op, op_size, &schema_op)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, snapshot_token, INT64)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, snapshot_type, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, snapshot_ack, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, goal_op_id, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, min_cut_cost, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, heur_token, INT32)
PLAN_MSG_SCHEMA_ADD_ARR(plan_ma_msg_t, heur_requested_agent, heur_requested_agent_size, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, heur_cost, INT32)
PLAN_MSG_SCHEMA_ADD_MSG(plan_ma_msg_t, dtg_req, &schema_dtg_req)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_t, search_res, INT32)
PLAN_MSG_SCHEMA_END(schema_msg, plan_ma_msg_t, header)
#define M_type                 0x000001u
#define M_agent_id             0x000002u
#define M_terminate_agent_id   0x000004u

#define M_state_buf            0x000008u
#define M_state_private_id     0x000010u
#define M_state_id             0x000020u
#define M_state_cost           0x000040u
#define M_state_heur           0x000080u

#define M_initiator_agent_id   0x000100u
#define M_op                   0x000200u
#define M_snapshot_token       0x000400u
#define M_snapshot_type        0x000800u
#define M_snapshot_ack         0x001000u
#define M_goal_op_id           0x002000u
#define M_min_cut_cost         0x004000u
#define M_heur_token           0x008000u
#define M_heur_requested_agent 0x010000u
#define M_heur_cost            0x020000u
#define M_dtg_req              0x040000u
#define M_search_res           0x080000u


#define SET_VAL(msg, member, val) \
    do { \
        (msg)->member = (val); \
        (msg)->header |= M_##member; \
    } while (0)

#define ADD_VAL(msg, member, val) \
    do { \
        (msg)->header |= M_##member; \
        ++(msg)->member##_size; \
        (msg)->member = BOR_REALLOC_ARR((msg)->member, \
                                        typeof(*((msg)->member)), \
                                        (msg)->member##_size); \
        (msg)->member[(msg)->member##_size - 1] = (val); \
    } while (0)

#define PREP_ARR(msg, member, src, src_size) \
    do { \
        if ((msg)->member != NULL){ \
            BOR_FREE((msg)->member); \
        } \
        int size = sizeof(*((msg)->member)); \
        (msg)->member = BOR_MALLOC((src_size) * size); \
        (msg)->member##_size = (src_size); \
    } while (0)

#define MEMCPY_ARR(msg, member, src, src_size) \
    do { \
        (msg)->header |= M_##member; \
        PREP_ARR(msg, member, src, src_size); \
        int size = sizeof(*((msg)->member)); \
        memcpy((msg)->member, (src), (src_size) * size); \
    } while (0)

#define CPY_ARR(msg, member, src, src_size) \
    do { \
        (msg)->header |= M_##member; \
        PREP_ARR(msg, member, src, src_size); \
        int i; \
        for (i = 0; i < src_size; ++i) \
            (msg)->member[i] = src[i]; \
    } while (0)


static int snapshot_token_counter = 0;

void planMAMsgOpFree(plan_ma_msg_op_t *op)
{
    if (op->name != NULL)
        BOR_FREE(op->name);
}

void planMAMsgOpCopy(plan_ma_msg_op_t *dst, const plan_ma_msg_op_t *src)
{
    *dst = *src;
    if (src->name != NULL){
        dst->name = BOR_ALLOC_ARR(int8_t, src->name_size);
        memcpy(dst->name, src->name, src->name_size);
    }
}

void planMAMsgDTGReqFree(plan_ma_msg_dtg_req_t *dr)
{
    if (dr->reachable != NULL)
        BOR_FREE(dr->reachable);
}

void planMAMsgDTGReqCopy(plan_ma_msg_dtg_req_t *dst,
                         const plan_ma_msg_dtg_req_t *src)
{
    *dst = *src;
    if (src->reachable != NULL){
        dst->reachable = BOR_ALLOC_ARR(int32_t, src->reachable_size);
        memcpy(dst->reachable, src->reachable,
               src->reachable_size * sizeof(int32_t));
    }
}


plan_ma_msg_t *planMAMsgNew(int type, int subtype, int agent_id)
{
    plan_ma_msg_t *msg;
    int32_t stype;

    msg = BOR_ALLOC(plan_ma_msg_t);
    bzero(msg, sizeof(*msg));
    stype = (subtype << 4) | type;
    SET_VAL(msg, type, stype);
    SET_VAL(msg, agent_id, agent_id);

    if (type == PLAN_MA_MSG_TRACE_PATH){
        SET_VAL(msg, initiator_agent_id, agent_id);

    }else if (type == PLAN_MA_MSG_SNAPSHOT
                && subtype == PLAN_MA_MSG_SNAPSHOT_INIT){
        uint64_t token = __sync_fetch_and_add(&snapshot_token_counter, 1);
        token = token << 32;
        token = token | (uint32_t)agent_id;
        SET_VAL(msg, snapshot_token, token);
    }

    return msg;
}

void planMAMsgDel(plan_ma_msg_t *msg)
{
    int i;

    if (msg->state_buf != NULL)
        BOR_FREE(msg->state_buf);
    if (msg->state_private_id != NULL)
        BOR_FREE(msg->state_private_id);
    if (msg->op != NULL){
        for (i = 0; i < msg->op_size; ++i)
            planMAMsgOpFree(msg->op + i);
        BOR_FREE(msg->op);
    }
    if (msg->heur_requested_agent != NULL)
        BOR_FREE(msg->heur_requested_agent);
    planMAMsgDTGReqFree(&msg->dtg_req);
    BOR_FREE(msg);
}

plan_ma_msg_t *planMAMsgClone(const plan_ma_msg_t *msg_in)
{
    plan_ma_msg_t *msg;
    int i;

    msg = BOR_ALLOC(plan_ma_msg_t);
    *msg = *msg_in;

    if (msg_in->state_buf != NULL){
        msg->state_buf = BOR_ALLOC_ARR(int8_t, msg_in->state_buf_size);
        memcpy(msg->state_buf, msg_in->state_buf, msg->state_buf_size);
    }

    if (msg_in->state_private_id != NULL){
        msg->state_private_id = BOR_ALLOC_ARR(int32_t, msg_in->state_private_id_size);
        memcpy(msg->state_private_id, msg_in->state_private_id,
               sizeof(int32_t) * msg->state_private_id_size);
    }

    if (msg_in->op != NULL){
        msg->op = BOR_ALLOC_ARR(plan_ma_msg_op_t, msg_in->op_size);
        for (i = 0; i < msg->op_size; ++i){
            planMAMsgOpCopy(msg->op + i, msg_in->op + i);
        }
    }
    planMAMsgDTGReqCopy(&msg->dtg_req, &msg_in->dtg_req);

    return msg;
}

int planMAMsgType(const plan_ma_msg_t *msg)
{
    return msg->type & 0xf;
}

int planMAMsgSubType(const plan_ma_msg_t *msg)
{
    return msg->type >> 4;
}

int planMAMsgAgent(const plan_ma_msg_t *msg)
{
    return msg->agent_id;
}

void *planMAMsgPacked(const plan_ma_msg_t *msg, size_t *size)
{
    int siz;
    void *buf;
    buf = planMsgBufEncode(msg, &schema_msg, &siz);
    *size = siz;
    return buf;
}

plan_ma_msg_t *planMAMsgUnpacked(void *buf, size_t size)
{
    plan_ma_msg_t *msg;

    msg = BOR_ALLOC(plan_ma_msg_t);
    bzero(msg, sizeof(plan_ma_msg_t));
    planMsgBufDecode(msg, &schema_msg, buf);
    return msg;
}

void planMAMsgSetStateBuf(plan_ma_msg_t *msg, const void *buf, size_t bufsize)
{
    MEMCPY_ARR(msg, state_buf, buf, bufsize);
}

void planMAMsgSetStatePrivateIds(plan_ma_msg_t *msg, const int *ids, int size)
{
    CPY_ARR(msg, state_private_id, ids, size);
}

void planMAMsgSetStateId(plan_ma_msg_t *msg, plan_state_id_t state_id)
{
    SET_VAL(msg, state_id, state_id);
}

void planMAMsgSetStateCost(plan_ma_msg_t *msg, int cost)
{
    SET_VAL(msg, state_cost, cost);
}

void planMAMsgSetStateHeur(plan_ma_msg_t *msg, int heur)
{
    SET_VAL(msg, state_heur, heur);
}

const void *planMAMsgStateBuf(const plan_ma_msg_t *msg)
{
    return msg->state_buf;
}

int planMAMsgStateBufSize(const plan_ma_msg_t *msg)
{
    return msg->state_buf_size;
}

int planMAMsgStatePrivateIdsSize(const plan_ma_msg_t *msg)
{
    return msg->state_private_id_size;
}

void planMAMsgStatePrivateIds(const plan_ma_msg_t *msg, int *ids)
{
    int i;
    for (i = 0; i < msg->state_private_id_size; ++i)
        ids[i] = msg->state_private_id[i];
}

int planMAMsgStateId(const plan_ma_msg_t *msg)
{
    return msg->state_id;
}

int planMAMsgStateCost(const plan_ma_msg_t *msg)
{
    return msg->state_cost;
}

int planMAMsgStateHeur(const plan_ma_msg_t *msg)
{
    return msg->state_heur;
}


void planMAMsgTerminateSetAgent(plan_ma_msg_t *msg, int agent_id)
{
    SET_VAL(msg, terminate_agent_id, agent_id);
}

int planMAMsgTerminateAgent(const plan_ma_msg_t *msg)
{
    return msg->terminate_agent_id;
}

void planMAMsgSetSearchRes(plan_ma_msg_t *msg, int res)
{
    SET_VAL(msg, search_res, res);
}

int planMAMsgSearchRes(const plan_ma_msg_t *msg)
{
    return msg->search_res;
}


void planMAMsgTracePathSetStateId(plan_ma_msg_t *msg, int state_id)
{
    SET_VAL(msg, state_id, state_id);
}

void planMAMsgTracePathAddPath(plan_ma_msg_t *msg, const plan_path_t *path)
{
    bor_list_t *item;
    const plan_path_op_t *p;
    plan_ma_msg_op_t *op;
    int i, size;

    for (size = 0, item = path->prev; item != path; item = item->prev, ++size);
    if (size == 0)
        return;

    msg->op = BOR_REALLOC_ARR(msg->op, plan_ma_msg_op_t, msg->op_size + size);
    op = msg->op + msg->op_size;
    msg->op_size += size;
    bzero(op, sizeof(plan_ma_msg_op_t) * size);

    for (i = 0, item = path->prev; item != path; item = item->prev, ++i){
        p = BOR_LIST_ENTRY(item, plan_path_op_t, path);

        SET_VAL(op + i, op_id, p->global_id);
        SET_VAL(op + i, cost, p->cost);
        SET_VAL(op + i, owner, p->owner);
        op[i].name_size = strlen(p->name) + 1;
        op[i].name = BOR_ALLOC_ARR(int8_t, op[i].name_size);
        strcpy((char *)op[i].name, p->name);
    }

    msg->header |= M_op;
}

int planMAMsgTracePathStateId(const plan_ma_msg_t *msg)
{
    return msg->state_id;
}

void planMAMsgTracePathExtractPath(const plan_ma_msg_t *msg,
                                   plan_path_t *path)
{
    const plan_ma_msg_op_t *op;
    int i, size;

    size = msg->op_size;
    for (i = 0; i < size; ++i){
        op = msg->op + i;
        planPathPrepend(path, (const char *)op->name, op->cost, op->op_id,
                        op->owner, PLAN_NO_STATE, PLAN_NO_STATE);
    }
}

int planMAMsgTracePathInitAgent(const plan_ma_msg_t *msg)
{
    return msg->initiator_agent_id;
}


void planMAMsgSnapshotSetType(plan_ma_msg_t *msg, int type)
{
    SET_VAL(msg, snapshot_type, type);
}

int planMAMsgSnapshotType(const plan_ma_msg_t *msg)
{
    return msg->snapshot_type;
}

long planMAMsgSnapshotToken(const plan_ma_msg_t *msg)
{
    return msg->snapshot_token;
}

plan_ma_msg_t *planMAMsgSnapshotNewMark(const plan_ma_msg_t *snapshot_init,
                                        int agent_id)
{
    plan_ma_msg_t *msg = planMAMsgNew(PLAN_MA_MSG_SNAPSHOT,
                                      PLAN_MA_MSG_SNAPSHOT_MARK, agent_id);
    SET_VAL(msg, snapshot_token, planMAMsgSnapshotToken(snapshot_init));
    planMAMsgSnapshotSetType(msg, planMAMsgSnapshotType(snapshot_init));
    return msg;
}

plan_ma_msg_t *planMAMsgSnapshotNewResponse(const plan_ma_msg_t *sshot_init,
                                            int agent_id)
{
    plan_ma_msg_t *msg = planMAMsgNew(PLAN_MA_MSG_SNAPSHOT,
                                      PLAN_MA_MSG_SNAPSHOT_RESPONSE, agent_id);
    SET_VAL(msg, snapshot_token, planMAMsgSnapshotToken(sshot_init));
    planMAMsgSnapshotSetType(msg, planMAMsgSnapshotType(sshot_init));
    return msg;
}

void planMAMsgSnapshotSetAck(plan_ma_msg_t *msg, int ack)
{
    SET_VAL(msg, snapshot_ack, ack);
}

int planMAMsgSnapshotAck(const plan_ma_msg_t *msg)
{
    return msg->snapshot_ack;
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
    SET_VAL(msg, goal_op_id, goal_op_id);
}

int planMAMsgGoalOpId(const plan_ma_msg_t *msg)
{
    return msg->goal_op_id;
}


void planMAMsgAddOp(plan_ma_msg_t *msg, int op_id, plan_cost_t cost,
                    int owner, plan_cost_t value)
{
    plan_ma_msg_op_t *op;

    ++msg->op_size;
    msg->op = BOR_REALLOC_ARR(msg->op, plan_ma_msg_op_t, msg->op_size);
    op = msg->op + msg->op_size - 1;
    bzero(op, sizeof(*op));

    SET_VAL(op, op_id, op_id);

    if (cost != PLAN_COST_INVALID)
        SET_VAL(op, cost, cost);
    if (owner >= 0)
        SET_VAL(op, owner, owner);
    if (value != PLAN_COST_INVALID)
        SET_VAL(op, value, value);

    msg->header |= M_op;
}

int planMAMsgOpSize(const plan_ma_msg_t *msg)
{
    return msg->op_size;
}

int planMAMsgOp(const plan_ma_msg_t *msg, int i,
                plan_cost_t *cost, int *owner, plan_cost_t *value)
{
    const plan_ma_msg_op_t *op = msg->op + i;

    if (cost)
        *cost = op->cost;
    if (owner)
        *owner = op->owner;
    if (value)
        *value = op->value;
    return op->op_id;
}

void planMAMsgSetMinCutCost(plan_ma_msg_t *msg, plan_cost_t cost)
{
    SET_VAL(msg, min_cut_cost, cost);
}

plan_cost_t planMAMsgMinCutCost(const plan_ma_msg_t *msg)
{
    return msg->min_cut_cost;
}

void planMAMsgSetHeurToken(plan_ma_msg_t *msg, int token)
{
    SET_VAL(msg, heur_token, token);
}

int planMAMsgHeurToken(const plan_ma_msg_t *msg)
{
    return msg->heur_token;
}

void planMAMsgAddHeurRequestedAgent(plan_ma_msg_t *msg, int agent_id)
{
    ADD_VAL(msg, heur_requested_agent, agent_id);
}

int planMAMsgHeurRequestedAgentSize(const plan_ma_msg_t *msg)
{
    return msg->heur_requested_agent_size;
}

int planMAMsgHeurRequestedAgent(const plan_ma_msg_t *msg, int i)
{
    return msg->heur_requested_agent[i];
}

void planMAMsgSetHeurCost(plan_ma_msg_t *msg, int cost)
{
    SET_VAL(msg, heur_cost, cost);
}

int planMAMsgHeurCost(const plan_ma_msg_t *msg)
{
    return msg->heur_cost;
}

void planMAMsgSetDTGReq(plan_ma_msg_t *msg, int var, int from, int to)
{
    plan_ma_msg_dtg_req_t *dr;

    msg->header |= M_dtg_req;
    dr = &msg->dtg_req;
    SET_VAL(dr, var, var);
    SET_VAL(dr, val_from, from);
    SET_VAL(dr, val_to, to);
}

void planMAMsgAddDTGReqReachable(plan_ma_msg_t *msg, int val)
{
    plan_ma_msg_dtg_req_t *dr;
    msg->header |= M_dtg_req;
    dr = &msg->dtg_req;
    ADD_VAL(dr, reachable, val);
}

void planMAMsgDTGReq(const plan_ma_msg_t *msg, int *var, int *from, int *to)
{
    const plan_ma_msg_dtg_req_t *req;
    req = &msg->dtg_req;
    *var = req->var;
    *from = req->val_from;
    *to = req->val_to;
}

int planMAMsgDTGReqReachableSize(const plan_ma_msg_t *msg)
{
    return msg->dtg_req.reachable_size;
}

int planMAMsgDTGReqReachable(const plan_ma_msg_t *msg, int i)
{
    return msg->dtg_req.reachable[i];
}

void planMAMsgCopyDTGReqReachable(plan_ma_msg_t *dst, const plan_ma_msg_t *src)
{
    plan_ma_msg_dtg_req_t *dreq = &dst->dtg_req;
    const plan_ma_msg_dtg_req_t *sreq = &src->dtg_req;
    MEMCPY_ARR(dreq, reachable, sreq->reachable, sreq->reachable_size);
}

void planMAMsgSetInitAgent(plan_ma_msg_t *msg, int agent_id)
{
    SET_VAL(msg, initiator_agent_id, agent_id);
}

int planMAMsgInitAgent(const plan_ma_msg_t *msg)
{
    return msg->initiator_agent_id;
}
