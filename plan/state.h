#ifndef __PLAN_STATE_H__
#define __PLAN_STATE_H__

#include <stdlib.h>
#include <stdio.h>

#include <boruvka/segmarr.h>
#include <boruvka/htable.h>

#include <plan/var.h>
#include <plan/dataarr.h>

/**
 * Type of a state ID which is used for reference into state pool.
 */
typedef long plan_state_id_t;

struct _plan_state_pool_t {
    size_t num_vars;        /*!< Num of variables per state */
    size_t state_size;      /*!< Size of a state in bytes */

    plan_data_arr_t **data; /*!< Data arrays */
    size_t data_size;       /*!< Number of data arrays */
    bor_htable_t *htable;   /*!< Hash table for uniqueness of states. */
    size_t num_states;
};
typedef struct _plan_state_pool_t plan_state_pool_t;

struct _plan_part_state_t {
    unsigned *val;
    unsigned *mask;
};
typedef struct _plan_part_state_t plan_part_state_t;



/**
 * Initializes a new state pool.
 */
plan_state_pool_t *planStatePoolNew(const plan_var_t *var, size_t var_size);

/**
 * Frees previously allocated pool.
 */
void planStatePoolDel(plan_state_pool_t *pool);

/**
 * Creates a new state. The given array {values} must have at least
 * pool->num_vars elements.
 * Uniqueness of the state is not checked!
 * So, this function is suitable basicaly only for creating of an initial
 * state.
 */
plan_state_id_t planStatePoolCreate(plan_state_pool_t *pool,
                                    unsigned *values);

/**
 * Returns value of variable var of the state specified by its id.
 * This function is just for debugging.
 */
unsigned planStatePoolStateVal(const plan_state_pool_t *pool,
                               plan_state_id_t id,
                               unsigned var);




/**
 * Creates a partial state.
 */
plan_part_state_t *planStatePoolCreatePartState(plan_state_pool_t *pool);

/**
 * Destroys a partial state.
 */
void planStatePoolDestroyPartState(plan_state_pool_t *pool,
                                   plan_part_state_t *part_state);

/**
 * Returns a value of a specified variable.
 */
int planPartStateGet(const plan_part_state_t *state, unsigned var);

/**
 * Sets a value of a specified variable.
 */
void planPartStateSet(plan_part_state_t *state, unsigned var, unsigned val);

/**
 * Returns true if var's variable is set.
 */
int planPartStateIsSet(const plan_part_state_t *state, unsigned var);

#endif /* __PLAN_STATE_H__ */
