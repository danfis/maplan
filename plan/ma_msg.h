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
#define PLAN_MA_MSG_HEUR_FF_REQUEST   0x01
#define PLAN_MA_MSG_HEUR_FF_RESPONSE  0x10
#define PLAN_MA_MSG_HEUR_MAX_REQUEST  0x02
#define PLAN_MA_MSG_HEUR_MAX_RESPONSE 0x20
#define PLAN_MA_MSG_HEUR_LM_CUT_INIT_REQUEST 0x03
#define PLAN_MA_MSG_HEUR_LM_CUT_MAX_REQUEST 0x04
#define PLAN_MA_MSG_HEUR_LM_CUT_MAX_RESPONSE 0x30
#define PLAN_MA_MSG_HEUR_LM_CUT_SUPP_REQUEST 0x05
#define PLAN_MA_MSG_HEUR_LM_CUT_FIND_CUT_REQUEST 0x06
#define PLAN_MA_MSG_HEUR_LM_CUT_FIND_CUT_RESPONSE 0x60

/**
 * Returns values for planMAMsgHeurType().
 */
#define PLAN_MA_MSG_HEUR_NONE    0
#define PLAN_MA_MSG_HEUR_UPDATE  1
#define PLAN_MA_MSG_HEUR_REQUEST 2

struct _plan_ma_msg_t {
    char _; /*!< This is just placeholder */
};
typedef struct _plan_ma_msg_t plan_ma_msg_t;

/**
 * Frees global memory allocated by google protobuffers.
 * This is just wrapper for
 *      google::protobuf::ShutdownProtobufLibrary().
 */
void planShutdownProtobuf(void);

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



/*** TERMINATE: ***/

/**
 * Set terminate-agent-id to TERMINATE message.
 */
void planMAMsgTerminateSetAgent(plan_ma_msg_t *msg, int agent_id);

/**
 * Returns ID of agent that started termination.
 */
int planMAMsgTerminateAgent(const plan_ma_msg_t *msg);



/*** PUBLIC STATE: ***/

/**
 * Sets public-state message data.
 */
void planMAMsgPublicStateSetState(plan_ma_msg_t *msg,
                                  const void *statebuf,
                                  size_t statebuf_size,
                                  int state_id, int cost, int heur);

/**
 * Returns pointer to the state buffer.
 */
const void *planMAMsgPublicStateStateBuf(const plan_ma_msg_t *msg);

/**
 * Returns state-id of the state in the source agent's pool.
 */
int planMAMsgPublicStateStateId(const plan_ma_msg_t *msg);

/**
 * Returns cost of the path from initial state to this state as computed by
 * remote agent.
 */
int planMAMsgPublicStateCost(const plan_ma_msg_t *msg);

/**
 * Returns heuristic value computed by the remote agent.
 */
int planMAMsgPublicStateHeur(const plan_ma_msg_t *msg);



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
 * Sets request for FF heuristic.
 */
void planMAMsgHeurFFSetRequest(plan_ma_msg_t *msg,
                               const int *init_state, int init_state_size,
                               int goal_op_id);

/**
 * Sets response for FF heuristic.
 */
void planMAMsgHeurFFSetResponse(plan_ma_msg_t *msg, int goal_op_id);

/**
 * Adds operator to message related to FF heuristic.
 */
#define planMAMsgHeurFFAddOp(msg, op_id, cost, owner) \
    planMAMsgAddOp((msg), (op_id), (cost), (owner), PLAN_COST_INVALID)

/**
 * Writes to state argument state stored in the message.
 */
#define planMAMsgHeurFFState planMAMsgStateFull

/**
 * Returns goal op ID stored in the message.
 */
int planMAMsgHeurFFOpId(const plan_ma_msg_t *msg);

/**
 * Returns number of operators stored in message.
 */
#define planMAMsgHeurFFOpSize planMAMsgOpSize

/**
 * Returns i'th operator's ID and its cost and owner.
 */
#define planMAMsgHeurFFOp(msg, i, cost, owner) \
    planMAMsgOp((msg), (i), (cost), (owner), NULL)



