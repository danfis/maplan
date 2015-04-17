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

#ifndef __PLAN_STATE_PACKER_H__
#define __PLAN_STATE_PACKER_H__

#include <plan/common.h>
#include <plan/state.h>
#include <plan/part_state.h>
#include <plan/var.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Forward declaration */
struct _plan_state_packer_var_t;

/**
 * Struct implementing packing of states into binary buffers.
 */
struct _plan_state_packer_t {
    struct _plan_state_packer_var_t *vars;
    int num_vars;
    int bufsize;

    int pub_bufsize; /*!< Size of the buffer for public part only */
    int pub_last_word; /*!< ID of the last word of public part */
    plan_packer_word_t pub_last_word_mask; /*!< Mask for the last word in
                                                public buffer */

    int private_bufsize; /*!< Size of the buffer for the private part only */
    int private_first_word; /*!< ID of the first word where is stored
                                 private part */
    plan_packer_word_t private_first_word_mask;

    int ma_privacy; /*!< True if there is ma-privacy variable */
};
typedef struct _plan_state_packer_t plan_state_packer_t;

/**
 * Creates a new object for packing states into binary buffers.
 */
plan_state_packer_t *planStatePackerNew(const plan_var_t *var,
                                        int var_size);

/**
 * Deletes a state packer object.
 */
void planStatePackerDel(plan_state_packer_t *p);

/**
 * Creates an exact copy of the packer object.
 */
plan_state_packer_t *planStatePackerClone(const plan_state_packer_t *p);

/**
 * Returns size of the buffer in bytes required for storing packed state.
 */
_bor_inline int planStatePackerBufSize(const plan_state_packer_t *p);

/**
 * Pack the given state into provided buffer that must be at least
 * planStatePackerBufSize(p) long.
 */
void planStatePackerPack(const plan_state_packer_t *p,
                         const plan_state_t *state,
                         void *buffer);

/**
 * Unpacks the given packed state into plan_state_t state structure.
 */
void planStatePackerUnpack(const plan_state_packer_t *p,
                           const void *buffer,
                           plan_state_t *state);

/**
 * Packs part-state into value and mask buffer stored within
 * plan_part_state_t structure.
 */
void planStatePackerPackPartState(const plan_state_packer_t *p,
                                  plan_part_state_t *part_state);

/**
 * Just alternative call of planStatePackerPackPartState().
 */
_bor_inline void planPartStatePack(plan_part_state_t *ps,
                                   const plan_state_packer_t *p);


/**
 * Returns buffer size needed for public part of the packed state.
 */
_bor_inline int planStatePackerBufSizePubPart(const plan_state_packer_t *p);

/**
 * Extracts public part of the packed state from bufstate and stores it
 * into pub_buffer output buffer.
 */
void planStatePackerExtractPubPart(const plan_state_packer_t *p,
                                   const void *bufstate,
                                   void *pub_buffer);

/**
 * Sets public part of the bufstate to be same as pub_buffer.
 */
void planStatePackerSetPubPart(const plan_state_packer_t *p,
                               const void *pub_buffer,
                               void *bufstate);

/**
 * Returns buffer size needed for the private part of the packed state.
 */
_bor_inline int planStatePackerBufSizePrivatePart(const plan_state_packer_t *p);

/**
 * Extracts private part of the packed state from bufstate and stores it
 * into private_buffer output buffer.
 */
void planStatePackerExtractPrivatePart(const plan_state_packer_t *p,
                                       const void *bufstate,
                                       void *private_buffer);

/**
 * Sets private part of the bufstate to be same as private_buffer.
 */
void planStatePackerSetPrivatePart(const plan_state_packer_t *p,
                                   const void *private_buffer,
                                   void *bufstate);

/**
 * Sets value of the ma-privacy variable directly into packed state.
 */
void planStatePackerSetMAPrivacyVar(const plan_state_packer_t *p,
                                    plan_val_t val,
                                    void *buf);
/**
 * Returns value of the ma-privacy variable directly from the packed state.
 */
plan_val_t planStatePackerGetMAPrivacyVar(const plan_state_packer_t *p,
                                          const void *buf);

/**** INLINES ****/
_bor_inline int planStatePackerBufSize(const plan_state_packer_t *p)
{
    return p->bufsize;
}

_bor_inline void planPartStatePack(plan_part_state_t *ps,
                                   const plan_state_packer_t *p)
{
    planStatePackerPackPartState(p, ps);
}

_bor_inline int planStatePackerBufSizePubPart(const plan_state_packer_t *p)
{
    return p->pub_bufsize;
}

_bor_inline int planStatePackerBufSizePrivatePart(const plan_state_packer_t *p)
{
    return p->private_bufsize;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_STATE_PACKER_H__ */
