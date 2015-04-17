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
#include <boruvka/hfunc.h>
#include <plan/ma_state.h>

//#define ENABLE_ASSERTS

struct _priv_el_t {
    int id;
    bor_list_t htable; /*!< Connector to bor_htable_t */
    char buf[];        /*!< Buffer with private part of packed state */
};
typedef struct _priv_el_t priv_el_t;

static void privInitEl(void *el, int id, const void *userdata);
static bor_htable_key_t privTableHash(const bor_list_t *key, void *ud);
static int privTableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud);

/** Returns unique ID corresponding to the private part of the given state */
static int privID(plan_ma_state_t *ma_state, const void *statebuf);
/** Returns private part buffer corresponding to the specified buffer ID */
static const void *privBuf(plan_ma_state_t *ma_state, int id);

plan_ma_state_t *planMAStateNew(plan_state_pool_t *state_pool,
                                int num_agents, int agent_id)
{
    plan_ma_state_t *ma_state;
    int i, id, size;
    const void *buf;

    ma_state = BOR_ALLOC(plan_ma_state_t);
    ma_state->state_pool = state_pool;
    ma_state->packer = state_pool->packer;
    ma_state->num_agents = num_agents;
    ma_state->agent_id = agent_id;
    ma_state->bufsize = planStatePackerBufSize(state_pool->packer);
    ma_state->buf = BOR_CALLOC_ARR(char, ma_state->bufsize);
    ma_state->ma_privacy = 0;
    ma_state->private_ids = NULL;
    ma_state->pub_buf = NULL;
    ma_state->pub_bufsize = 0;
    ma_state->priv_bufsize = 0;
    ma_state->priv_data = NULL;
    ma_state->priv_table = NULL;
    ma_state->priv_size = 0;

    if (state_pool->packer->ma_privacy){
        // Enable ma-privacy mode
        ma_state->ma_privacy = 1;

        // Pre-allocate various arrays
        ma_state->private_ids = BOR_ALLOC_ARR(int, num_agents);
        ma_state->pub_bufsize = planStatePackerBufSizePubPart(ma_state->packer);
        ma_state->pub_buf = BOR_ALLOC_ARR(char, ma_state->pub_bufsize);

        // Initialize table of private state IDs
        planMAPrivateStateInit(&ma_state->private_state, num_agents, agent_id);

        // Prepare default array of private state identification.
        for (i = 0; i < num_agents; ++i)
            ma_state->private_ids[i] = 0;

        // Insert default array
        id = planMAPrivateStateInsert(&ma_state->private_state,
                                      ma_state->private_ids);

        // Initialize structure for private parts of packed states
        ma_state->priv_bufsize = planStatePackerBufSizePrivatePart(ma_state->packer);
        if (ma_state->priv_bufsize > 0){
            size = sizeof(priv_el_t) + ma_state->priv_bufsize;
            ma_state->priv_data = borExtArrNew2(size, 32, 128, privInitEl, NULL);
            ma_state->priv_table = borHTableNew(privTableHash, privTableEq,
                                                (void *)ma_state);
        }

        // Make sure that we don't need any more records in
        // ma_state->private_state structure.
        for (i = 0; i < ma_state->state_pool->num_states; ++i){
            buf = planStatePoolGetPackedState(state_pool, i);
            while (planStatePackerGetMAPrivacyVar(ma_state->packer, buf) > id){
                id = planMAPrivateStateInsert(&ma_state->private_state,
                                              ma_state->private_ids);
            }

            // Initialize table of private parts.
            privID(ma_state, buf);
        }
    }

    return ma_state;
}

void planMAStateDel(plan_ma_state_t *ma_state)
{
    if (ma_state->ma_privacy)
        planMAPrivateStateFree(&ma_state->private_state);
    if (ma_state->private_ids != NULL)
        BOR_FREE(ma_state->private_ids);
    if (ma_state->pub_buf != NULL)
        BOR_FREE(ma_state->pub_buf);
    if (ma_state->buf != NULL)
        BOR_FREE(ma_state->buf);
    if (ma_state->priv_table)
        borHTableDel(ma_state->priv_table);
    if (ma_state->priv_data)
        borExtArrDel(ma_state->priv_data);
    BOR_FREE(ma_state);
}

