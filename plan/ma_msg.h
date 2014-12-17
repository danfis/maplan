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
#define PLAN_MA_MSG_HEUR_FF_REQUEST 0x0
#define PLAN_MA_MSG_HEUR_FF_RESPONSE 0x1

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
void planMAMsgHeurFFAddOp(plan_ma_msg_t *msg, int op_id,
                          plan_cost_t cost, int owner);

/**
 * Writes to state argument state stored in the message.
 */
void planMAMsgHeurFFState(const plan_ma_msg_t *msg, plan_state_t *state);

/**
 * Returns goal op ID stored in the message.
 */
int planMAMsgHeurFFOpId(const plan_ma_msg_t *msg);

/**
 * Returns number of operators stored in message.
 */
int planMAMsgHeurFFOpSize(const plan_ma_msg_t *msg);

/**
 * Returns i'th operator's ID and its cost and owner.
 */
int planMAMsgHeurFFOp(const plan_ma_msg_t *msg, int i,
                      plan_cost_t *cost, int *owner);

#if 0
typedef void plan_ma_msg_t;

/**
 * Creates a new message.
 */
plan_ma_msg_t *planMAMsgNew(void);

/**
 * Deletes message.
 */
void planMAMsgDel(plan_ma_msg_t *msg);

/**
 * Returns true if the message is of type msg_type (see PLAN_MA_MSG_TYPE_*
 * above).
 */
int planMAMsgIsType(const plan_ma_msg_t *msg, int msg_type);

/**
 * Returns type as integer -- just for testing.
 */
int planMAMsgType(const plan_ma_msg_t *msg);

/**
 * Returns heap-allocated packed message and its size.
 */
void *planMAMsgPacked(const plan_ma_msg_t *msg, size_t *size);

/**
 * Returns a new message unpacked from the given buffer.
 */
plan_ma_msg_t *planMAMsgUnpacked(void *buf, size_t size);

/**
 * Returns true if the message is one of SEARCH types.
 */
int planMAMsgIsSearchType(const plan_ma_msg_t *msg);

/**
 * Set public-state type of message.
 */
void planMAMsgSetPublicState(plan_ma_msg_t *msg, int agent_id,
                             const void *state, size_t state_size,
                             int state_id,
                             int cost, int heuristic);

/**
 * Returns true if the message is of type public-state.
 */
int planMAMsgIsPublicState(const plan_ma_msg_t *msg);

/**
 * Returns agent_id of the source agent.
 */
int planMAMsgPublicStateAgent(const plan_ma_msg_t *msg);

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

/**
 * Returns true if the message is any of TERMINATE* types.
 */
int planMAMsgIsTerminateType(const plan_ma_msg_t *msg);

/**
 * Sets message as type TERMINATE
 */
void planMAMsgSetTerminate(plan_ma_msg_t *msg);

/**
 * Returns if the message is of type TERMINATE
 */
int planMAMsgIsTerminate(const plan_ma_msg_t *msg);

/**
 * Sets message as TERMINATE_REQUEST
 */
void planMAMsgSetTerminateRequest(plan_ma_msg_t *msg, int agent_id);

/**
 * Returns if the message is of type TERMINATE_REQUEST
 */
int planMAMsgIsTerminateRequest(const plan_ma_msg_t *msg);

/**
 * Returns agent ID from the TERMINATE_REQUEST message.
 */
int planMAMsgTerminateRequestAgent(const plan_ma_msg_t *msg);

/**
 * Initializes message as TRACE_PATH
 */
void planMAMsgSetTracePath(plan_ma_msg_t *msg, int origin_agent_id);

/**
 * Sets the state from which tracing should continue
 */
void planMAMsgTracePathSetStateId(plan_ma_msg_t *msg,
                                  int state_id);

/**
 * Adds operator to the path
 */
void planMAMsgTracePathAddOperator(plan_ma_msg_t *msg,
                                   const char *name,
                                   int cost);

/**
 * Sets tracing as done
 */
void planMAMsgTracePathSetDone(plan_ma_msg_t *msg);

