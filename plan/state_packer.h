#ifndef __PLAN_STATE_PACKER_H__
#define __PLAN_STATE_PACKER_H__

#include <plan/common.h>
#include <plan/state.h>
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


/**** INLINES ****/
_bor_inline int planStatePackerBufSize(const plan_state_packer_t *p)
{
    return p->bufsize;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_STATE_PACKER_H__ */