static void setPrivacyMAMsg(plan_ma_state_t *ma_state,
                            plan_state_id_t state_id, const void *buf,
                            plan_ma_msg_t *ma_msg)
{
    int private_id, priv_id;

    // Get ID to ma_state->private_state table
    private_id = planStatePackerGetMAPrivacyVar(ma_state->packer, buf);
    if (private_id < 0){
        fprintf(stderr, "MA-State Error: You are probably trying to send a"
                        " state that is not completely defined. Make sure"
                        " that you didn't use planMAStateGetFromMAMsg() and"
                        " then re-sent the state again.\n");
        exit(-1);
    }

    // Recover private IDs and set the corresponding local ID
    planMAPrivateStateGet(&ma_state->private_state, private_id,
                          ma_state->private_ids);
    priv_id = privID(ma_state, buf);
    ma_state->private_ids[ma_state->agent_id] = priv_id;

    // Get packed public part
    planStatePackerExtractPubPart(ma_state->packer, buf, ma_state->pub_buf);

    // Set message
    planMAMsgSetStateBuf(ma_msg, ma_state->pub_buf, ma_state->pub_bufsize);
    planMAMsgSetStatePrivateIds(ma_msg, ma_state->private_ids,
                                ma_state->num_agents);
}

int planMAStateSetMAMsg(plan_ma_state_t *ma_state,
                        plan_state_id_t state_id,
                        plan_ma_msg_t *ma_msg)
{
    const void *buf;

    buf = planStatePoolGetPackedState(ma_state->state_pool, state_id);
    if (buf == NULL)
        return -1;

    if (ma_state->ma_privacy){
        setPrivacyMAMsg(ma_state, state_id, buf, ma_msg);
    }else{
        planMAMsgSetStateBuf(ma_msg, buf, ma_state->bufsize);
    }

    planMAMsgSetStateId(ma_msg, state_id);
    return 0;
}

int planMAStateSetMAMsg2(plan_ma_state_t *ma_state,
                         const plan_state_t *state,
                         plan_ma_msg_t *ma_msg)
{
    planStatePackerPack(ma_state->packer, state, ma_state->buf);

    if (ma_state->ma_privacy){
        setPrivacyMAMsg(ma_state, state->state_id, ma_state->buf, ma_msg);
    }else{
        planMAMsgSetStateBuf(ma_msg, ma_state->buf, ma_state->bufsize);
    }

    return 0;
}

static void recoverPrivacyStateBuf(plan_ma_state_t *ma_state,
                                   const plan_ma_msg_t *ma_msg,
                                   int recover_privacy_var)
{
    const void *pubbuf;
    const void *ref_privbuf;
    plan_state_id_t ref_state_id;
    int private_id;

#ifdef ENABLE_ASSERTS
    if (planMAMsgStateBufSize(ma_msg) != ma_state->pub_bufsize
            || planMAMsgStatePrivateIdsSize(ma_msg) != ma_state->num_agents){
        fprintf(stderr, "Error: You're trying to recover state from the"
                        " wrong ma message. Exiting...\n");
        exit(-1);
    }
#endif /* ENABLE_ASSERTS */

    // Read packed public part of state and private state IDs from the
    // message
    pubbuf = planMAMsgStateBuf(ma_msg);
    planMAMsgStatePrivateIds(ma_msg, ma_state->private_ids);

    // Cherry-pick state ID corresponding to the current agent
    ref_state_id = ma_state->private_ids[ma_state->agent_id];

    // Retrieve private part of the packed state corresponding to the
    // ref_state_id and set it to the pre-allocated buffer.
    ref_privbuf = privBuf(ma_state, ref_state_id);
    if (ref_privbuf != NULL){
        planStatePackerSetPrivatePart(ma_state->packer, ref_privbuf, ma_state->buf);
    }

    // Exchange the public part of the output buffer with the received
    // public part from the message.
    planStatePackerSetPubPart(ma_state->packer, pubbuf, ma_state->buf);

    if (recover_privacy_var){
        // Get the correct ID to the table of private state IDs and set it to
        // the output buffer
        private_id = planMAPrivateStateInsert(&ma_state->private_state,
                                              ma_state->private_ids);
        planStatePackerSetMAPrivacyVar(ma_state->packer, private_id,
                                       ma_state->buf);
    }else{
        // Set privacy var to some bogus value so we can detect it when it
        // reused later.
        planStatePackerSetMAPrivacyVar(ma_state->packer, -1, ma_state->buf);
    }
}

