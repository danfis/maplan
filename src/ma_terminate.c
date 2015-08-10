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

#include "plan/ma_terminate.h"

void planMATerminateInit(plan_ma_terminate_t *term,
                         plan_ma_terminate_update_fin_msg update_fin_fn,
                         plan_ma_terminate_receive_fin_msg receive_fin_fn,
                         void *userdata)
{
    bzero(term, sizeof(*term));
    term->state = PLAN_MA_TERMINATE_NONE;
    term->initiator_id = INT_MAX;

    term->update_fin_fn = update_fin_fn;
    term->receive_fin_fn = receive_fin_fn;
    term->userdata = userdata;
}

void planMATerminateFree(plan_ma_terminate_t *term)
{
}

int planMATerminateStart(plan_ma_terminate_t *term,
                         plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;

    if (term->state != PLAN_MA_TERMINATE_NONE)
        return -1;

    // Send request in ring to determine which node is the initiator in
    // case more than one termination starts in the same time
    msg = planMAMsgNew(PLAN_MA_MSG_TERMINATE,
                       PLAN_MA_MSG_TERMINATE_REQUEST,
                       comm->node_id);
    planMAMsgSetTerminateAgent(msg, comm->node_id);
    planMACommSendInRing(comm, msg);
    planMAMsgDel(msg);

    term->state = PLAN_MA_TERMINATE_IN_PROGRESS;
    term->is_initiator = 1;

    return 0;
}

int planMATerminateWait(plan_ma_terminate_t *term,
                        plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;

    if (term->state == PLAN_MA_TERMINATE_NONE)
        return -1;

    if (term->state == PLAN_MA_TERMINATE_TERMINATED)
        return 0;

    // Ignore all non-terminate messages
    while (term->state == PLAN_MA_TERMINATE_IN_PROGRESS
            && (msg = planMACommRecvBlock(comm, -1)) != NULL){
        if (planMAMsgType(msg) == PLAN_MA_MSG_TERMINATE)
            planMATerminateProcessMsg(term, msg, comm);
        planMAMsgDel(msg);
    }

    if (term->state == PLAN_MA_TERMINATE_IN_PROGRESS && msg == NULL)
        return -1;
    return 0;
}

static void prepareTargets(plan_ma_terminate_t *term,
                           plan_ma_comm_t *comm,
                           int initiator_id)
{
    int target;

    if (initiator_id == -1){
        term->is_first = 1;
        term->is_last = 0;
        term->final_counter = 0;
        term->final_target = 0;
        term->final_ack_counter = 0;
        term->final_ack_target = comm->node_size - 1;

    }else{
        term->is_first = 0;
        term->is_last = 0;
        term->final_counter = 0;
        term->final_ack_counter = 0;

        target = comm->node_id - initiator_id;
        target = target + comm->node_size;
        target = target % comm->node_size;
        term->final_target = target;
        target = comm->node_size - 1 - target;
        term->final_ack_target = target;
        if (term->final_ack_target == 0)
            term->is_last = 1;
    }
}

static void sendFinal(plan_ma_terminate_t *term,
                      plan_ma_comm_t *comm)
{
    plan_ma_msg_t *term_msg;
    int i, to;

    if (term->final_ack_target == 0)
        return;

    term_msg = planMAMsgNew(PLAN_MA_MSG_TERMINATE,
                            PLAN_MA_MSG_TERMINATE_FINAL,
                            comm->node_id);
    to = comm->node_id + 1;
    to = to % comm->node_size;
    for (i = 0; i < term->final_ack_target; ++i){
        planMACommSendToNode(comm, to, term_msg);
        to = (to + 1) % comm->node_size;
    }
    planMAMsgDel(term_msg);
}

static void sendFinalAck(plan_ma_terminate_t *term,
                         plan_ma_comm_t *comm)
{
    plan_ma_msg_t *term_msg;
    int i, to, node_size;

    if (term->final_target == 0)
        return;

    term_msg = planMAMsgNew(PLAN_MA_MSG_TERMINATE,
                            PLAN_MA_MSG_TERMINATE_FINAL_ACK,
                            comm->node_id);

    node_size = comm->node_size;
    to = comm->node_id - 1;
    to = (to + node_size) % node_size;
    for (i = 0; i < term->final_target; ++i){
        planMACommSendToNode(comm, to, term_msg);
        to = (to + node_size - 1) % node_size;
    }
    planMAMsgDel(term_msg);
}

