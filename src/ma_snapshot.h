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

#ifndef __PLAN_MA_SNAPSHOT_H__
#define __PLAN_MA_SNAPSHOT_H__

#include <plan/ma_msg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _plan_ma_snapshot_t plan_ma_snapshot_t;

/**
 * Delete method
 */
typedef void (*plan_ma_snapshot_del)(plan_ma_snapshot_t *s);

/**
 * Update snapshot object by a non-snapshot message.
 */
typedef void (*plan_ma_snapshot_update)(plan_ma_snapshot_t *s,
                                        plan_ma_msg_t *msg);

/**
 * Method for processing snapshot-init message.
 */
typedef void (*plan_ma_snapshot_init)(plan_ma_snapshot_t *s,
                                      plan_ma_msg_t *msg);

/**
 * Method for processing snapshot-mark message.
 */
typedef void (*plan_ma_snapshot_mark)(plan_ma_snapshot_t *s,
                                      plan_ma_msg_t *msg);

/**
 * Method for processing snapshot-response message.
 */
typedef void (*plan_ma_snapshot_response)(plan_ma_snapshot_t *s,
                                          plan_ma_msg_t *msg);

/**
 * This method is called whenever snapshot-mark messages from all other
 * agents were received. Returns 0 if object should wait for
 * snapshot-response messages or -1 if not (and in this case the object is
 * deleted by *del* method).
 */
typedef int (*plan_ma_snapshot_mark_finalize)(plan_ma_snapshot_t *s);

/**
 * This method is called whenever response messages from all other agents
 * were received. After this method the snapshot object is deleted.
 */
typedef void (*plan_ma_snapshot_response_finalize)(plan_ma_snapshot_t *s);

/**
 * Base snapshot object.
 */
struct _plan_ma_snapshot_t {
    long token;      /*!< Identification token of the snapshot object */
    int *mark;       /*!< Bool array. True if a snapshot-mark message was
                          received for the corresponding agent. */
    int mark_remain; /*!< Number of snapshot-mark messages remaining to
                          receive */
    int *resp;       /*!< Similar to .makr[] but for snapshot-reposnse msgs */
    int resp_remain; /*!< Same as .mark_remain but for snapshot-response msgs */

    plan_ma_snapshot_del del_fn;
    plan_ma_snapshot_update update_fn;
    plan_ma_snapshot_init init_fn;
    plan_ma_snapshot_mark mark_fn;
    plan_ma_snapshot_response response_fn;
    plan_ma_snapshot_mark_finalize mark_finalize_fn;
    plan_ma_snapshot_response_finalize response_finalize_fn;
};

/**
 * Registry of active snapshots.
 */
struct _plan_ma_snapshot_reg_t {
    plan_ma_snapshot_t **snapshot;
    int size;
    int alloc;
};
typedef struct _plan_ma_snapshot_reg_t plan_ma_snapshot_reg_t;

/**
 * Initializes snashot registry.
 */
void planMASnapshotRegInit(plan_ma_snapshot_reg_t *snapshot_reg,
                           int agent_size);

/**
 * Frees allocated resources in snapshot registry
 */
void planMASnapshotRegFree(plan_ma_snapshot_reg_t *snapshot_reg);

/**
 * Adds snapshot object to the register.
 */
void planMASnapshotRegAdd(plan_ma_snapshot_reg_t *reg, plan_ma_snapshot_t *s);

/**
 * Process message.
 * Returns 0 on success and -1 if the message is snapshot message and no
 * corresponding snapshot object is registered. In that case, the caller
 * should create corresponding snapshot object, add it using
 * planMASnapshotRegAdd() and call this function again.
 */
int planMASnapshotRegMsg(plan_ma_snapshot_reg_t *reg, plan_ma_msg_t *msg);

/**
 * Returns true if there are no active snapshot objects.
 */
_bor_inline int planMASnapshotRegEmpty(const plan_ma_snapshot_reg_t *reg);


/**
 * Initializes parent snapshot object.
 */
void _planMASnapshotInit(plan_ma_snapshot_t *s,
                         long token,
                         int cur_agent_id, int agent_size,
                         plan_ma_snapshot_del del,
                         plan_ma_snapshot_update update,
                         plan_ma_snapshot_init init_fn,
                         plan_ma_snapshot_mark mark,
                         plan_ma_snapshot_response response,
                         plan_ma_snapshot_mark_finalize mark_finalize,
                         plan_ma_snapshot_response_finalize response_finalize);
                          
/**
 * Frees snapshot object.
 */
void _planMASnapshotFree(plan_ma_snapshot_t *s);


/**** INLINES: ****/
_bor_inline int planMASnapshotRegEmpty(const plan_ma_snapshot_reg_t *reg)
{
    return reg->size == 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_MA_SNAPSHOT_H__ */