/**
 * Sets request for Max heuristic.
 */
#define planMAMsgHeurMaxSetRequest planMAMsgSetStateFull

/**
 * Adds operator to message related to Max heuristic.
 */
#define planMAMsgHeurMaxAddOp(msg, op_id, value) \
    planMAMsgAddOp((msg), (op_id), PLAN_COST_INVALID, -1, value)

/**
 * Loads state from the message.
 */
#define planMAMsgHeurMaxState planMAMsgStateFull
#define planMAMsgHeurMaxStateVal planMAMsgStateFullVal

/**
 * Returns number of operators stored in message.
 */
#define planMAMsgHeurMaxOpSize planMAMsgOpSize

/**
 * Returns i'th operator's ID and its value
 */
#define planMAMsgHeurMaxOp(msg, i, value) \
    planMAMsgOp((msg), (i), NULL, NULL, (value))



/**
 * Sets init-request for max phase of lm-cut heuristic.
 */
#define planMAMsgHeurLMCutSetInitRequest planMAMsgSetStateFull2

/**
 * Adds operator to the max request/repsonse.
 */
#define planMAMsgHeurLMCutMaxAddOp(msg, op_id, value) \
    planMAMsgAddOp((msg), (op_id), PLAN_COST_INVALID, -1, value)

/**
 * Returns number of operators stored in message.
 */
#define planMAMsgHeurLMCutMaxOpSize planMAMsgOpSize

/**
 * Returns i'th operator's ID and its value
 */
#define planMAMsgHeurLMCutMaxOp(msg, i, value) \
    planMAMsgOp((msg), (i), NULL, NULL, (value))



/**
 * Adds operator to the HEUR_LM_CUT_SUPP_* message.
 */
#define planMAMsgHeurLMCutSuppAddOp(msg, op_id) \
    planMAMsgAddOp((msg), (op_id), PLAN_COST_INVALID, -1, PLAN_COST_INVALID)

/**
 * Returns number of operators stored in the message.
 */
#define planMAMsgHeurLMCutSuppOpSize planMAMsgOpSize

/**
 * Returns ID of the i'th operator.
 */
#define planMAMsgHeurLMCutSuppOp(msg, i) \
    planMAMsgOp((msg), (i), NULL, NULL, NULL)


/**
 * Set full state in the message.
 */
#define planMAMsgHeurLMCutFindCutSetState(msg, state) \
    planMAMsgSetStateFull2((msg), (state))

/**
 * Adds operator to the *FIND_CUT* message.
 */
#define planMAMsgHeurLMCutFindCutAddOp(msg, op_id) \
    planMAMsgAddOp((msg), (op_id), PLAN_COST_INVALID, -1, PLAN_COST_INVALID)

/**
 * Returns number of operators stored in the message.
 */
#define planMAMsgHeurLMCutFindCutOpSize planMAMsgOpSize

/**
 * Returns ID of the i'th operator.
 */
#define planMAMsgHeurLMCutFindCutOp(msg, i) \
    planMAMsgOp((msg), (i), NULL, NULL, NULL)


/**
 * Sets full state member of the message.
 */
void planMAMsgSetStateFull(plan_ma_msg_t *msg, const int *state, int size);

/**
 * Sets full state member of the message.
 */
void planMAMsgSetStateFull2(plan_ma_msg_t *msg, const plan_state_t *state);

/**
 * Returns true if the full state is set.
 */
int planMAMsgHasStateFull(const plan_ma_msg_t *msg);

/**
 * Loads full state from the message
 */
void planMAMsgStateFull(const plan_ma_msg_t *msg, plan_state_t *state);

/**
 * Returns value of a specified variable in full state.
 */
plan_val_t planMAMsgStateFullVal(const plan_ma_msg_t *msg, plan_var_id_t var);

/**
 * Adds operator to the message.
 */
void planMAMsgAddOp(plan_ma_msg_t *msg, int op_id, plan_cost_t cost,
                    int owner, plan_cost_t value);

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


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_MSG_H__ */
