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

#ifndef __PLAN_MA_MSG_H__
#define __PLAN_MA_MSG_H__

#include <stdlib.h>
#include <boruvka/core.h>

#include <plan/path.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Message types:
 */
#define PLAN_MA_MSG_TERMINATE    0x0
#define PLAN_MA_MSG_TRACE_PATH   0x1
#define PLAN_MA_MSG_PUBLIC_STATE 0x2
#define PLAN_MA_MSG_SNAPSHOT     0x3
#define PLAN_MA_MSG_HEUR         0x4


/**
 * Terminate sub-types:
 */
#define PLAN_MA_MSG_TERMINATE_REQUEST 0x0
#define PLAN_MA_MSG_TERMINATE_FINAL   0x1

/**
 * Snapshot sub-types
 */
#define PLAN_MA_MSG_SNAPSHOT_INIT     0x0
#define PLAN_MA_MSG_SNAPSHOT_MARK     0x1
#define PLAN_MA_MSG_SNAPSHOT_RESPONSE 0x2

/**
 * Snapshot specific types (see planMAMsgSnapshotSetType()).
 */
#define PLAN_MA_MSG_SOLUTION_VERIFICATION 0x0
#define PLAN_MA_MSG_DEAD_END_VERIFICATION 0x1


/**
 * Heur sub-types
 */
#define PLAN_MA_MSG_HEUR_FF_REQUEST   0x0001
#define PLAN_MA_MSG_HEUR_FF_RESPONSE  0x0100
#define PLAN_MA_MSG_HEUR_MAX_REQUEST  0x0002
#define PLAN_MA_MSG_HEUR_MAX_RESPONSE 0x0200

#define PLAN_MA_MSG_HEUR_LM_CUT_INIT_REQUEST       0x0003
#define PLAN_MA_MSG_HEUR_LM_CUT_HMAX_REQUEST       0x0004
#define PLAN_MA_MSG_HEUR_LM_CUT_HMAX_RESPONSE      0x0400
#define PLAN_MA_MSG_HEUR_LM_CUT_HMAX_INC_REQUEST   0x0005
#define PLAN_MA_MSG_HEUR_LM_CUT_SUPP_REQUEST       0x0006
#define PLAN_MA_MSG_HEUR_LM_CUT_FIND_CUT_REQUEST   0x0007
#define PLAN_MA_MSG_HEUR_LM_CUT_FIND_CUT_RESPONSE  0x0700
#define PLAN_MA_MSG_HEUR_LM_CUT_CUT_REQUEST        0x0008
#define PLAN_MA_MSG_HEUR_LM_CUT_CUT_RESPONSE       0x0800
#define PLAN_MA_MSG_HEUR_LM_CUT_GOAL_ZONE_REQUEST  0x0009
#define PLAN_MA_MSG_HEUR_LM_CUT_GOAL_ZONE_RESPONSE 0x0900

#define PLAN_MA_MSG_HEUR_DTG_REQUEST     0x000a
#define PLAN_MA_MSG_HEUR_DTG_RESPONSE    0x0a00
#define PLAN_MA_MSG_HEUR_DTG_REQRESPONSE 0x000b

/**
 * Returns values for planMAMsgHeurType().
 */
#define PLAN_MA_MSG_HEUR_NONE    0
#define PLAN_MA_MSG_HEUR_UPDATE  1
#define PLAN_MA_MSG_HEUR_REQUEST 2

typedef struct _plan_ma_msg_op_t plan_ma_msg_op_t;
typedef struct _plan_ma_msg_dtg_req_t plan_ma_msg_dtg_req_t;
typedef struct _plan_ma_msg_t plan_ma_msg_t;

/**
 * Creates a new message of specified type and subtype.
 */
plan_ma_msg_t *planMAMsgNew(int type, int subtype, int agent_id);

/**
 * Deletes the message.
 */
void planMAMsgDel(plan_ma_msg_t *msg);

/**
 * Creates a complete copy of the message.
 */
plan_ma_msg_t *planMAMsgClone(const plan_ma_msg_t *msg);

/**
 * Returns type (PLAN_MA_MSG_TYPE_*).
 */
int planMAMsgType(const plan_ma_msg_t *msg);

/**
 * Returns a sub-type.
 */
int planMAMsgSubType(const plan_ma_msg_t *msg);

/**
 * Returns the originating agent.
 */
