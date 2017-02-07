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

struct _plan_ma_msg_pot_submatrix_t {
    uint32_t header;
    int32_t cols;
    int32_t rows;
    int32_t *coef;
    int coef_size;
};
typedef struct _plan_ma_msg_pot_submatrix_t plan_ma_msg_pot_submatrix_t;

struct _plan_ma_msg_pot_agent_t {
    uint32_t header;
    plan_ma_msg_pot_submatrix_t pub_op;
    plan_ma_msg_pot_submatrix_t priv_op;
    plan_ma_msg_pot_submatrix_t maxpot;
    int32_t *op_cost;
    int op_cost_size;
    int32_t *goal;
    int goal_size;
    int32_t *coef;
    int coef_size;
};
typedef struct _plan_ma_msg_pot_agent_t plan_ma_msg_pot_agent_t;

struct _plan_ma_msg_pot_t {
    uint32_t header;
    int32_t *fact_range;
    int fact_range_size;
    int32_t lp_private_var_size;

    int32_t fact_range_lcm;
    int32_t lp_var_size;
    int32_t lp_var_offset;
    int64_t *pot;
    int pot_size;
    int64_t init_heur;

    plan_ma_msg_pot_agent_t agent;
};
typedef struct _plan_ma_msg_pot_t plan_ma_msg_pot_t;

struct _plan_ma_msg_op_t {
    uint32_t header;
    int32_t op_id;
    int32_t cost;
    int32_t owner;
    int32_t value;
    int8_t *name;
    int name_size;
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

    plan_ma_msg_op_t *op;
    int op_size;

    plan_ma_msg_pot_t pot;
};

PLAN_MSG_SCHEMA_BEGIN(schema_pot_submatrix)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_pot_submatrix_t, cols, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_pot_submatrix_t, rows, INT32)
PLAN_MSG_SCHEMA_ADD_ARR(plan_ma_msg_pot_submatrix_t, coef, coef_size, INT32)
PLAN_MSG_SCHEMA_END(schema_pot_submatrix, plan_ma_msg_pot_submatrix_t, header)
#define M_pot_submatrix_cols 0x01u
#define M_pot_submatrix_rows 0x02u
#define M_pot_submatrix_coef 0x04u

PLAN_MSG_SCHEMA_BEGIN(schema_pot_agent)
PLAN_MSG_SCHEMA_ADD_MSG(plan_ma_msg_pot_agent_t, pub_op, &schema_pot_submatrix)
PLAN_MSG_SCHEMA_ADD_MSG(plan_ma_msg_pot_agent_t, priv_op, &schema_pot_submatrix)
PLAN_MSG_SCHEMA_ADD_MSG(plan_ma_msg_pot_agent_t, maxpot, &schema_pot_submatrix)
PLAN_MSG_SCHEMA_ADD_ARR(plan_ma_msg_pot_agent_t, op_cost, op_cost_size, INT32)
PLAN_MSG_SCHEMA_ADD_ARR(plan_ma_msg_pot_agent_t, goal, goal_size, INT32)
PLAN_MSG_SCHEMA_ADD_ARR(plan_ma_msg_pot_agent_t, coef, coef_size, INT32)
PLAN_MSG_SCHEMA_END(schema_pot_agent, plan_ma_msg_pot_agent_t, header)
#define M_pot_agent_pub_op  0x01u
#define M_pot_agent_priv_op 0x02u
#define M_pot_agent_maxpot  0x04u
#define M_pot_agent_op_cost 0x08u
#define M_pot_agent_goal    0x10u
#define M_pot_agent_coef    0x20u

PLAN_MSG_SCHEMA_BEGIN(schema_pot)
PLAN_MSG_SCHEMA_ADD_ARR(plan_ma_msg_pot_t, fact_range, fact_range_size, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_pot_t, lp_private_var_size, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_pot_t, fact_range_lcm, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_pot_t, lp_var_size, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_pot_t, lp_var_offset, INT32)
PLAN_MSG_SCHEMA_ADD_ARR(plan_ma_msg_pot_t, pot, pot_size, INT64)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_pot_t, init_heur, INT64)
PLAN_MSG_SCHEMA_ADD_MSG(plan_ma_msg_pot_t, agent, &schema_pot_agent)
PLAN_MSG_SCHEMA_END(schema_pot, plan_ma_msg_pot_t, header)
#define M_pot_fact_range          0x01u
#define M_pot_lp_private_var_size 0x02u
#define M_pot_fact_range_lcm      0x04u
#define M_pot_lp_var_size         0x08u
#define M_pot_lp_var_offset       0x10u
#define M_pot_pot                 0x20u
#define M_pot_init_heur           0x40u
#define M_pot_agent               0x80u

