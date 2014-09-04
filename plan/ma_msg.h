#ifndef __PLAN_MA_COMM_H__
#define __PLAN_MA_COMM_H__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void plan_ma_msg_t;

/**
 * Frees global memory allocated by google protobuffers.
 * This is just wrapper for
 *      google::protobuf::ShutdownProtobufLibrary().
 */
void planShutdownProtobuf(void);

/**
 * Creates a new message.
 */
plan_ma_msg_t *planMAMsgNew(void);

/**
 * Deletes message.
 */
void planMAMsgDel(plan_ma_msg_t *msg);

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
 * Sets agent's token
 */
void planMAMsgPublicStateSetToken(plan_ma_msg_t *msg,
                                  int agent_id, int token);

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
 * Returns token corresponding to the agent.
 */
int planMAMsgPublicStateToken(const plan_ma_msg_t *msg, int agent_id);

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
void planMAMsgSetTerminateRequest(plan_ma_msg_t *msg);

/**
 * Returns if the message is of type TERMINATE_REQUEST
 */
int planMAMsgIsTerminateRequest(const plan_ma_msg_t *msg);

/**
 * Sets message as TERMINATE_ACK
 */
void planMAMsgSetTerminateAck(plan_ma_msg_t *msg);

/**
 * Returns if the message is of type TERMINATE_ACK
 */
int planMAMsgIsTerminateAck(const plan_ma_msg_t *msg);

/**
 * Returns true if the message one of TERMINATE_* type.
 */
int planMAMsgIsTerminateType(const plan_ma_msg_t *msg);

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
 * Sets HEUR_REQUEST type message.
 */
void planMAMsgSetHeurRequest(plan_ma_msg_t *msg,
                             int agent_id,
                             const int *state, int state_size,
                             int op_id);

/**
 * Returns true if the message is of type HEUR_REQUEST.
 */
int planMAMsgIsHeurRequest(const plan_ma_msg_t *msg);

/**
 * Returns agent_id stored in heur-request message.
 */
int planMAMsgHeurRequestAgentId(const plan_ma_msg_t *msg);

/**
 * Returns op_id stored in heur-request message.
 */
int planMAMsgHeurRequestOpId(const plan_ma_msg_t *msg);

/**
 * Returns var'th variable from the state stored in heur-request message.
 */
int planMAMsgHeurRequestState(const plan_ma_msg_t *msg, int var);



/**
 * Set heur-response type of message
 */
void planMAMsgSetHeurResponse(plan_ma_msg_t *msg, int op_id);

/**
 * Adds operator to the response
 */
void planMAMsgHeurResponseAddOp(plan_ma_msg_t *msg, int op_id, int cost);

/**
 * Adds peer operator to the response.
 */
void planMAMsgHeurResponseAddPeerOp(plan_ma_msg_t *msg,
                                    int op_id, int owner);

/**
 * Returns true if the message is of type heur-response.
 */
int planMAMsgIsHeurResponse(const plan_ma_msg_t *msg);

/**
 * Returns ref_id from the message
 */
int planMAMsgHeurResponseOpId(const plan_ma_msg_t *msg);

/**
 * Returns number of operators stored in response.
 */
int planMAMsgHeurResponseOpSize(const plan_ma_msg_t *msg);

/**
 * Returns i'th operator's ID and its cost.
 */
int planMAMsgHeurResponseOp(const plan_ma_msg_t *msg, int i, int *cost);

/**
 * Returns number of peer-operators stored in response.
 */
int planMAMsgHeurResponsePeerOpSize(const plan_ma_msg_t *msg);

/**
 * Returns i'th peer-operator's ID and the corresponding owner.
 */
int planMAMsgHeurResponsePeerOp(const plan_ma_msg_t *msg, int i, int *owner);



/**
 * Creates SOLUTION_ACK message.
 */
void planMAMsgSetSolutionAck(plan_ma_msg_t *msg, int state_id);

/**
 * Returns ID of the solution state.
 */
int planMAMsgSolutionAckStateId(const plan_ma_msg_t *msg);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_COMM_H__ */