int planMAMsgAgent(const plan_ma_msg_t *msg);

/**
 * Returns heap-allocated packed message and its size.
 */
void *planMAMsgPacked(const plan_ma_msg_t *msg, size_t *size);

/**
 * Returns a new message unpacked from the given buffer.
 */
plan_ma_msg_t *planMAMsgUnpacked(void *buf, size_t size);


/*** STATE ***/
void planMAMsgSetStateBuf(plan_ma_msg_t *msg, const void *buf, size_t bufsize);
void planMAMsgSetStatePrivateIds(plan_ma_msg_t *msg, const int *ids, int size);
void planMAMsgSetStateId(plan_ma_msg_t *msg, plan_state_id_t state_id);
void planMAMsgSetStateCost(plan_ma_msg_t *msg, int cost);
void planMAMsgSetStateHeur(plan_ma_msg_t *msg, int heur);

const void *planMAMsgStateBuf(const plan_ma_msg_t *msg);
int planMAMsgStateBufSize(const plan_ma_msg_t *msg);
int planMAMsgStatePrivateIdsSize(const plan_ma_msg_t *msg);
void planMAMsgStatePrivateIds(const plan_ma_msg_t *msg, int *ids);
int planMAMsgStateId(const plan_ma_msg_t *msg);
int planMAMsgStateCost(const plan_ma_msg_t *msg);
int planMAMsgStateHeur(const plan_ma_msg_t *msg);

void planMAMsgSetGoalOpId(plan_ma_msg_t *msg, int goal_op_id);
int planMAMsgGoalOpId(const plan_ma_msg_t *msg);


/*** TERMINATE: ***/

/**
 * Set terminate-agent-id to TERMINATE message.
 */
void planMAMsgTerminateSetAgent(plan_ma_msg_t *msg, int agent_id);

/**
 * Returns ID of agent that started termination.
 */
int planMAMsgTerminateAgent(const plan_ma_msg_t *msg);

/**
 * Sets a result code of search.
 */
void planMAMsgSetSearchRes(plan_ma_msg_t *msg, int res);

/**
 * Returns a result code.
 */
int planMAMsgSearchRes(const plan_ma_msg_t *msg);


/*** TRACE PATH: ***/

/**
 * Sets next state-id to trace-path message.
 */
void planMAMsgTracePathSetStateId(plan_ma_msg_t *msg, int state_id);

/**
 * Adds next operator to the path.
 */
void planMAMsgTracePathAddPath(plan_ma_msg_t *msg, const plan_path_t *path);

/**
 * Returns ID of the last state the path was tracked to.
 */
int planMAMsgTracePathStateId(const plan_ma_msg_t *msg);

/**
 * Extracts path from the message to path object.
 */
void planMAMsgTracePathExtractPath(const plan_ma_msg_t *msg,
                                   plan_path_t *path);


/**
 * Returns ID of the initiator.
 */
int planMAMsgTracePathInitAgent(const plan_ma_msg_t *msg);



/*** SNAPSHOT: ***/

/**
 * Sets snapshot type, see above for list of types.
 */
void planMAMsgSnapshotSetType(plan_ma_msg_t *msg, int type);

/**
 * Returns snapshot type
 */
int planMAMsgSnapshotType(const plan_ma_msg_t *msg);

/**
 * Returns assigned snapshot token.
 */
long planMAMsgSnapshotToken(const plan_ma_msg_t *msg);

/**
 * Creates a new SNAPSHOT_MARK message from SNAPSHOT_INIT message.
 */
plan_ma_msg_t *planMAMsgSnapshotNewMark(const plan_ma_msg_t *snapshot_init,
                                        int agent_id);

/**
 * Creates a new SNAPSHOT_RESPONSE message from a SNAPSHOT_INIT message.
 */
plan_ma_msg_t *planMAMsgSnapshotNewResponse(const plan_ma_msg_t *sshot_init,
                                            int agent_id);

/**
 * Sets ack flag to the specified value.
 */
void planMAMsgSnapshotSetAck(plan_ma_msg_t *msg, int ack);

/**
 * Returns ack flag.
 */
int planMAMsgSnapshotAck(const plan_ma_msg_t *msg);



/*** HEUR: ***/

/**
 * Returns whether the heur message is for planHeurMAUpdate() or for
 * planHeurMARequest().
 */
int planMAMsgHeurType(const plan_ma_msg_t *msg);



