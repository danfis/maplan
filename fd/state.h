#ifndef __FD_STATE_H__
#define __FD_STATE_H__

#include <stdlib.h>
#include <stdio.h>


struct _fd_state_pool_t {
    size_t num_vars;         /*!< Num of variables per state */
    unsigned *states;        /*!< States stored consecutively one after another */
    size_t states_size;      /*!< Number of states stored */
    size_t states_allocated; /*!< Actual allocated size of .states */
};
typedef struct _fd_state_pool_t fd_state_pool_t;


struct _fd_state_t {
    unsigned *val;
};
typedef struct _fd_state_t fd_state_t;



/**
 * Initializes a new state pool.
 */
fd_state_pool_t *fdStatePoolNew(size_t num_vars);

/**
 * Frees previously allocated pool.
 */
void fdStatePoolDel(fd_state_pool_t *pool);

/**
 * Allocates and returns a new state in pool.
 */
fd_state_t fdStatePoolNewState(fd_state_pool_t *pool);

/**
 * Returns a temporary state that can be used for setting its values and
 * inserting it back to the state pool.
 */
fd_state_t fdStatePoolTmpState(fd_state_pool_t *pool);


/**
 * Returns a value of a specified variable.
 */
int fdStateGet(const fd_state_t *state, unsigned var);

/**
 * Sets a value of a specified variable.
 */
void fdStateSet(fd_state_t *state, unsigned var, unsigned val);

#endif /* __FD_STATE_H__ */
