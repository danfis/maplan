#ifndef __PLAN_STATE_H__
#define __PLAN_STATE_H__

#include <stdlib.h>
#include <stdio.h>

#include <boruvka/segmarr.h>
#include <boruvka/htable.h>

#include <plan/common.h>
#include <plan/var.h>
#include <plan/dataarr.h>

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
 * Main struct managing all states and its corresponding informations.
 */
struct _plan_state_pool_t {
    int num_vars;        /*!< Num of variables per state */

    plan_state_packer_t *packer;
    plan_data_arr_t **data; /*!< Data arrays */
    int data_size;       /*!< Number of data arrays */
    bor_htable_t *htable;   /*!< Hash table for uniqueness of states. */
    size_t num_states;
};
typedef struct _plan_state_pool_t plan_state_pool_t;


/**
 * Struct containing "unpacked" state.
 */
struct _plan_state_t {
    plan_val_t *val;
    int num_vars;
};
typedef struct _plan_state_t plan_state_t;


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
    int num_vars;

    void *valbuf;  /*!< Buffer of packed values */
    void *maskbuf; /*!< Buffer of mask for values */

    plan_part_state_pair_t *vals; /*!< Unrolled values */
    int vals_size;
};
typedef struct _plan_part_state_t plan_part_state_t;



/**
 * Initializes a new state pool.
 */
plan_state_pool_t *planStatePoolNew(const plan_var_t *var, int var_size);

/**
 * Frees previously allocated pool.
 */
void planStatePoolDel(plan_state_pool_t *pool);

/**
 * Reserves a data array with elements of specified size and each element
 * is initialized once it is allocated with using pair {init_fn} and
 * {init_data} (see dataarr.h).
 * The function returns ID by which the data array can be referenced later.
 */
int planStatePoolDataReserve(plan_state_pool_t *pool,
                             size_t element_size,
                             plan_data_arr_el_init_fn init_fn,
                             const void *init_data);

/**
 * Returns an element from data array that corresponds to the specified
 * state.
 */
void *planStatePoolData(plan_state_pool_t *pool,
                        int data_id,
                        plan_state_id_t state_id);

/**
 * Inserts a new state into the pool.
 * If the pool already contains the same state nothing is changed and the
 * ID of the already present state is returned.
 */
plan_state_id_t planStatePoolInsert(plan_state_pool_t *pool,
                                    const plan_state_t *state);

/**
 * Same as planStatePoolInsert() but packed state is accepted as input.
 */
plan_state_id_t planStatePoolInsertPacked(plan_state_pool_t *pool,
                                          const void *packed_state);

/**
 * Returns state ID corresponding to the given state.
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
 * Returns pointer to the internally managed packed state corresponding to
 * the given state ID.
 */
const void *planStatePoolGetPackedState(const plan_state_pool_t*pool,
                                        plan_state_id_t sid);

/**
 * Returns if the given partial state is subset of a state identified by
 * its ID.
 */
int planStatePoolPartStateIsSubset(const plan_state_pool_t *pool,
                                   const plan_part_state_t *part_state,
                                   plan_state_id_t sid);

/**
 * Applies partial state to the state identified by its ID and saves the
 * resulting state into state pool.
 * The ID of the resulting state is returned.
 */
plan_state_id_t planStatePoolApplyPartState(plan_state_pool_t *pool,
                                            const plan_part_state_t *part_state,
                                            plan_state_id_t sid);



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
 * Creates a partial state.
 */
plan_part_state_t *planPartStateNew(plan_state_pool_t *pool);

/**
 * Destroys a partial state.
 */
void planPartStateDel(plan_state_pool_t *pool,
                      plan_part_state_t *part_state);

/**
 * Returns number of variable the state consists of.
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
void planPartStateSet(plan_state_pool_t *pool, plan_part_state_t *state,
                      plan_var_id_t var, plan_val_t val);

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
 * Macro for iterating over "unrolled" set values of partial state.
 */
#define PLAN_PART_STATE_FOR_EACH(part_state, tmpi, var, val) \
    if ((part_state)->vals_size > 0) \
    for ((tmpi) = 0; \
         (tmpi) < (part_state)->vals_size \
            && ((var) = (part_state)->vals[(tmpi)].var, \
                (val) = (part_state)->vals[(tmpi)].val, 1); \
         ++(tmpi))



/**** INLINES ****/
_bor_inline int planStatePackerBufSize(const plan_state_packer_t *p)
{
    return p->bufsize;
}


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
    return state->num_vars;
}

_bor_inline int planPartStateSize(const plan_part_state_t *state)
{
    return state->num_vars;
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

#endif /* __PLAN_STATE_H__ */
