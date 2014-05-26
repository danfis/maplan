#ifndef __PLAN_STATE_H__
#define __PLAN_STATE_H__

#include <stdlib.h>
#include <stdio.h>

#include <boruvka/segmarr.h>
#include <boruvka/htable.h>

#include <plan/var.h>


struct _plan_state_pool_t {
    size_t num_vars;         /*!< Num of variables per state */
    size_t state_size;
    bor_segmarr_t *states;
    size_t num_states;
};
typedef struct _plan_state_pool_t plan_state_pool_t;


struct _plan_state_t {
    bor_list_t htable;
};
typedef struct _plan_state_t plan_state_t;

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
 * Tries to find an equal state to the one provided.
 * If such a state is found its ID is returned, otherwise -1 is returned.
 */
ssize_t planStatePoolFind(plan_state_pool_t *pool,
                          const plan_state_t *state);

/**
 * Inserts a new state into state pool and returns its ID.
 * If an equal state is already in the state pool, its ID is silently
 * returned.
 */
size_t planStatePoolInsert(plan_state_pool_t *pool,
                           const plan_state_t *state);

/**
 * Returns a state with the corresponding ID or NULL if no such state
 * exists.
 */
const plan_state_t *planStatePoolGet(const plan_state_pool_t *pool, size_t id);

/**
 * Creates a new state outside an internal register.
 * The resulting state can be modified ba planState*() functions.
 */
plan_state_t *planStatePoolCreateState(plan_state_pool_t *pool);

/**
 * Destroys a state previously created by planStatePoolCreateState().
 */
void planStatePoolDestroyState(plan_state_pool_t *pool,
                               plan_state_t *state);

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
int planStateGet(const plan_state_t *state, unsigned var);

/**
 * Sets a value of a specified variable.
 */
void planStateSet(plan_state_t *state, unsigned var, unsigned val);

/**
 * Sets all variables to zero.
 */
void planStateZeroize(const plan_state_pool_t *pool, plan_state_t *state);

/**
 * Returns true if the two states equal.
 */
int planStateEq(const plan_state_pool_t *pool,
                const plan_state_t *s1, const plan_state_t *s2);



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