PLAN_MSG_SCHEMA_BEGIN(schema_op)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_op_t, op_id, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_op_t, cost, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_op_t, owner, INT32)
PLAN_MSG_SCHEMA_ADD(plan_ma_msg_op_t, value, INT32)
PLAN_MSG_SCHEMA_ADD_ARR(plan_ma_msg_op_t, name, name_size, INT8)
PLAN_MSG_SCHEMA_END(schema_op, plan_ma_msg_op_t, header)
#define M_op_id 0x01u
#define M_cost  0x02u
#define M_owner 0x04u
#define M_value 0x08u
#define M_name  0x10u


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
PLAN_MSG_SCHEMA_ADD_MSG_ARR(plan_ma_msg_t, op, op_size, &schema_op)
PLAN_MSG_SCHEMA_ADD_MSG(plan_ma_msg_t, pot, &schema_pot)

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
#define M_snapshot_token       0x000200u
#define M_snapshot_type        0x000400u
#define M_snapshot_ack         0x000800u
#define M_goal_op_id           0x001000u
#define M_min_cut_cost         0x002000u
#define M_heur_token           0x004000u
#define M_heur_requested_agent 0x008000u
#define M_heur_cost            0x010000u
#define M_dtg_req              0x020000u
#define M_search_res           0x040000u

#define M_op                   0x080000u
#define M_pot                  0x100000u


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
        (msg)->member = BOR_ALLOC_ARR(typeof(*((msg)->member)), (src_size)); \
        (msg)->member##_size = (src_size); \
    } while (0)

#define MEMCPY_ARR(msg, member, src, src_size) \
    do { \
        (msg)->header |= M_##member; \
        PREP_ARR(msg, member, src, src_size); \
        memcpy((msg)->member, (src), (src_size) * sizeof(*((msg)->member))); \
    } while (0)

#define CPY_ARR(msg, member, src, src_size) \
    do { \
        (msg)->header |= M_##member; \
        PREP_ARR(msg, member, src, src_size); \
        int i; \
        for (i = 0; i < src_size; ++i) \
            (msg)->member[i] = src[i]; \
    } while (0)

#define CLONE_ARR(dst, src, size, type) \
        do { \
            (dst) = BOR_ALLOC_ARR(type, (size)); \
            memcpy((dst), (src), sizeof(type) * (size)); \
        } while (0)

#define _GETTER(Base, msg_type, Name, member, type) \
    type plan##Base##Name(const msg_type *msg) \
    { \
        return msg->member; \
    }

#define _SETTER(Base, msg_type, Name, member, type) \
    void plan##Base##Set##Name(msg_type *msg, type val) \
    { \
        SET_VAL(msg, member, val); \
    }

#define GETTER(Name, member, type) \
    _GETTER(MAMsg, plan_ma_msg_t, Name, member, type)
#define SETTER(Name, member, type) \
    _SETTER(MAMsg, plan_ma_msg_t, Name, member, type)
#define GETTER_SETTER(Name, member, type) \
    GETTER(Name, member, type) \
    SETTER(Name, member, type)

#define GETTER_OP(Name, member, type) \
    _GETTER(MAMsgOp, plan_ma_msg_op_t, Name, member, type)
#define SETTER_OP(Name, member, type) \
    _SETTER(MAMsgOp, plan_ma_msg_op_t, Name, member, type)
#define GETTER_SETTER_OP(Name, member, type) \
    GETTER_OP(Name, member, type) \
    SETTER_OP(Name, member, type)

#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

static uint64_t pack754(long double f, unsigned bits, unsigned expbits);
static long double unpack754(uint64_t i, unsigned bits, unsigned expbits);
static void planMAMsgPotFree(plan_ma_msg_pot_t *pot);
static void planMAMsgPotClone(plan_ma_msg_pot_t *dst,
                              const plan_ma_msg_pot_t *src);
static void planMAMsgPotAgentFree(plan_ma_msg_pot_agent_t *mpa);
static void planMAMsgPotAgentClone(plan_ma_msg_pot_agent_t *dst,
                                   const plan_ma_msg_pot_agent_t *src);


static int snapshot_token_counter = 0;