/**
 * Returns true if the message is TRACE_PATH message.
 */
int planMAMsgIsTracePath(const plan_ma_msg_t *msg);

/**
 * Returns true if the path tracing is done.
 */
int planMAMsgTracePathIsDone(const plan_ma_msg_t *msg);

/**
 * Returns ID of the agent who started tracing.
 */
int planMAMsgTracePathOriginAgent(const plan_ma_msg_t *msg);

/**
 * Returns state_id from the trace path message.
 */
int planMAMsgTracePathStateId(const plan_ma_msg_t *msg);

/**
 * Returns number of operators stored in path.
 */
int planMAMsgTracePathNumOperators(const plan_ma_msg_t *msg);

/**
 * Returns i'th operator from path and its cost.
 */
const char *planMAMsgTracePathOperator(const plan_ma_msg_t *msg, int i,
                                       int *cost);


/**
 * Returns true if the message if type of HEUR_REQUEST for any heuristic.
 */
int planMAMsgIsHeurRequest(const plan_ma_msg_t *msg);

/**
 * Returns true if the message if type of HEUR_RESPONSE for any heuristic.
 */
int planMAMsgIsHeurResponse(const plan_ma_msg_t *msg);

/**
 * Sets HEUR_FF_REQUEST type message.
 */
void planMAMsgSetHeurFFRequest(plan_ma_msg_t *msg,
                               int agent_id,
                               const int *state, int state_size,
                               int op_id);

/**
 * Returns true if the message is of type HEUR_REQUEST.
 */
int planMAMsgIsHeurFFRequest(const plan_ma_msg_t *msg);

/**
 * Returns agent_id stored in heur-request message.
 */
int planMAMsgHeurFFRequestAgentId(const plan_ma_msg_t *msg);

/**
 * Returns op_id stored in heur-request message.
 */
int planMAMsgHeurFFRequestOpId(const plan_ma_msg_t *msg);

/**
 * Returns var'th variable from the state stored in heur-request message.
 */
int planMAMsgHeurFFRequestState(const plan_ma_msg_t *msg, int var);



/**
 * Set heur-response type of message for ff heuristic.
 */
void planMAMsgSetHeurFFResponse(plan_ma_msg_t *msg, int op_id);

/**
 * Adds operator to the response
 */
void planMAMsgHeurFFResponseAddOp(plan_ma_msg_t *msg, int op_id, int cost);

/**
 * Adds peer operator to the response.
 */
void planMAMsgHeurFFResponseAddPeerOp(plan_ma_msg_t *msg,
                                      int op_id, int cost, int owner);

/**
 * Returns true if the message is of type heur-response.
 */
int planMAMsgIsHeurFFResponse(const plan_ma_msg_t *msg);

/**
 * Returns ref_id from the message
 */
int planMAMsgHeurFFResponseOpId(const plan_ma_msg_t *msg);

/**
 * Returns number of operators stored in response.
 */
int planMAMsgHeurFFResponseOpSize(const plan_ma_msg_t *msg);

/**
 * Returns i'th operator's ID and its cost.
 */
int planMAMsgHeurFFResponseOp(const plan_ma_msg_t *msg, int i, int *cost);

/**
 * Returns number of peer-operators stored in response.
 */
int planMAMsgHeurFFResponsePeerOpSize(const plan_ma_msg_t *msg);

/**
 * Returns i'th peer-operator's ID and the corresponding owner.
 */
int planMAMsgHeurFFResponsePeerOp(const plan_ma_msg_t *msg, int i,
                                  int *cost, int *owner);


/**
 * Creates a max-heuristic request.
 */
void planMAMsgSetHeurMaxRequest(plan_ma_msg_t *msg, int agent_id, 
                                const int *state, int state_size);

/**
 * Adds operator and its value to heur-max-request.
 */
void planMAMsgHeurMaxRequestAddOp(plan_ma_msg_t *msg, int op_id, int val);

/**
 * Returns true if the messsage is HEUR_MAX_REQUEST.
 */
