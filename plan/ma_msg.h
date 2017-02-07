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
#include <plan/pot.h>

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
#define PLAN_MA_MSG_TERMINATE_REQUEST   0x0
#define PLAN_MA_MSG_TERMINATE_FINAL     0x1
#define PLAN_MA_MSG_TERMINATE_FINAL_ACK 0x2
#define PLAN_MA_MSG_TERMINATE_FINAL_FIN 0x3

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

#define PLAN_MA_MSG_HEUR_POT_REQUEST            0x000c
#define PLAN_MA_MSG_HEUR_POT_RESPONSE           0x0c00
#define PLAN_MA_MSG_HEUR_POT_RESULTS_REQUEST    0x000d
#define PLAN_MA_MSG_HEUR_POT_RESULTS_RESPONSE   0x0d00
#define PLAN_MA_MSG_HEUR_POT_INFO_REQUEST       0x000e
#define PLAN_MA_MSG_HEUR_POT_INFO_RESPONSE      0x0e00
#define PLAN_MA_MSG_HEUR_POT_LP_REQUEST         0x000f
#define PLAN_MA_MSG_HEUR_POT_LP_RESPONSE        0x0f00
#define PLAN_MA_MSG_HEUR_POT_INIT_HEUR_REQUEST  0x0010
#define PLAN_MA_MSG_HEUR_POT_INIT_HEUR_RESPONSE 0x1000

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
 * Initiaze ma-msg structure
 */
void planMAMsgInit(plan_ma_msg_t *msg, int type, int subtype, int agent_id);

/**
 * Free memory allocated withing ma-msg structure.
 */
void planMAMsgFree(plan_ma_msg_t *msg);

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


/*** Getters / Setters: ***/

/**
 * Returns type (PLAN_MA_MSG_TYPE_*).
 */
int planMAMsgType(const plan_ma_msg_t *msg);

/**
 * Returns a sub-type.
 */
int planMAMsgSubType(const plan_ma_msg_t *msg);

/**
 * Returns whether the heur message is for planHeurMAUpdate() or for
 * planHeurMARequest().
 */
int planMAMsgHeurType(const plan_ma_msg_t *msg);

/**
 * Returns the originating agent.
 */
int planMAMsgAgent(const plan_ma_msg_t *msg);


int planMAMsgTerminateAgent(const plan_ma_msg_t *msg);
void planMAMsgSetTerminateAgent(plan_ma_msg_t *msg, int agent_id);

int planMAMsgStateBufSize(const plan_ma_msg_t *msg);
const void *planMAMsgStateBuf(const plan_ma_msg_t *msg);
void planMAMsgSetStateBuf(plan_ma_msg_t *msg, const void *buf, size_t bufsize);

int planMAMsgStatePrivateIdsSize(const plan_ma_msg_t *msg);
void planMAMsgStatePrivateIds(const plan_ma_msg_t *msg, int *ids);
void planMAMsgSetStatePrivateIds(plan_ma_msg_t *msg, const int *ids, int size);

plan_state_id_t planMAMsgStateId(const plan_ma_msg_t *msg);
void planMAMsgSetStateId(plan_ma_msg_t *msg, plan_state_id_t state_id);

int planMAMsgStateCost(const plan_ma_msg_t *msg);
void planMAMsgSetStateCost(plan_ma_msg_t *msg, int cost);

int planMAMsgStateHeur(const plan_ma_msg_t *msg);
void planMAMsgSetStateHeur(plan_ma_msg_t *msg, int heur);

int planMAMsgInitAgent(const plan_ma_msg_t *msg);
void planMAMsgSetInitAgent(plan_ma_msg_t *msg, int agent_id);


/**
 * For snapshot type, see above list.
 */
int planMAMsgSnapshotType(const plan_ma_msg_t *msg);
void planMAMsgSetSnapshotType(plan_ma_msg_t *msg, int type);

long planMAMsgSnapshotToken(const plan_ma_msg_t *msg);
void planMAMsgSetSnapshotAck(plan_ma_msg_t *msg, int ack);
int planMAMsgSnapshotAck(const plan_ma_msg_t *msg);

int planMAMsgGoalOpId(const plan_ma_msg_t *msg);
void planMAMsgSetGoalOpId(plan_ma_msg_t *msg, int goal_op_id);

plan_cost_t planMAMsgMinCutCost(const plan_ma_msg_t *msg);
void planMAMsgSetMinCutCost(plan_ma_msg_t *msg, plan_cost_t cost);

