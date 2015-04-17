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

#ifndef __PLAN_MA_STATE_H__
#define __PLAN_MA_STATE_H__

#include <boruvka/htable.h>
#include <boruvka/extarr.h>

#include <plan/state_pool.h>
#include <plan/ma_private_state.h>
#include <plan/ma_msg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_ma_state_t {
    plan_state_pool_t *state_pool;
    plan_state_packer_t *packer;
    int num_agents; /*!< Number of agents in cluster */
    int agent_id;   /*!< ID of the current agent */
    int bufsize;    /*!< Size of the buffer needed for packed state */
    void *buf;      /*!< Pre-allocated buffer for the packed state */
    int ma_privacy; /*!< True if ma-privacy is enabled */
    /*!< Table of state IDs that is used for tracking private parts of states */
    plan_ma_private_state_t private_state;

    int *private_ids; /*!< Pre-allocated array for private IDs */
    void *pub_buf; /*!< Pre-allocated buffer for public part of packed state */
    int pub_bufsize; /*!< Size of the buffer for public part of packed state */

    int priv_bufsize; /*!< Size of the private part of the packed state. */
    bor_extarr_t *priv_data; /*!< Array of elements in .priv_table */
    bor_htable_t *priv_table; /*!< Hash table of unique private parts */
    int priv_size; /*!< Number of private parts stored in table */
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
int planMAStateSetMAMsg(plan_ma_state_t *ma_state,
                        plan_state_id_t state_id,
                        plan_ma_msg_t *ma_msg);

/**
 * Same as planMAStateSetMAMsg() but state is set from the unrolled state.
 */
int planMAStateSetMAMsg2(plan_ma_state_t *ma_state,
                         const plan_state_t *state,
                         plan_ma_msg_t *ma_msg);

/**
 * Inserts state stored in ma_msg into state-pool.
 */
plan_state_id_t planMAStateInsertFromMAMsg(plan_ma_state_t *ma_state,
                                           const plan_ma_msg_t *ma_msg);


/**
 * Unpack state from the ma-msg object. The unpacked state is not complete
 * in the ma-privacy mode because ma-privacy variable is set to a bogus
 * value. Make sure that you never resend state unpacked this way from a
 * message!
 */
void planMAStateGetFromMAMsg(plan_ma_state_t *ma_state,
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
