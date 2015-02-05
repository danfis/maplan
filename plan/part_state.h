#ifndef __PLAN_PART_STATE_H__
#define __PLAN_PART_STATE_H__

#include <plan/common.h>
#include <plan/var.h>
#include <plan/state.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_part_state_pair_t {
    plan_var_id_t var;
    plan_val_t val;
};
typedef struct _plan_part_state_pair_t plan_part_state_pair_t;

/**
 * Struct representing partial state.
 */
struct _plan_part_state_t {
    plan_val_t *val; /*!< Array of unpacked values */
    int *is_set;     /*!< Array of bool flags stating whether the
                          corresponding value is set */
    int size;

    void *valbuf;  /*!< Buffer of packed values */
    void *maskbuf; /*!< Buffer of mask for values */
    int bufsize;   /*!< Size of the buffers */

    plan_part_state_pair_t *vals; /*!< Unrolled values */
    int vals_size;
};
typedef struct _plan_part_state_t plan_part_state_t;


/**
 * Creates a partial state.
 */
plan_part_state_t *planPartStateNew(int size);

/**
 * Destroys a partial state.
 */
void planPartStateDel(plan_part_state_t *part_state);

/**
 * Copies partial state from src to dst.
 */
void planPartStateCopy(plan_part_state_t *dst, const plan_part_state_t *src);

/**
 * Returns number of variables the state consists of.
 */
_bor_inline int planPartStateSize(const plan_part_state_t *state);

/**
 * Returns a value of a specified variable.
 */
_bor_inline plan_val_t planPartStateGet(const plan_part_state_t *state,
                                        plan_var_id_t var);

/**
 * Sets a value of a specified variable.
 */
void planPartStateSet(plan_part_state_t *state,
                      plan_var_id_t var, plan_val_t val);

/**
 * Unset the value of the specified variable.
 */
void planPartStateUnset(plan_part_state_t *state, plan_var_id_t var);

/**
 * Returns true if var's variable is set.
 */
_bor_inline int planPartStateIsSet(const plan_part_state_t *state,
                                   plan_var_id_t var);

/**
 * Converts partial state to a state.
 * All non-set value of partial state will be set to PLAN_VAL_UNDEFINED in
 * the full state.
 */
void planPartStateToState(const plan_part_state_t *part_state,
                          plan_state_t *state);

/**
 * Returns true if part_state1 is subset of part_state2.
 */
int planPartStateIsSubset(const plan_part_state_t *part_state1,
                          const plan_part_state_t *part_state2);

/**
 * Returns true if the partial state is a subset of the packed state.
 * This call requires part_state to be packed before this call.
 */
int planPartStateIsSubsetPackedState(const plan_part_state_t *part_state,
                                     const void *bufstate);

/**
 * Returns true if the given part-states equal.
 */
int planPartStateEq(const plan_part_state_t *part_state1,
                    const plan_part_state_t *part_state2);

/**
 * Macro for iterating over "unrolled" set values of partial state.
 */
#define PLAN_PART_STATE_FOR_EACH(__part_state, __tmpi, __var, __val) \
    if ((__part_state)->vals_size > 0) \
    for ((__tmpi) = 0; \
         (__tmpi) < (__part_state)->vals_size \
            && ((__var) = (__part_state)->vals[(__tmpi)].var, \
                (__val) = (__part_state)->vals[(__tmpi)].val, 1); \
         ++(__tmpi))

#define PLAN_PART_STATE_FOR_EACH_VAR(__part_state, __tmpi, __var) \
    if ((__part_state)->vals_size > 0) \
    for ((__tmpi) = 0; \
         (__tmpi) < (__part_state)->vals_size \
            && ((__var) = (__part_state)->vals[(__tmpi)].var, 1); \
         ++(__tmpi))



/**** INLINES ****/
_bor_inline int planPartStateSize(const plan_part_state_t *state)
{
    return state->size;
}

_bor_inline plan_val_t planPartStateGet(const plan_part_state_t *state,
                                        plan_var_id_t var)
{
    return state->val[var];
}

_bor_inline int planPartStateIsSet(const plan_part_state_t *state,
                                   plan_var_id_t var)
{
    return state->is_set[var];
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PART_STATE_H__ */
