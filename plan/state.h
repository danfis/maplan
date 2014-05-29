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

/**
 * TODO
 */
#define PLAN_NO_STATE ((plan_state_id_t)-1)


/**
 * Struct implementing packing of states into binary buffers.
 */
struct _plan_state_packer_t {
    size_t num_vars;
    size_t bufsize;
};
typedef struct _plan_state_packer_t plan_state_packer_t;


/**
 * Main struct managing all states and its corresponding informations.
 */
struct _plan_state_pool_t {
    size_t num_vars;        /*!< Num of variables per state */

    plan_state_packer_t *packer;
    plan_data_arr_t **data; /*!< Data arrays */
    size_t data_size;       /*!< Number of data arrays */
    bor_htable_t *htable;   /*!< Hash table for uniqueness of states. */
    size_t num_states;
};
typedef struct _plan_state_pool_t plan_state_pool_t;


/**
 * Struct containing "unpacked" state.
 */
struct _plan_state_t {
    unsigned *val;
    size_t num_vars;
};
typedef struct _plan_state_t plan_state_t;


/**
 * Struct representing partial state.
 */
struct _plan_part_state_t {
    unsigned *val; /*!< Array of unpacked values */
    int *is_set;   /*!< Array of bool flags stating whether the
                        corresponding value is set */
    size_t num_vars;

    void *valbuf;  /*!< Buffer of packed values */
    void *maskbuf; /*!< Buffer of mask for values */
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
 * Inserts a new state into the pool.
 * If the pool already contains the same state nothing is changed and the
 * ID of the already present state is returned.
 */
plan_state_id_t planStatePoolInsert(plan_state_pool_t *pool,
                                    const plan_state_t *state);

/**
 * TODO
 */
plan_state_id_t planStatePoolFind(plan_state_pool_t *pool,
                                  const plan_state_t *state);

/**
 * Fills the given state structure with the values from the state specified
 * by its ID.
 */
void planStatePoolGetState(const plan_state_pool_t *pool,
                           plan_state_id_t sid,
                           plan_state_t *state);




/**
 * Creates a new object for packing states into binary buffers.
 */
plan_state_packer_t *planStatePackerNew(const plan_var_t *var,
                                        size_t var_size);

/**
 * Deletes a state packer object.
 */
void planStatePackerDel(plan_state_packer_t *p);

/**
 * Returns size of the buffer in bytes required for storing packed state.
 */
_bor_inline size_t planStatePackerBufSize(const plan_state_packer_t *p);

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
 * Creates a new struct representing state.
 * Note that by calling this function no state is inserted into pool!
 */
plan_state_t *planStateNew(plan_state_pool_t *pool);

/**
 * Destroyes previously created state struct.
 */
void planStateDel(plan_state_pool_t *pool, plan_state_t *state);

/**
 * Returns value of the specified variable.
 */
_bor_inline unsigned planStateGet(const plan_state_t *state, unsigned var);

/**
 * Sets value of the specified variable.
 */
_bor_inline void planStateSet(plan_state_t *state,
                              unsigned var, unsigned val);

/**
 * Zeroize all variable values.
 */
void planStateZeroize(plan_state_t *state);




/**
 * Creates a partial state.
 */
plan_part_state_t *planPartStateNew(plan_state_pool_t *pool);

/**
 * Destroys a partial state.
 */
void planPartStateDel(plan_state_pool_t *pool,
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



/**** INLINES ****/
_bor_inline size_t planStatePackerBufSize(const plan_state_packer_t *p)
{
    return p->bufsize;
}


_bor_inline unsigned planStateGet(const plan_state_t *state, unsigned var)
{
    return state->val[var];
}

_bor_inline void planStateSet(plan_state_t *state,
                              unsigned var, unsigned val)
{
    state->val[var] = val;
}

#endif /* __PLAN_STATE_H__ */
