#ifndef __PLAN_STATE_H__
#define __PLAN_STATE_H__

#include <stdlib.h>
#include <stdio.h>


struct _plan_state_pool_t {
    size_t num_vars;         /*!< Num of variables per state */
    unsigned *states;        /*!< States stored consecutively one after another */
    size_t states_size;      /*!< Number of states stored */
    size_t states_allocated; /*!< Actual allocated size of .states */
};
typedef struct _plan_state_pool_t plan_state_pool_t;


struct _plan_state_t {
    unsigned *val;
};
typedef struct _plan_state_t plan_state_t;



/**
 * Initializes a new state pool.
 */
plan_state_pool_t *planStatePoolNew(size_t num_vars);

/**
 * Frees previously allocated pool.
 */
void planStatePoolDel(plan_state_pool_t *pool);

/**
 * Allocates and returns a new state in pool.
 */
plan_state_t planStatePoolNewState(plan_state_pool_t *pool);

/**
 * Returns a temporary state that can be used for setting its values and
 * inserting it back to the state pool.
 */
plan_state_t planStatePoolTmpState(plan_state_pool_t *pool);


/**
 * Returns a value of a specified variable.
 */
int planStateGet(const plan_state_t *state, unsigned var);

/**
 * Sets a value of a specified variable.
 */
void planStateSet(plan_state_t *state, unsigned var, unsigned val);

#endif /* __PLAN_STATE_H__ */
