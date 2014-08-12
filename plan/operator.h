#ifndef __PLAN_OPERATOR_H__
#define __PLAN_OPERATOR_H__

#include <plan/common.h>
#include <plan/state.h>

struct _plan_operator_cond_eff_t {
    plan_part_state_t *pre;
    plan_part_state_t *eff;
};
typedef struct _plan_operator_cond_eff_t plan_operator_cond_eff_t;

struct _plan_operator_t {
    plan_state_pool_t *state_pool;
    plan_part_state_t *pre; /*!< Precondition */
    plan_part_state_t *eff; /*!< Effect */
    plan_operator_cond_eff_t *cond_eff; /*!< Conditional effects */
    int cond_eff_size; /*!< Number of conditional effects */
    plan_cost_t cost;
    char *name;
    int is_private;
    int owner;
    int global_id;
    int *send_peer; /*!< List of IDs of remote peers that are interested in
                         states created by this operator */
    int send_peer_size;
};
typedef struct _plan_operator_t plan_operator_t;


void planOperatorInit(plan_operator_t *op, plan_state_pool_t *state_pool);
void planOperatorFree(plan_operator_t *op);
void planOperatorCopy(plan_operator_t *dst, const plan_operator_t *src);

_bor_inline void planOperatorSetPrivate(plan_operator_t *op);
_bor_inline int planOperatorIsPrivate(const plan_operator_t *op);
_bor_inline void planOperatorSetOwner(plan_operator_t *op, int owner);
_bor_inline int planOperatorOwner(const plan_operator_t *op);
_bor_inline void planOperatorSetGlobalId(plan_operator_t *op, int id);
_bor_inline int planOperatorGlobalId(const plan_operator_t *op);
void planOperatorAddSendPeer(plan_operator_t *op, int peer);

void planOperatorSetPrecondition(plan_operator_t *op,
                                 plan_var_id_t var,
                                 plan_val_t val);
void planOperatorSetEffect(plan_operator_t *op,
                           plan_var_id_t var,
                           plan_val_t val);
void planOperatorSetName(plan_operator_t *op, const char *name);
void planOperatorSetCost(plan_operator_t *op, plan_cost_t cost);

/**
 * Adds a new conditional effect and returns a reference ID.
 */
int planOperatorAddCondEff(plan_operator_t *op);

/**
 * Sets precondition of a conditional effect.
 */
void planOperatorCondEffSetPre(plan_operator_t *op, int cond_eff,
                               plan_var_id_t var, plan_val_t val);

/**
 * Sets effect of a conditional effect.
 */
void planOperatorCondEffSetEff(plan_operator_t *op, int cond_eff,
                               plan_var_id_t var, plan_val_t val);

/**
 * Simplify conditional effects.
 */
void planOperatorCondEffSimplify(plan_operator_t *op);

/**
 * Applies the operator on the given state and store the resulting state
 * into state pool it has reference to. The ID of the resulting state is
 * returned.
 */
plan_state_id_t planOperatorApply(const plan_operator_t *op,
                                  plan_state_id_t state_id);


/**** INLINES: ****/
_bor_inline void planOperatorSetPrivate(plan_operator_t *op)
{
    op->is_private = 1;
}

_bor_inline int planOperatorIsPrivate(const plan_operator_t *op)
{
    return op->is_private;
}

_bor_inline void planOperatorSetOwner(plan_operator_t *op, int owner)
{
    op->owner = owner;
}

_bor_inline int planOperatorOwner(const plan_operator_t *op)
{
    return op->owner;
}

_bor_inline void planOperatorSetGlobalId(plan_operator_t *op, int id)
{
    op->global_id = id;
}

_bor_inline int planOperatorGlobalId(const plan_operator_t *op)
{
    return op->global_id;
}

#endif /* __PLAN_OPERATOR_H__ */