static void planMAMsgOpFree(plan_ma_msg_op_t *op)
{
    if (op->name != NULL)
        BOR_FREE(op->name);
}

static void planMAMsgOpCopy(plan_ma_msg_op_t *dst,
                            const plan_ma_msg_op_t *src)
{
    *dst = *src;
    if (src->name != NULL){
        dst->name = BOR_ALLOC_ARR(int8_t, src->name_size);
        memcpy(dst->name, src->name, src->name_size);
    }
}

static void planMAMsgDTGReqFree(plan_ma_msg_dtg_req_t *dr)
{
    if (dr->reachable != NULL)
        BOR_FREE(dr->reachable);
}

static void planMAMsgDTGReqCopy(plan_ma_msg_dtg_req_t *dst,
                                const plan_ma_msg_dtg_req_t *src)
{
    *dst = *src;
    if (src->reachable != NULL){
        dst->reachable = BOR_ALLOC_ARR(int32_t, src->reachable_size);
        memcpy(dst->reachable, src->reachable,
               src->reachable_size * sizeof(int32_t));
    }
}

void planMAMsgInit(plan_ma_msg_t *msg, int type, int subtype, int agent_id)
{
    int32_t stype;

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
}


void planMAMsgFree(plan_ma_msg_t *msg)
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
    planMAMsgPotFree(&msg->pot);
}

plan_ma_msg_t *planMAMsgNew(int type, int subtype, int agent_id)
{
    plan_ma_msg_t *msg;
    msg = BOR_ALLOC(plan_ma_msg_t);
    planMAMsgInit(msg, type, subtype, agent_id);
    return msg;
}

void planMAMsgDel(plan_ma_msg_t *msg)
{
    planMAMsgFree(msg);
    BOR_FREE(msg);
}