int planMAMsgIsHeurMaxRequest(const plan_ma_msg_t *msg);

/**
 * Returns agent-id
 */
int planMAMsgHeurMaxRequestAgent(const plan_ma_msg_t *msg);

/**
 * Returns value of the var'th variable from the state.
 */
int planMAMsgHeurMaxRequestState(const plan_ma_msg_t *msg, int var);

/**
 * Number of operators stored in request.
 */
int planMAMsgHeurMaxRequestOpSize(const plan_ma_msg_t *msg);

/**
 * Returns i'th operator ID and corresponding value.
 */
int planMAMsgHeurMaxRequestOp(const plan_ma_msg_t *msg, int i, int *value);


/**
 * Creates a max-heuristic response.
 */
void planMAMsgSetHeurMaxResponse(plan_ma_msg_t *msg, int agent_id);

/**
 * Adds operator and its value to heur-max-request.
 */
void planMAMsgHeurMaxResponseAddOp(plan_ma_msg_t *msg, int op_id, int val);

/**
 * Returns true if the messsage is HEUR_MAX_REQUEST.
 */
int planMAMsgIsHeurMaxResponse(const plan_ma_msg_t *msg);

/**
 * Returns agent-id
 */
int planMAMsgHeurMaxResponseAgent(const plan_ma_msg_t *msg);

/**
 * Number of operators stored in request.
 */
int planMAMsgHeurMaxResponseOpSize(const plan_ma_msg_t *msg);

/**
 * Returns i'th operator ID and corresponding value.
 */
int planMAMsgHeurMaxResponseOp(const plan_ma_msg_t *msg, int i, int *value);



/**
 * Creates SOLUTION message.
 */
void planMAMsgSetSolution(plan_ma_msg_t *msg, int agent_id,
                          const void *state, size_t state_size,
                          int state_id,
                          int cost, int heuristic,
                          int token);

/**
 * Returns true if the message is of type SOLUTION.
 */
int planMAMsgIsSolution(const plan_ma_msg_t *msg);

/**
 * Returns public-state message created from the solution message.
 */
plan_ma_msg_t *planMAMsgSolutionPublicState(const plan_ma_msg_t *msg);

/**
 * Returns token associated with the solution.
 */
int planMAMsgSolutionToken(const plan_ma_msg_t *msg);

/**
 * Returns cost of the solution.
 */
int planMAMsgSolutionCost(const plan_ma_msg_t *msg);

/**
 * Returns source agent of the message.
 */
int planMAMsgSolutionAgent(const plan_ma_msg_t *msg);

/**
 * Returns state-id of the solution.
 */
int planMAMsgSolutionStateId(const plan_ma_msg_t *msg);


/**
 * Creates SOLUTION_ACK message.
 */
void planMAMsgSetSolutionAck(plan_ma_msg_t *msg, int agent_id,
                             int ack, int token);

/**
 * Returns true if the message is of type SOLUTION_ACK;
 */
int planMAMsgIsSolutionAck(const plan_ma_msg_t *msg);

/**
 * Returns ID of the source agent.
 */
int planMAMsgSolutionAckAgent(const plan_ma_msg_t *msg);

/**
 * Returns true if ACK or false if NACK.
 */
int planMAMsgSolutionAck(const plan_ma_msg_t *msg);

/**
 * Returns ID of the solution state.
 */
int planMAMsgSolutionAckToken(const plan_ma_msg_t *msg);


/**
 * Creates SOLUTION_MARK message.
 */
void planMAMsgSetSolutionMark(plan_ma_msg_t *msg, int agent_id, int token);

/**
 * Returns true if the message is of type SOLUTION_MARK.
 */
int planMAMsgIsSolutionMark(const plan_ma_msg_t *msg);

/**
 * Returns ID of the source agent.
 */
int planMAMsgSolutionMarkAgent(const plan_ma_msg_t *msg);

/**
 * Returns token of stop-mark message.
 */
int planMAMsgSolutionMarkToken(const plan_ma_msg_t *msg);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_MSG_H__ */
