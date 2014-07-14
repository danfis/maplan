#ifndef __PLAN_MA_COMM_H__
#define __PLAN_MA_COMM_H__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_COMM_H__ */
