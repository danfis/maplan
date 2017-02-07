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

#ifndef __PLAN_MA_TERMINATE_H__
#define __PLAN_MA_TERMINATE_H__

#include <plan/ma_msg.h>
#include <plan/ma_comm.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PLAN_MA_TERMINATE_NONE        0
#define PLAN_MA_TERMINATE_IN_PROGRESS 1
#define PLAN_MA_TERMINATE_TERMINATED  2

typedef void (*plan_ma_terminate_update_fin_msg)(plan_ma_msg_t *msg, void *ud);
typedef void (*plan_ma_terminate_receive_fin_msg)(const plan_ma_msg_t *msg,
                                                  void *ud);

struct _plan_ma_terminate_t {
    int state;             /*!< State of termination:
                                none/in-progress/terminated */
    int is_initiator;      /*!< True if this node is the intiator of the
                                termination */
    int initiator_id;      /*!< Node ID of the initiator */
    int final_target;      /*!< Number of FINAL messages that this node is
                                waiting for */
    int final_counter;     /*!< Number of actually received FINAL messages */
    int final_ack_target;  /*!< Number FINAL_ACK messages this node is
                                waiting for */
    int final_ack_counter; /*!< Number of actually received FINAL_ACK msgs */
    int is_first;          /*!< True if this node is the first one in the ring */
    int is_last;           /*!< True if this node is the last one in the ring */

    plan_ma_terminate_update_fin_msg update_fin_fn;
    plan_ma_terminate_receive_fin_msg receive_fin_fn;
    void *userdata;
};
typedef struct _plan_ma_terminate_t plan_ma_terminate_t;


/**
 * Initializes terminate structure.
 */
void planMATerminateInit(plan_ma_terminate_t *term,
                         plan_ma_terminate_update_fin_msg update_fin_fn,
                         plan_ma_terminate_receive_fin_msg receive_fin_fn,
                         void *userdata);

/**
 * Frees terminate structure.
 */
void planMATerminateFree(plan_ma_terminate_t *term);

/**
 * Starts termination of the cluster.
 * Returns 0 on success, -1 if already in-progress or already terminated.
 */
int planMATerminateStart(plan_ma_terminate_t *term,
                         plan_ma_comm_t *comm);


int planMATerminateWait(plan_ma_terminate_t *term,
                        plan_ma_comm_t *comm);

/**
 * Process one *_TERMINATE_* message.
 */
int planMATerminateProcessMsg(plan_ma_terminate_t *term,
                              plan_ma_msg_t *msg,
                              plan_ma_comm_t *comm);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_TERMINATE_H__ */
