#ifndef __PLAN_MA_PRIVATE_STATE_H__
#define __PLAN_MA_PRIVATE_STATE_H__

#include <boruvka/htable.h>

#include <plan/common.h>
#include <plan/dataarr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_ma_private_state_t {
    int num_agents;
    int agent_id;
    plan_data_arr_t *states;
    int num_states;
    size_t state_size;
    bor_htable_t *htable;
};
typedef struct _plan_ma_private_state_t plan_ma_private_state_t;

/**
 * Initializes registry for private state IDs of other agents.
 * num_agents is number of agents in the cluster, agent_id is the ID of the
 * current agent which is not stored.
 */
void planMAPrivateStateInit(plan_ma_private_state_t *aps,
                            int num_agents, int agent_id);

/**
 * Frees allocated resources.
 */
void planMAPrivateStateFree(plan_ma_private_state_t *aps);

/**
 * Inserts a new array of private state IDs if not already inserted.
 * The state_ids argument must have exactly aps->num_agents elements,
 * aps->agent_id's value is ignored.
 */
int planMAPrivateStateInsert(plan_ma_private_state_t *aps, int *state_ids);

/**
 * Returns (via state_ids) an array of private state IDs corresponding to
 * the given ID.
 * Note that aps->agent_id's value is always -1 because it is not stored.
 */
void planMAPrivateStateGet(const plan_ma_private_state_t *aps, int id,
                           int *state_ids);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MA_PRIVATE_STATE_H__ */
