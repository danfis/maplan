#ifndef __PLAN_MA_STATE_H__
#define __PLAN_MA_STATE_H__

#include <plan/state_pool.h>
#include <plan/ma_private_state.h>
#include <plan/ma_msg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_ma_state_t {
    plan_state_pool_t *state_pool;
    plan_state_packer_t *packer;
    int bufsize;
    int num_agents;
    int agent_id;
    int ma_privacy; /*!< True if ma-privacy is enabled */
    plan_ma_private_state_t private_state;
};
typedef struct _plan_ma_state_t plan_ma_state_t;


/**
 * Creates ma-state object
 */
plan_ma_state_t *planMAStateNew(plan_state_pool_t *state_pool,
                                int num_agents, int agent_id);

/**
 * Destruct ma-state object.
 */
void planMAStateDel(plan_ma_state_t *ma_state);

/**
 * Sets state to ma-msg object.
 * Returns 0 on success.
 */
int planMAStateSetMAMsg(const plan_ma_state_t *ma_state,
                        plan_state_id_t state_id,
                        plan_ma_msg_t *ma_msg);

/**
 * Same as planMAStateSetMAMsg() but state is set from the unrolled state.
 */
int planMAStateSetMAMsg2(const plan_ma_state_t *ma_state,
                         const plan_state_t *state,
                         plan_ma_msg_t *ma_msg);

/**
 * Inserts state stored in ma_msg into state-pool.
 */
plan_state_id_t planMAStateInsertFromMAMsg(plan_ma_state_t *ma_state,
                                           const plan_ma_msg_t *ma_msg);


/**
 * Unpack state from the ma-msg object.
 */
void planMAStateGetFromMAMsg(const plan_ma_state_t *ma_state,
                             const plan_ma_msg_t *ma_msg,
                             plan_state_t *state);

/**
 * Returns true if the state is properly set in the ma message.
 */
int planMAStateMAMsgIsSet(const plan_ma_state_t *ma_state,
                          const plan_ma_msg_t *ma_msg);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_STATE_H__ */