plan_state_id_t planMAStateInsertFromMAMsg(plan_ma_state_t *ma_state,
                                           const plan_ma_msg_t *ma_msg)
{
    const void *buf;

    if (ma_state->ma_privacy){
        recoverPrivacyStateBuf(ma_state, ma_msg, 1);
        buf = ma_state->buf;
    }else{
        buf = planMAMsgStateBuf(ma_msg);
    }
    return planStatePoolInsertPacked(ma_state->state_pool, buf);
}

void planMAStateGetFromMAMsg(plan_ma_state_t *ma_state,
                             const plan_ma_msg_t *ma_msg,
                             plan_state_t *state)
{
    const void *buf;

    if (ma_state->ma_privacy){
        recoverPrivacyStateBuf(ma_state, ma_msg, 0);
        buf = ma_state->buf;
    }else{
        buf = planMAMsgStateBuf(ma_msg);
    }
    planStatePackerUnpack(ma_state->packer, buf, state);
}

int planMAStateMAMsgIsSet(const plan_ma_state_t *ma_state,
                          const plan_ma_msg_t *ma_msg)
{
    if (ma_state->ma_privacy){
        return planMAMsgStateBufSize(ma_msg) == ma_state->pub_bufsize;
    }else{
        return planMAMsgStateBufSize(ma_msg) == ma_state->bufsize;
    }
}

static void privInitEl(void *_el, int id, const void *userdata)
{
    priv_el_t *el = _el;
    el->id = id;
}

static bor_htable_key_t privTableHash(const bor_list_t *key, void *ud)
{
    const priv_el_t *el = BOR_LIST_ENTRY(key, priv_el_t, htable);
    const plan_ma_state_t *ma_state = ud;
    return borCityHash_64(el->buf, ma_state->priv_bufsize);
}

static int privTableEq(const bor_list_t *k1, const bor_list_t *k2, void *ud)
{
    const priv_el_t *el1 = BOR_LIST_ENTRY(k1, priv_el_t, htable);
    const priv_el_t *el2 = BOR_LIST_ENTRY(k2, priv_el_t, htable);
    const plan_ma_state_t *ma_state = ud;
    return memcmp(el1->buf, el2->buf, ma_state->priv_bufsize) == 0;
}

static int privID(plan_ma_state_t *ma_state, const void *statebuf)
{
    priv_el_t *el;
    bor_list_t *hfound;

    if (ma_state->priv_data == NULL)
        return 0;

    // Get next element from array
    el = borExtArrGet(ma_state->priv_data, ma_state->priv_size);

    // Read private part into that element so we can avoid another mem
    // allocation
    planStatePackerExtractPrivatePart(ma_state->packer, statebuf, el->buf);

    // Try to insert it into hash table and return result
    hfound = borHTableInsertUnique(ma_state->priv_table, &el->htable);
    if (hfound == NULL){
        ++ma_state->priv_size;
        return el->id;
    }else{
        el = BOR_LIST_ENTRY(hfound, priv_el_t, htable);
        return el->id;
    }
}

static const void *privBuf(plan_ma_state_t *ma_state, int id)
{
    priv_el_t *el;

    if (ma_state->priv_data == NULL)
        return NULL;

    el = borExtArrGet(ma_state->priv_data, id);
    return el->buf;
}