plan_ma_msg_t *planMAMsgClone(const plan_ma_msg_t *msg_in)
{
    plan_ma_msg_t *msg;
    int i;

    msg = BOR_ALLOC(plan_ma_msg_t);
    *msg = *msg_in;

    if (msg_in->state_buf != NULL){
        msg->state_buf = NULL;
        MEMCPY_ARR(msg, state_buf, msg_in->state_buf, msg_in->state_buf_size);
    }

    if (msg_in->state_private_id != NULL){
        msg->state_private_id = NULL;
        MEMCPY_ARR(msg, state_private_id, msg_in->state_private_id,
                   msg_in->state_private_id_size);
    }

    if (msg_in->op != NULL){
        msg->op = BOR_ALLOC_ARR(plan_ma_msg_op_t, msg_in->op_size);
        for (i = 0; i < msg->op_size; ++i)
            planMAMsgOpCopy(msg->op + i, msg_in->op + i);
    }

    if (msg_in->heur_requested_agent != NULL){
        msg->heur_requested_agent = NULL;
        MEMCPY_ARR(msg, heur_requested_agent, msg_in->heur_requested_agent,
                   msg_in->heur_requested_agent_size);
    }

    planMAMsgDTGReqCopy(&msg->dtg_req, &msg_in->dtg_req);
    planMAMsgPotClone(&msg->pot, &msg_in->pot);

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

GETTER(Agent, agent_id, int)
GETTER_SETTER(TerminateAgent, terminate_agent_id, int)

void planMAMsgSetStateBuf(plan_ma_msg_t *msg, const void *buf, size_t bufsize)
{
    MEMCPY_ARR(msg, state_buf, buf, bufsize);
}
GETTER(StateBuf, state_buf, const void *)
GETTER(StateBufSize, state_buf_size, int)

void planMAMsgSetStatePrivateIds(plan_ma_msg_t *msg, const int *ids, int size)
{
    CPY_ARR(msg, state_private_id, ids, size);
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

GETTER_SETTER(StateId, state_id, plan_state_id_t)
GETTER_SETTER(StateCost, state_cost, int)
GETTER_SETTER(StateHeur, state_heur, int)
GETTER_SETTER(InitAgent, initiator_agent_id, int)
GETTER_SETTER(SnapshotType, snapshot_type, int)
GETTER(SnapshotToken, snapshot_token, long)
GETTER_SETTER(SnapshotAck, snapshot_ack, int)
GETTER_SETTER(GoalOpId, goal_op_id, int)
GETTER_SETTER(MinCutCost, min_cut_cost, plan_cost_t)
GETTER_SETTER(HeurToken, heur_token, int)

void planMAMsgAddHeurRequestedAgent(plan_ma_msg_t *msg, int agent_id)
{
    ADD_VAL(msg, heur_requested_agent, agent_id);
}

GETTER(HeurRequestedAgentSize, heur_requested_agent_size, int)

int planMAMsgHeurRequestedAgent(const plan_ma_msg_t *msg, int i)
{
    return msg->heur_requested_agent[i];
}

GETTER_SETTER(HeurCost, heur_cost, int)
GETTER_SETTER(SearchRes, search_res, int)

static void updateStrArr(const char *names, int names_size,
                         char ***dst, int *dst_size)
{
    int num_names, i, size;

    // Find the number of strings in names
    for (i = 0, num_names = 0; i < names_size; ++i){
        if (names[i] == 0x0)
            ++num_names;
    }

    if (num_names == 0)
        return;

    // Create output array and assign strings
    size = *dst_size + num_names;
    *dst = BOR_REALLOC_ARR(*dst, char *, size);
    (*dst)[(*dst_size)++] = (char *)names;
    for (i = 1; i < names_size; ++i){
        if (names[i] == 0x0 && (*dst_size) < size)
            (*dst)[(*dst_size)++] = (char *)(names + i + 1);
    }
}




GETTER(OpSize, op_size, int)
const plan_ma_msg_op_t *planMAMsgOp(const plan_ma_msg_t *msg, int idx)
{
    return msg->op + idx;
}

plan_ma_msg_op_t *planMAMsgAddOp(plan_ma_msg_t *msg)
{
    plan_ma_msg_op_t *op;

    ++msg->op_size;
    msg->op = BOR_REALLOC_ARR(msg->op, plan_ma_msg_op_t, msg->op_size);
    op = msg->op + msg->op_size - 1;
    bzero(op, sizeof(*op));
    msg->header |= M_op;
    return op;
}

GETTER_SETTER_OP(OpId, op_id, int)
GETTER_SETTER_OP(Cost, cost, plan_cost_t)
GETTER_SETTER_OP(Owner, owner, int)
GETTER_SETTER_OP(Value, value, plan_cost_t)

const char *planMAMsgOpName(const plan_ma_msg_op_t *op)
{
    return (const char *)op->name;
}

void planMAMsgOpSetName(plan_ma_msg_op_t *op, const char *name)
{
    if (op->name != NULL)
        BOR_FREE(op->name);
    op->name_size = strlen(name) + 1;
    op->name = (int8_t *)BOR_STRDUP(name);
    op->header |= M_name;
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





void planMAMsgTracePathAppendPath(plan_ma_msg_t *msg,
                                  const plan_path_t *path)
{
    bor_list_t *item;
    const plan_path_op_t *p;
    plan_ma_msg_op_t *op;

    for (item = path->prev; item != path; item = item->prev){
        p = BOR_LIST_ENTRY(item, plan_path_op_t, path);

        op = planMAMsgAddOp(msg);
        planMAMsgOpSetOpId(op, p->global_id);
        planMAMsgOpSetCost(op, p->cost);
        planMAMsgOpSetOwner(op, p->owner);
        if (p->name)
            planMAMsgOpSetName(op, p->name);
    }
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




plan_ma_msg_t *planMAMsgSnapshotNewMark(const plan_ma_msg_t *snapshot_init,
                                        int agent_id)
{
    plan_ma_msg_t *msg = planMAMsgNew(PLAN_MA_MSG_SNAPSHOT,
                                      PLAN_MA_MSG_SNAPSHOT_MARK, agent_id);
    SET_VAL(msg, snapshot_token, planMAMsgSnapshotToken(snapshot_init));
    planMAMsgSetSnapshotType(msg, planMAMsgSnapshotType(snapshot_init));
    return msg;
}

plan_ma_msg_t *planMAMsgSnapshotNewResponse(const plan_ma_msg_t *sshot_init,
                                            int agent_id)
{
    plan_ma_msg_t *msg = planMAMsgNew(PLAN_MA_MSG_SNAPSHOT,
                                      PLAN_MA_MSG_SNAPSHOT_RESPONSE, agent_id);
    SET_VAL(msg, snapshot_token, planMAMsgSnapshotToken(sshot_init));
    planMAMsgSetSnapshotType(msg, planMAMsgSnapshotType(sshot_init));
    return msg;
}



void *planMAMsgPacked(const plan_ma_msg_t *msg, int *size)
{
    return planMsgEncode(msg, &schema_msg, size);
}

int planMAMsgPackToBuf(const plan_ma_msg_t *msg, void *buf, int *size)
{
    return planMsgEncode2(msg, &schema_msg, buf, size);
}

plan_ma_msg_t *planMAMsgUnpacked(void *buf, size_t size)
{
    plan_ma_msg_t *msg;

    msg = BOR_ALLOC(plan_ma_msg_t);
    planMsgDecode(msg, &schema_msg, buf);
    return msg;
}



#ifdef PLAN_LP
static void potAgentSetSubmatrix(plan_ma_msg_pot_submatrix_t *ms,
                                 const plan_pot_submatrix_t *s)
{
    int i, size;

    ms->header |= M_pot_submatrix_cols;
    ms->cols = s->cols;
    ms->header |= M_pot_submatrix_rows;
    ms->rows = s->rows;

    if (s->cols * s->rows > 0){
        ms->header |= M_pot_submatrix_coef;

        size = s->cols * s->rows;
        ms->coef_size = size;
        ms->coef = BOR_ALLOC_ARR(int32_t, size);
        for (i = 0; i < size; ++i)
            ms->coef[i] = s->coef[i];
    }
}

static void potAgentGetSubmatrix(const plan_ma_msg_pot_submatrix_t *ms,
                                 plan_pot_submatrix_t *s)
{
    int i, size;

    s->cols = ms->cols;
    s->rows = ms->rows;

    if (ms->cols * s->rows > 0){
        size = ms->cols * ms->rows;
        s->coef = BOR_ALLOC_ARR(int, size);
        for (i = 0; i < size; ++i)
            s->coef[i] = ms->coef[i];
    }
}
#endif /* PLAN_LP */

static void planMAMsgPotFree(plan_ma_msg_pot_t *pot)
{
    if (pot->fact_range != NULL)
        BOR_FREE(pot->fact_range);
    if (pot->pot != NULL)
        BOR_FREE(pot->pot);
    planMAMsgPotAgentFree(&pot->agent);
}

static void planMAMsgPotClone(plan_ma_msg_pot_t *dst,
                              const plan_ma_msg_pot_t *src)
{
    *dst = *src;

    if (dst->fact_range != NULL)
        CLONE_ARR(dst->fact_range, src->fact_range, src->fact_range_size, int32_t);
    if (dst->pot != NULL)
        CLONE_ARR(dst->pot, src->pot, src->pot_size, int64_t);
    planMAMsgPotAgentClone(&dst->agent, &src->agent);
}

void planMAMsgAddPotFactRange(plan_ma_msg_t *msg, int range)
{
    plan_ma_msg_pot_t *pot;
    int i;

    msg->header |= M_pot;

    pot = &msg->pot;
    pot->header |= M_pot_fact_range;

    for (i = 0; i < pot->fact_range_size; ++i){
        if (pot->fact_range[i] == range)
            return;
    }

    ++pot->fact_range_size;
    pot->fact_range = BOR_REALLOC_ARR(pot->fact_range, int32_t,
                                      pot->fact_range_size);
    pot->fact_range[pot->fact_range_size - 1] = range;
}

int planMAMsgPotFactRangeSize(const plan_ma_msg_t *msg)
{
    const plan_ma_msg_pot_t *pot = &msg->pot;
    return pot->fact_range_size;
}

void planMAMsgPotFactRange(const plan_ma_msg_t *msg, int *fact_range)
{
    const plan_ma_msg_pot_t *pot = &msg->pot;
    int i;

    for (i = 0; i < pot->fact_range_size; ++i)
        fact_range[i] = pot->fact_range[i];
}

void planMAMsgSetPotLPPrivateVarSize(plan_ma_msg_t *msg, int var_size)
{
    plan_ma_msg_pot_t *pot = &msg->pot;

    msg->header |= M_pot;
    pot->header |= M_pot_lp_private_var_size;
    pot->lp_private_var_size = var_size;
}

int planMAMsgPotLPPrivateVarSize(const plan_ma_msg_t *msg)
{
    const plan_ma_msg_pot_t *pot = &msg->pot;
    return pot->lp_private_var_size;
}

void planMAMsgSetPotFactRangeLCM(plan_ma_msg_t *msg, int val)
{
    plan_ma_msg_pot_t *pot = &msg->pot;

    msg->header |= M_pot;
    pot->header |= M_pot_fact_range_lcm;
    pot->fact_range_lcm = val;
}

int planMAMsgPotFactRangeLCM(const plan_ma_msg_t *msg)
{
    const plan_ma_msg_pot_t *pot = &msg->pot;
    return pot->fact_range_lcm;
}

void planMAMsgSetPotLPVarSize(plan_ma_msg_t *msg, int val)
{
    plan_ma_msg_pot_t *pot = &msg->pot;

    msg->header |= M_pot;
    pot->header |= M_pot_lp_var_size;
    pot->lp_var_size = val;
}

int planMAMsgPotLPVarSize(const plan_ma_msg_t *msg)
{
    const plan_ma_msg_pot_t *pot = &msg->pot;
    return pot->lp_var_size;
}

void planMAMsgSetPotLPVarOffset(plan_ma_msg_t *msg, int val)
{
    plan_ma_msg_pot_t *pot = &msg->pot;

    msg->header |= M_pot;
    pot->header |= M_pot_lp_var_offset;
    pot->lp_var_offset = val;
}

int planMAMsgPotLPVarOffset(const plan_ma_msg_t *msg)
{
    const plan_ma_msg_pot_t *pot = &msg->pot;
    return pot->lp_var_offset;
}

void planMAMsgSetPotPot(plan_ma_msg_t *msg, const double *src, int pot_size)
{
    plan_ma_msg_pot_t *pot = &msg->pot;
    int i;

    msg->header |= M_pot;
    pot->header |= M_pot_pot;
    pot->pot = BOR_ALLOC_ARR(int64_t, pot_size);
    pot->pot_size = pot_size;
    for (i = 0; i < pot_size; ++i)
        pot->pot[i] = pack754_64(src[i]);
}

int planMAMsgPotPotSize(const plan_ma_msg_t *msg)
{
    const plan_ma_msg_pot_t *pot = &msg->pot;
    return pot->pot_size;
}

void planMAMsgPotPot(const plan_ma_msg_t *msg, double *dst)
{
    const plan_ma_msg_pot_t *pot = &msg->pot;
    int i;

    for (i = 0; i < pot->pot_size; ++i)
        dst[i] = unpack754_64(pot->pot[i]);
}

void planMAMsgSetPotInitHeur(plan_ma_msg_t *msg, double val)
{
    plan_ma_msg_pot_t *pot = &msg->pot;

    msg->header |= M_pot;
    pot->header |= M_pot_init_heur;
    pot->init_heur = pack754_64(val);
}

double planMAMsgPotInitHeur(const plan_ma_msg_t *msg)
{
    const plan_ma_msg_pot_t *pot = &msg->pot;
    return unpack754_64(pot->init_heur);
}


static void planMAMsgPotAgentFree(plan_ma_msg_pot_agent_t *mpa)
{
    if (mpa->pub_op.coef)
        BOR_FREE(mpa->pub_op.coef);
    if (mpa->priv_op.coef)
        BOR_FREE(mpa->priv_op.coef);
    if (mpa->maxpot.coef)
        BOR_FREE(mpa->maxpot.coef);

    if (mpa->op_cost)
        BOR_FREE(mpa->op_cost);
    if (mpa->goal)
        BOR_FREE(mpa->goal);
    if (mpa->coef)
        BOR_FREE(mpa->coef);
}

static void potSubmatrixClone(plan_ma_msg_pot_submatrix_t *sm)
{
    const int *src;

    if (sm->coef != NULL){
        src = sm->coef;
        sm->coef = BOR_ALLOC_ARR(int32_t, sm->coef_size);
        memcpy(sm->coef, src, sizeof(int32_t) * sm->coef_size);
    }
}

static void planMAMsgPotAgentClone(plan_ma_msg_pot_agent_t *dst,
                                   const plan_ma_msg_pot_agent_t *src)
{
    *dst = *src;

    potSubmatrixClone(&dst->pub_op);
    potSubmatrixClone(&dst->priv_op);
    potSubmatrixClone(&dst->maxpot);
    if (dst->op_cost != NULL)
        CLONE_ARR(dst->op_cost, src->op_cost, src->op_cost_size, int32_t);
    if (dst->goal != NULL)
        CLONE_ARR(dst->goal, src->goal, src->goal_size, int32_t);
    if (dst->coef != NULL)
        CLONE_ARR(dst->coef, src->coef, src->coef_size, int32_t);
}

#ifdef PLAN_LP
void planMAMsgSetPotAgent(plan_ma_msg_t *msg, const plan_pot_agent_t *pa)
{
    plan_ma_msg_pot_agent_t *mpa;
    int i;

    msg->header |= M_pot;
    msg->pot.header |= M_pot_agent;
    mpa = &msg->pot.agent;

    mpa->header |= M_pot_agent_pub_op;
    potAgentSetSubmatrix(&mpa->pub_op, &pa->pub_op);
    mpa->header |= M_pot_agent_priv_op;
    potAgentSetSubmatrix(&mpa->priv_op, &pa->priv_op);
    mpa->header |= M_pot_agent_maxpot;
    potAgentSetSubmatrix(&mpa->maxpot, &pa->maxpot);

    mpa->header |= M_pot_agent_op_cost;
    mpa->op_cost_size = pa->pub_op.rows;
    mpa->op_cost = BOR_ALLOC_ARR(int32_t, mpa->op_cost_size);
    for (i = 0; i < mpa->op_cost_size; ++i)
        mpa->op_cost[i] = pa->op_cost[i];

    if (pa->priv_op.cols > 0){
        mpa->header |= M_pot_agent_goal;
        mpa->goal_size = pa->priv_op.cols;
        mpa->goal = BOR_ALLOC_ARR(int32_t, mpa->goal_size);
        for (i = 0; i < mpa->goal_size; ++i)
            mpa->goal[i] = pa->goal[i];

        mpa->header |= M_pot_agent_coef;
        mpa->coef_size = pa->priv_op.cols;
        mpa->coef = BOR_ALLOC_ARR(int32_t, mpa->coef_size);
        for (i = 0; i < mpa->coef_size; ++i)
            mpa->coef[i] = pa->coef[i];
    }
}

void planMAMsgPotAgent(const plan_ma_msg_t *msg, plan_pot_agent_t *pa)
{
    const plan_ma_msg_pot_agent_t *mpa = &msg->pot.agent;
    int i;

    bzero(pa, sizeof(*pa));
    potAgentGetSubmatrix(&mpa->pub_op, &pa->pub_op);
    potAgentGetSubmatrix(&mpa->priv_op, &pa->priv_op);
    potAgentGetSubmatrix(&mpa->maxpot, &pa->maxpot);

    pa->op_cost = BOR_ALLOC_ARR(int, mpa->pub_op.rows);
    for (i = 0; i < mpa->pub_op.rows; ++i)
        pa->op_cost[i] = mpa->op_cost[i];

    if (mpa->priv_op.cols > 0){
        pa->goal = BOR_ALLOC_ARR(int, mpa->priv_op.cols);
        pa->coef = BOR_ALLOC_ARR(int, mpa->priv_op.cols);
        for (i = 0; i < mpa->priv_op.cols; ++i){
            pa->goal[i] = mpa->goal[i];
            pa->coef[i] = mpa->coef[i];
        }
    }
}
#endif /* PLAN_LP */

static uint64_t pack754(long double f, unsigned bits, unsigned expbits)
{
    long double fnorm;
    int shift;
    long long sign, exp, significand;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (f == 0.0) return 0; // get this special case out of the way

    // check sign and begin normalization
    if (f < 0) { sign = 1; fnorm = -f; }
    else { sign = 0; fnorm = f; }

    // get the normalized form of f and track the exponent
    shift = 0;
    while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
    while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
    fnorm = fnorm - 1.0;

    // calculate the binary form (non-float) of the significand data
    significand = fnorm * ((1LL<<significandbits) + 0.5f);

    // get the biased exponent
    exp = shift + ((1<<(expbits-1)) - 1); // shift + bias

    // return the final answer
    return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}

static long double unpack754(uint64_t i, unsigned bits, unsigned expbits)
{
    long double result;
    long long shift;
    unsigned bias;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (i == 0) return 0.0;

    // pull the significand
    result = (i&((1LL<<significandbits)-1)); // mask
    result /= (1LL<<significandbits); // convert back to float
    result += 1.0f; // add the one back on

    // deal with the exponent
    bias = (1<<(expbits-1)) - 1;
    shift = ((i>>significandbits)&((1LL<<expbits)-1)) - bias;
    while(shift > 0) { result *= 2.0; shift--; }
    while(shift < 0) { result /= 2.0; shift++; }

    // sign it
    result *= (i>>(bits-1))&1? -1.0: 1.0;

    return result;
}