/**
 * Adds operator to the message.
 */
void planMAMsgAddOp(plan_ma_msg_t *msg, int op_id, plan_cost_t cost,
                    int owner, plan_cost_t value);

/**
 * Adds only op_id part of operator.
 */
#define planMAMsgAddOpId(msg, op_id) \
    planMAMsgAddOp((msg), (op_id), PLAN_COST_INVALID, -1, PLAN_COST_INVALID)

/**
 * Adds op_id and value part of operator.
 */
#define planMAMsgAddOpIdValue(msg, op_id, value) \
    planMAMsgAddOp((msg), (op_id), PLAN_COST_INVALID, -1, (value))

/**
 * Adds op_id, cost and owner of operator.
 */
#define planMAMsgAddOpIdCostOwner(msg, op_id, cost, owner) \
    planMAMsgAddOp((msg), (op_id), (cost), (owner), PLAN_COST_INVALID)

/**
 * Returns number of operators stored in message.
 */
int planMAMsgOpSize(const plan_ma_msg_t *msg);

/**
 * Returns i'th operator's ID and its corresponding values if argument set
 * to non-NULL.
 */
int planMAMsgOp(const plan_ma_msg_t *msg, int i,
                plan_cost_t *cost, int *owner, plan_cost_t *value);

/**
 * Returns i'th op_id.
 */
#define planMAMsgOpId(msg, i) planMAMsgOp((msg), (i), NULL, NULL, NULL)

/**
 * Returns i'th op_id and value.
 */
#define planMAMsgOpIdValue(msg, i, value) \
    planMAMsgOp((msg), (i), NULL, NULL, (value))

/**
 * Returns i'th operator's ID and its cost and owner.
 */
#define planMAMsgOpIdCostOwner(msg, i, cost, owner) \
    planMAMsgOp((msg), (i), (cost), (owner), NULL)


/**
 * Sets min-cut-cost value.
 */
void planMAMsgSetMinCutCost(plan_ma_msg_t *msg, plan_cost_t cost);

/**
 * Returns min-cut-cost value.
 */
plan_cost_t planMAMsgMinCutCost(const plan_ma_msg_t *msg);


/**
 * Sets token for heuristic.
 */
void planMAMsgSetHeurToken(plan_ma_msg_t *msg, int token);

/**
 * Returns heuristic token.
 */
int planMAMsgHeurToken(const plan_ma_msg_t *msg);

/**
 * Adds agent-id to the list of agents already requested.
 */
void planMAMsgAddHeurRequestedAgent(plan_ma_msg_t *msg, int agent_id);

/**
 * Returns number of IDs stored in list of requested agents.
 */
int planMAMsgHeurRequestedAgentSize(const plan_ma_msg_t *msg);

/**
 * Returns i'th ID from list of requested agents.
 */
int planMAMsgHeurRequestedAgent(const plan_ma_msg_t *msg, int i);

/**
 * Sets cost of heuristic function.
 */
void planMAMsgSetHeurCost(plan_ma_msg_t *msg, int cost);

/**
 * Returns cost of heuristic stored in message.
 */
int planMAMsgHeurCost(const plan_ma_msg_t *msg);

/**
 * Sets request for DTG heuristic.
 */
void planMAMsgSetDTGReq(plan_ma_msg_t *msg, int var, int from, int to);

/**
 * Adds value to list of dtg-reachable values.
 */
void planMAMsgAddDTGReqReachable(plan_ma_msg_t *msg, int val);

/**
 * Returns data set by planMAMsgSetDTGReq()
 */
void planMAMsgDTGReq(const plan_ma_msg_t *msg, int *var, int *from, int *to);

/**
 * Number of dtg-reachable values.
 */
int planMAMsgDTGReqReachableSize(const plan_ma_msg_t *msg);

/**
 * Returns i'th reachable value.
 */
int planMAMsgDTGReqReachable(const plan_ma_msg_t *msg, int i);

/**
 * Copies reachable from src to dst.
 */
void planMAMsgCopyDTGReqReachable(plan_ma_msg_t *dst, const plan_ma_msg_t *src);

/**
 * Sets initiator agent ID
 */
void planMAMsgSetInitAgent(plan_ma_msg_t *msg, int agent_id);

/**
 * Returns initiator agent ID
 */
int planMAMsgInitAgent(const plan_ma_msg_t *msg);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_MSG_H__ */
