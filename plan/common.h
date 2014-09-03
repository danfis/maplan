#ifndef __PLAN_COMMON_H__
#define __PLAN_COMMON_H__

#include <limits.h>
#include <boruvka/core.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Basic Types
 * ============
 */

/**
 * Type used for storing variable ID.
 */
typedef int plan_var_id_t;

/**
 * Type used for storing value of the variable.
 */
typedef uint32_t plan_val_t;

/**
 * Type for storing cost (cost of operator, heuristic value, ...).
 */
typedef int plan_cost_t;

/**
 * Type of a state ID which is used for reference into state pool.
 */
typedef int plan_state_id_t;

/**
 * Type of one word in buffer of packed variable values.
 * Bear in mind that the word's size must be big enough to store the whole
 * range of the biggest variable.
 */
typedef uint32_t plan_packer_word_t;

/**
 * Constants
 * ==========
 */

/**
 * Undefined variable ID.
 */
#define PLAN_VAR_ID_UNDEFINED ((plan_var_id_t)-1)

/**
 * Undefined (unknown) variable value.
 */
#define PLAN_VAL_UNDEFINED ((plan_val_t)-1)

/**
 * Heuristic value representing dead end (state from which isn't goal
 * reachable).
 */
#define PLAN_HEUR_DEAD_END INT_MAX

/**
 * Maximal value of cost.
 */
#define PLAN_COST_MAX INT_MAX

/**
 * Zero in plan_cost_t type.
 */
#define PLAN_COST_ZERO 0

/**
 * Constant saying that no state was assigned.
 */
#define PLAN_NO_STATE ((plan_state_id_t)-1)

/**
 * Number of bits in packed word
 */
#define PLAN_PACKER_WORD_BITS (8u * sizeof(plan_packer_word_t))

/**
 * Word with only highest bit set (i.e., 0x80000...)
 */
#define PLAN_PACKER_WORD_SET_HI_BIT \
    (((plan_packer_word_t)1u) << (PLAN_PACKER_WORD_BITS - 1u))

/**
 * Word with all bits set (i.e., 0xffff...)
 */
#define PLAN_PACKER_WORD_SET_ALL_BITS ((plan_packer_word_t)-1)

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_COMMON_H__ */
