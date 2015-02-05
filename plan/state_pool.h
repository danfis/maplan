#ifndef __PLAN_STATE_POOL_H__
#define __PLAN_STATE_POOL_H__

#include <boruvka/segmarr.h>
#include <boruvka/htable.h>

#include <plan/common.h>
#include <plan/var.h>
#include <plan/dataarr.h>
#include <plan/state.h>
#include <plan/part_state.h>
#include <plan/state_packer.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
 * Returns true if the given partial state is subset of a state identified by
 * its ID.
 */
int planStatePoolPartStateIsSubset(const plan_state_pool_t *pool,
                                   const plan_part_state_t *part_state,
                                   plan_state_id_t sid);

/**
 * Applies the partial state to the state identified by its ID and saves
 * the resulting state into state pool.
 * The ID of the resulting state is returned.
 */
_bor_inline plan_state_id_t planStatePoolApplyPartState(plan_state_pool_t *pool,
                                                        const plan_part_state_t *part_state,
                                                        plan_state_id_t sid);

/**
 * Same as planStatePoolApplyPartState() but uses "unrolled" mask and value
 * buffers from part-state directly.
 */
plan_state_id_t planStatePoolApplyPartState2(plan_state_pool_t *pool,
                                            const void *maskbuf,
                                            const void *valbuf,
                                            plan_state_id_t sid);


/**** INLINES ****/
_bor_inline plan_state_id_t planStatePoolApplyPartState(plan_state_pool_t *pool,
                                                        const plan_part_state_t *part_state,
                                                        plan_state_id_t sid)
{
    return planStatePoolApplyPartState2(pool, part_state->maskbuf,
                                        part_state->valbuf, sid);
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_STATE_POOL_H__ */
