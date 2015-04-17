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

#include <boruvka/alloc.h>

#include "ma_snapshot.h"

static void planMASnapshotUpdate(plan_ma_snapshot_t *s, plan_ma_msg_t *msg);
static int planMASnapshotSnapshotMsg(plan_ma_snapshot_t *s,
                                     plan_ma_msg_t *msg);

void planMASnapshotRegInit(plan_ma_snapshot_reg_t *reg,
                           int agent_size)
{
    reg->size = 0;
    reg->alloc = 0;
    reg->snapshot = NULL;
}

void planMASnapshotRegFree(plan_ma_snapshot_reg_t *reg)
{
    int i;

    for (i = 0; i < reg->size; ++i){
        reg->snapshot[i]->del_fn(reg->snapshot[i]);
    }

    if (reg->snapshot)
        BOR_FREE(reg->snapshot);
}

void planMASnapshotRegAdd(plan_ma_snapshot_reg_t *reg, plan_ma_snapshot_t *s)
{
    if (reg->alloc <= reg->size){
        reg->alloc = reg->size + 1;
        reg->snapshot = BOR_REALLOC_ARR(reg->snapshot, plan_ma_snapshot_t *,
                                        reg->alloc);
    }
    reg->snapshot[reg->size++] = s;
}

int planMASnapshotRegMsg(plan_ma_snapshot_reg_t *reg, plan_ma_msg_t *msg)
{
    int type = planMAMsgType(msg);
    long token;
    int i;

    if (type == PLAN_MA_MSG_SNAPSHOT){
        token = planMAMsgSnapshotToken(msg);

        for (i = 0; i < reg->size; ++i){
            if (reg->snapshot[i]->token == token){
                if (planMASnapshotSnapshotMsg(reg->snapshot[i], msg) == 1){
                    reg->snapshot[i] = reg->snapshot[--reg->size];
                }
                return 0;
            }
        }
        return -1;
    }

    for (i = 0; i < reg->size; ++i){
        planMASnapshotUpdate(reg->snapshot[i], msg);
    }
    return 0;
}

void _planMASnapshotInit(plan_ma_snapshot_t *s,
                         long token,
                         int cur_agent_id, int agent_size,
                         plan_ma_snapshot_del del,
                         plan_ma_snapshot_update update,
                         plan_ma_snapshot_init init_fn,
                         plan_ma_snapshot_mark mark,
                         plan_ma_snapshot_response response,
                         plan_ma_snapshot_mark_finalize mark_finalize,
                         plan_ma_snapshot_response_finalize response_finalize)
{
    s->token = token;
    s->mark = BOR_CALLOC_ARR(int, agent_size);
    s->mark_remain = agent_size - 1;
    s->resp = BOR_CALLOC_ARR(int, agent_size);
    s->resp_remain = agent_size - 1;

    s->del_fn = del;
    s->update_fn = update;
    s->init_fn = init_fn;
    s->mark_fn = mark;
    s->response_fn = response;
    s->mark_finalize_fn = mark_finalize;
    s->response_finalize_fn = response_finalize;
}

void _planMASnapshotFree(plan_ma_snapshot_t *s)
{
    if (s->mark)
        BOR_FREE(s->mark);
    if (s->resp)
        BOR_FREE(s->resp);
}

static void planMASnapshotUpdate(plan_ma_snapshot_t *s, plan_ma_msg_t *msg)
{
    // Skip messages where we already received mark message
    int agent = planMAMsgAgent(msg);
    if (s->mark[agent])
        return;

    if (s->update_fn != NULL)
        s->update_fn(s, msg);
}

static int planMASnapshotSnapshotMsg(plan_ma_snapshot_t *s,
                                     plan_ma_msg_t *msg)
{
    int subtype = planMAMsgSubType(msg);
    int agent = planMAMsgAgent(msg);
    int mark_finalize = 0;
    int resp_finalize = 0;
    int del = 0;

    // snapshot-init works also as a mark message
    if (subtype == PLAN_MA_MSG_SNAPSHOT_INIT
            || subtype == PLAN_MA_MSG_SNAPSHOT_MARK){
        if (s->mark[agent]){
            // TODO
            fprintf(stderr, "Error: mark from %d already received!\n",
                    agent);
            exit(-1);
        }
        s->mark[agent] = 1;
        if (--s->mark_remain == 0)
            mark_finalize = 1;

    }else if (subtype == PLAN_MA_MSG_SNAPSHOT_RESPONSE){
        if (s->resp[agent]){
            // TODO
            fprintf(stderr, "Error: response from %d already received!\n",
                    agent);
            exit(-1);
        }
        s->resp[agent] = 1;
        if (--s->resp_remain == 0)
            resp_finalize = 1;
    }

    if (subtype == PLAN_MA_MSG_SNAPSHOT_INIT){
        if (s->init_fn)
            s->init_fn(s, msg);
    }else if (subtype == PLAN_MA_MSG_SNAPSHOT_MARK){
        if (s->mark_fn)
            s->mark_fn(s, msg);
    }else if (subtype == PLAN_MA_MSG_SNAPSHOT_RESPONSE){
        if (s->response_fn)
            s->response_fn(s, msg);
    }

    if (mark_finalize){
        if (s->mark_finalize_fn && s->mark_finalize_fn(s) == -1){
            s->del_fn(s);
            del = 1;
        }
    }

    if (resp_finalize){
        if (s->response_finalize_fn)
            s->response_finalize_fn(s);
        s->del_fn(s);
        del = 1;
    }

    return del;
}