static void sendFinalFin(plan_ma_terminate_t *term,
                         plan_ma_comm_t *comm)
{
    plan_ma_msg_t *term_msg;

    term_msg = planMAMsgNew(PLAN_MA_MSG_TERMINATE,
                            PLAN_MA_MSG_TERMINATE_FINAL_FIN,
                            comm->node_id);

    if (term->update_fin_fn != NULL)
        term->update_fin_fn(term_msg, term->userdata);

    planMACommSendToAll(comm, term_msg);
    planMAMsgDel(term_msg);
}

static int processMsgRequest(plan_ma_terminate_t *term,
                             plan_ma_msg_t *msg,
                             plan_ma_comm_t *comm)
{
    int agent_id;

    // Received a terminate request from other agent.
    agent_id = planMAMsgTerminateAgent(msg);

    if (agent_id == comm->node_id){
        // The REQ message circled and arrived back to the initiator.
        // Initialize flushing the message queues.
        term->initiator_id = agent_id;
        prepareTargets(term, comm, -1);
        sendFinal(term, comm);

    }else{
        // Define fixed priority over nodes in the cluster. If this
        // node requested termination then stop requests from the lower
        // priority nodes to pass trought the ring. Just ignore it.
        if (term->is_initiator && agent_id > comm->node_id)
            return 0;

        // Accept requests from the higher priority nodes.
        if (agent_id < term->initiator_id){
            term->initiator_id = agent_id;
            prepareTargets(term, comm, agent_id);
        }

        // Pass the message to the next node in the ring.
        planMACommSendInRing(comm, msg);
    }

    return 0;
}

int planMATerminateProcessMsg(plan_ma_terminate_t *term,
                              plan_ma_msg_t *msg,
                              plan_ma_comm_t *comm)
{
    int subtype = planMAMsgSubType(msg);

    if (subtype == PLAN_MA_MSG_TERMINATE_REQUEST){
        term->state = PLAN_MA_TERMINATE_IN_PROGRESS;
        return processMsgRequest(term, msg, comm);

    }else if (subtype == PLAN_MA_MSG_TERMINATE_FINAL){
        ++term->final_counter;

    }else if (subtype == PLAN_MA_MSG_TERMINATE_FINAL_ACK){
        ++term->final_ack_counter;

    }else if (subtype == PLAN_MA_MSG_TERMINATE_FINAL_FIN){
        // Final message signaling flushed message queues
        if (term->receive_fin_fn != NULL)
            term->receive_fin_fn(msg, term->userdata);
        term->state = PLAN_MA_TERMINATE_TERMINATED;
        return 1;

    }else{
        fprintf(stderr, "[%d] Terminate Error: Unknown terminate message.\n",
                comm->node_id);
        return -1;
    }

    // If we received the desired number of FINAL messages then:
    //   1. Start FINAL_ACK wave in case this is the last node in the
    //   cluster, or
    //   2. Send FINAL message to the nodes next in the cluster.
    if (term->final_counter == term->final_target){
        if (term->is_last){
            sendFinalAck(term, comm);
        }else if (!term->is_first){
            sendFinal(term, comm);
        }
        term->final_counter = -1;
    }

    // All FINAL_ACK messages received, send FINAL_ACK to all previous
    // nodes if this is not the first of the last node in cluster.
    if (term->final_ack_counter == term->final_ack_target){
        if (!term->is_first && !term->is_last)
            sendFinalAck(term, comm);
        term->final_ack_counter = -1;
    }

    // The first node in the cluster has sent all FINAL messages and
    // received all FINAL_ACK messages, so send the last FINAL_FIN messages
    // and terminate.
    if (term->final_ack_counter == -1
            && term->final_counter == -1
            && term->is_first){
        sendFinalFin(term, comm);
        term->state = PLAN_MA_TERMINATE_TERMINATED;
        return 1;
    }

    return 0;
}
