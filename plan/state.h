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

#ifndef __PLAN_STATE_H__
#define __PLAN_STATE_H__

#include <stdlib.h>
#include <stdio.h>

#include <plan/common.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Struct containing "unpacked" state.
 */
struct _plan_state_t {
    plan_val_t *val;
    int size;
    plan_state_id_t state_id;
};
typedef struct _plan_state_t plan_state_t;

/**
 * Define plan_state_t structure on stack (without malloc). VLA method is
 * used.
 */
#define PLAN_STATE_STACK(name, var_size) \
    plan_val_t name ## __val__[var_size]; \
    plan_state_t name = { name ## __val__, var_size, PLAN_NO_STATE }


/**
 * Creates a new struct representing state.
 * Note that by calling this function no state is inserted into pool!
 */
plan_state_t *planStateNew(int size);

/**
 * Destroyes previously created state struct.
 */
void planStateDel(plan_state_t *state);

/**
 * Initialize state structure.
 */
void planStateInit(plan_state_t *state, int size);

/**
 * Frees state structure.
 */
void planStateFree(plan_state_t *state);

/**
 * Returns value of the specified variable.
 */
_bor_inline plan_val_t planStateGet(const plan_state_t *state,
                                    plan_var_id_t var);

/**
 * Sets value of the specified variable.
 */
_bor_inline void planStateSet(plan_state_t *state,
                              plan_var_id_t var,
                              plan_val_t val);

/**
 * Sets first n variables to specified values.
 */
void planStateSet2(plan_state_t *state, int n, ...);

/**
 * Returns number of variables in state.
 */
_bor_inline int planStateSize(const plan_state_t *state);

/**
 * Zeroize all variable values.
 */
void planStateZeroize(plan_state_t *state);

/**
 * For debug purposes.
 */
void planStatePrint(const plan_state_t *state, FILE *fout);

/**
 * Copy state from src to dst.
 */
void planStateCopy(plan_state_t *dst, const plan_state_t *src);


/**** INLINES ****/
_bor_inline plan_val_t planStateGet(const plan_state_t *state,
                                    plan_var_id_t var)
{
    return state->val[var];
}

_bor_inline void planStateSet(plan_state_t *state,
                              plan_var_id_t var,
                              plan_val_t val)
{
    state->val[var] = val;
}

_bor_inline int planStateSize(const plan_state_t *state)
{
    return state->size;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_STATE_H__ */