int planMAMsgHeurToken(const plan_ma_msg_t *msg);
void planMAMsgSetHeurToken(plan_ma_msg_t *msg, int token);

int planMAMsgHeurRequestedAgentSize(const plan_ma_msg_t *msg);
int planMAMsgHeurRequestedAgent(const plan_ma_msg_t *msg, int i);
void planMAMsgAddHeurRequestedAgent(plan_ma_msg_t *msg, int agent_id);

int planMAMsgHeurCost(const plan_ma_msg_t *msg);
void planMAMsgSetHeurCost(plan_ma_msg_t *msg, int cost);

int planMAMsgSearchRes(const plan_ma_msg_t *msg);
void planMAMsgSetSearchRes(plan_ma_msg_t *msg, int res);

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



int planMAMsgOpSize(const plan_ma_msg_t *msg);
const plan_ma_msg_op_t *planMAMsgOp(const plan_ma_msg_t *msg, int idx);
plan_ma_msg_op_t *planMAMsgAddOp(plan_ma_msg_t *msg);

int planMAMsgOpOpId(const plan_ma_msg_op_t *op);
void planMAMsgOpSetOpId(plan_ma_msg_op_t *op, int op_id);
plan_cost_t planMAMsgOpCost(const plan_ma_msg_op_t *op);
void planMAMsgOpSetCost(plan_ma_msg_op_t *op, plan_cost_t cost);
int planMAMsgOpOwner(const plan_ma_msg_op_t *op);
void planMAMsgOpSetOwner(plan_ma_msg_op_t *op, int op_id);
plan_cost_t planMAMsgOpValue(const plan_ma_msg_op_t *op);
void planMAMsgOpSetValue(plan_ma_msg_op_t *op, plan_cost_t cost);
const char *planMAMsgOpName(const plan_ma_msg_op_t *op);
void planMAMsgOpSetName(plan_ma_msg_op_t *op, const char *name);





/**
 * Append path operators to the message.
 */
void planMAMsgTracePathAppendPath(plan_ma_msg_t *msg,
                                  const plan_path_t *path);

/**
 * Extracts path from the message to path object.
 */
void planMAMsgTracePathExtractPath(const plan_ma_msg_t *msg,
                                   plan_path_t *path);


/*** SNAPSHOT: ***/


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
 * Returns heap-allocated packed message and its size.
 */
void *planMAMsgPacked(const plan_ma_msg_t *msg, int *size);

/**
 * Pack message to the buffer.
 * Returns 0 on success and size is filled with the size of actually used
 * memory.
 * Returns -1 if there is not enough space and size is thus filled with
 * needed memory size.
 */
int planMAMsgPackToBuf(const plan_ma_msg_t *msg, void *buf, int *size);

/**
 * Returns a new message unpacked from the given buffer.
 */
plan_ma_msg_t *planMAMsgUnpacked(void *buf, size_t size);


void planMAMsgAddPotFactRange(plan_ma_msg_t *msg, int range);
int planMAMsgPotFactRangeSize(const plan_ma_msg_t *msg);
void planMAMsgPotFactRange(const plan_ma_msg_t *msg, int *fact_range);
void planMAMsgSetPotLPPrivateVarSize(plan_ma_msg_t *msg, int var_size);
int planMAMsgPotLPPrivateVarSize(const plan_ma_msg_t *msg);
void planMAMsgSetPotFactRangeLCM(plan_ma_msg_t *msg, int val);
int planMAMsgPotFactRangeLCM(const plan_ma_msg_t *msg);
void planMAMsgSetPotLPVarSize(plan_ma_msg_t *msg, int val);
int planMAMsgPotLPVarSize(const plan_ma_msg_t *msg);
void planMAMsgSetPotLPVarOffset(plan_ma_msg_t *msg, int val);
int planMAMsgPotLPVarOffset(const plan_ma_msg_t *msg);
void planMAMsgSetPotPot(plan_ma_msg_t *msg, const double *pot, int pot_size);
int planMAMsgPotPotSize(const plan_ma_msg_t *msg);
void planMAMsgSetPotInitHeur(plan_ma_msg_t *msg, double val);
double planMAMsgPotInitHeur(const plan_ma_msg_t *msg);
void planMAMsgPotPot(const plan_ma_msg_t *msg, double *pot);

#ifdef PLAN_LP
void planMAMsgSetPotAgent(plan_ma_msg_t *msg, const plan_pot_agent_t *pa);
void planMAMsgPotAgent(const plan_ma_msg_t *msg, plan_pot_agent_t *pa);
#endif /* PLAN_LP */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_MSG_H__ */
