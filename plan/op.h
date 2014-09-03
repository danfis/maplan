#ifndef __PLAN_OP_H__
#define __PLAN_OP_H__

#include <plan/common.h>
#include <plan/state.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Conditional effect of an operator.
 */
struct _plan_op_cond_eff_t {
    plan_part_state_t *pre;
    plan_part_state_t *eff;
};
typedef struct _plan_op_cond_eff_t plan_op_cond_eff_t;

struct _plan_op_data_t {
    int is_private;
    int owner;
    int global_id;
    int *send_peer; /*!< List of IDs of remote peers that are interested in
                         states created by this operator */
    int send_peer_size;
};

/**
 * Struct represention one operator.
 */
struct _plan_op_t {
    char *name;                    /*!< String representation of the operator */
    plan_part_state_t *pre;        /*!< Precondition */
    plan_part_state_t *eff;        /*!< Effect */
    plan_op_cond_eff_t *cond_eff;  /*!< Conditional effects */
    int cond_eff_size;             /*!< Number of conditional effects */
    plan_cost_t cost;              /*!< Cost of the operator */
    plan_state_pool_t *state_pool; /*!< Reference to the state-pool this
                                        operator is made from */
};
typedef struct _plan_op_t plan_op_t;


/**
 * Initializes an empty operator.
 */
void planOpInit(plan_op_t *op, plan_state_pool_t *state_pool);

/**
 * Frees resources allocated within operator.
 */
void planOpFree(plan_op_t *op);

/**
 * Copies operator src to the dst. The operator dst must be already
 * initialized with state-pool object.
 */
void planOpCopy(plan_op_t *dst, const plan_op_t *src);

#if 0
void planOpSetPrecondition(plan_op_t *op,
                                 plan_var_id_t var,
                                 plan_val_t val);
void planOpSetEffect(plan_op_t *op,
                           plan_var_id_t var,
                           plan_val_t val);
void planOpSetName(plan_op_t *op, const char *name);
void planOpSetCost(plan_op_t *op, plan_cost_t cost);

/**
 * Adds a new conditional effect and returns a reference ID.
 */
int planOpAddCondEff(plan_op_t *op);
void planOpDelLastCondEff(plan_op_t *op);

/**
 * Sets precondition of a conditional effect.
 */
void planOpCondEffSetPre(plan_op_t *op, int cond_eff,
                               plan_var_id_t var, plan_val_t val);

/**
 * Sets effect of a conditional effect.
 */
void planOpCondEffSetEff(plan_op_t *op, int cond_eff,
                               plan_var_id_t var, plan_val_t val);

/**
 * Simplify conditional effects.
 */
void planOpCondEffSimplify(plan_op_t *op);

/**
 * Applies the operator on the given state and store the resulting state
 * into state pool it has reference to. The ID of the resulting state is
 * returned.
 */
plan_state_id_t planOpApply(const plan_op_t *op,
                                  plan_state_id_t state_id);



_bor_inline void planOpSetPrivate(plan_op_t *op);
_bor_inline int planOpIsPrivate(const plan_op_t *op);
_bor_inline void planOpSetOwner(plan_op_t *op, int owner);
_bor_inline int planOpOwner(const plan_op_t *op);
_bor_inline void planOpSetGlobalId(plan_op_t *op, int id);
_bor_inline int planOpGlobalId(const plan_op_t *op);
void planOpAddSendPeer(plan_op_t *op, int peer);

/**** INLINES: ****/
_bor_inline void planOpSetPrivate(plan_op_t *op)
{
    op->is_private = 1;
}

_bor_inline int planOpIsPrivate(const plan_op_t *op)
{
    return op->is_private;
}

_bor_inline void planOpSetOwner(plan_op_t *op, int owner)
{
    op->owner = owner;
}

_bor_inline int planOpOwner(const plan_op_t *op)
{
    return op->owner;
}

_bor_inline void planOpSetGlobalId(plan_op_t *op, int id)
{
    op->global_id = id;
}

_bor_inline int planOpGlobalId(const plan_op_t *op)
{
    return op->global_id;
}
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_OPERATOR_H__ */
