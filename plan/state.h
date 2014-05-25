#ifndef __PLAN_STATE_H__
#define __PLAN_STATE_H__

#include <stdlib.h>
#include <stdio.h>

#include <boruvka/segmarr.h>

#include <plan/var.h>


struct _plan_state_pool_t {
    size_t num_vars;         /*!< Num of variables per state */
    bor_segmarr_t *states;
    size_t allocated;
};
typedef struct _plan_state_pool_t plan_state_pool_t;


struct _plan_state_t {
    unsigned *val;
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
 * Allocates and returns a new state in pool.
 */
plan_state_t planStatePoolNewState(plan_state_pool_t *pool);

/**
 * Allocates and returns a new state in pool.
 */
plan_part_state_t planStatePoolNewPartState(plan_state_pool_t *pool);



/**
 * Returns a value of a specified variable.
 */
int planStateGet(const plan_state_t *state, unsigned var);

/**
 * Sets a value of a specified variable.
 */
void planStateSet(plan_state_t *state, unsigned var, unsigned val);



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
