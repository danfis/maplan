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

/**
 * Extra data needed for multi-agent public/private operators.
 */
struct _plan_op_ma_op_t {
    unsigned is_private:1;       /*!< True if the operator is private */
    unsigned recv_agent_size:31; /*!< Length of .recv_agent */
    int *recv_agent;             /*!< List of IDs of agents that should
                                      receive results of this operator. */
};
typedef struct _plan_op_ma_op_t plan_op_ma_op_t;

/**
 * Extra data needed for multi-agent projected operators
 */
struct _plan_op_ma_proj_op_t {
    int owner;     /*!< ID of the agent that owns this operator */
    int global_id; /*!< ID of the original operator the projection was made
                        from */
};
typedef struct _plan_op_ma_proj_op_t plan_op_ma_proj_op_t;

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

    int global_id;       /*!< Globally unique ID of the operator */
    int owner;           /*!< ID of the owner agent */
    int is_private;      /*!< True if the operator is private for the owner agent */
    uint64_t recv_agent; /*!< Bit array -- 1 for agent that should receive
                              effects of this operator */

    union {
        plan_op_ma_op_t ma_op;
        plan_op_ma_proj_op_t ma_proj_op;
    } extra;
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
 * The .extra data are not copied at all!
 */
void planOpCopy(plan_op_t *dst, const plan_op_t *src);

/**
 * Sets operator's name.
 */
void planOpSetName(plan_op_t *op, const char *name);

/**
 * Sets operator's cost.
 */
_bor_inline void planOpSetCost(plan_op_t *op, plan_cost_t cost);

/**
 * Sets one precondition variable.
 */
_bor_inline void planOpSetPre(plan_op_t *op, plan_var_id_t var, plan_val_t val);

/**
 * Sets one effect variable.
 */
_bor_inline void planOpSetEff(plan_op_t *op, plan_var_id_t var, plan_val_t val);

/**
 * Adds a new conditional effect and returns its reference ID.
 */
int planOpAddCondEff(plan_op_t *op);

/**
 * Sets a precondition of a conditional effect identified by its ID as
 * returned by planOpAddCondEff().
 */
void planOpCondEffSetPre(plan_op_t *op, int cond_eff,
                         plan_var_id_t var, plan_val_t val);

/**
 * Same as planOpCondEffSetPre() but set effect instead of precondition.
 */
void planOpCondEffSetEff(plan_op_t *op, int cond_eff,
                         plan_var_id_t var, plan_val_t val);

/**
 * Deletes last added conditional effect.
 */
void planOpDelLastCondEff(plan_op_t *op);

/**
 * Simplify conditional effects.
 */
void planOpCondEffSimplify(plan_op_t *op);

/**
 * Applies the operator on the given state and store the resulting state
 * into state pool it has reference to. The ID of the resulting state is
 * returned.
 */
plan_state_id_t planOpApply(const plan_op_t *op, plan_state_id_t state_id);

/**
 * Adds specified agent to the list of receiving agents.
 */
void planOpAddRecvAgent(plan_op_t *op, int agent_id);


/**
 * Returns true if the operator is marked as private.
 */
_bor_inline int planOpExtraMAOpIsPrivate(const plan_op_t *op);

/**
 * Sets operator as private in its extra data.
 */
_bor_inline void planOpExtraMAOpSetPrivate(plan_op_t *op);

/**
 * Returns array of agent IDs that should receive results of this operator.
 */
_bor_inline const int *planOpExtraMAOpRecvAgents(const plan_op_t *op, int *size);

/**
 * Record agent ID as receiving agent of the operator in extra data.
 */
void planOpExtraMAOpAddRecvAgent(plan_op_t *op, int agent_id);

/**
 * Free resources allocated within ma-op extra data.
 */
void planOpExtraMAOpFree(plan_op_t *op);

/**
 * Returns owner of the projected operator.
 */
_bor_inline int planOpExtraMAProjOpOwner(const plan_op_t *op);

/**
 * Sets owner ID in ma-proj-op extra data.
 */
_bor_inline void planOpExtraMAProjOpSetOwner(plan_op_t *op, int owner);

/**
 * Returns global ID associated with the operator.
 */
_bor_inline int planOpExtraMAProjOpGlobalId(const plan_op_t *op);

/**
 * Sets operator's global ID in ma-proj-op extra data.
 */
_bor_inline void planOpExtraMAProjOpSetGlobalId(plan_op_t *op, int id);


/**** INLINES ****/
_bor_inline void planOpSetPre(plan_op_t *op, plan_var_id_t var, plan_val_t val)
{
    planPartStateSet(op->pre, var, val);
}

_bor_inline void planOpSetEff(plan_op_t *op, plan_var_id_t var, plan_val_t val)
{
    planPartStateSet(op->eff, var, val);
}

_bor_inline void planOpSetCost(plan_op_t *op, plan_cost_t cost)
{
    op->cost = cost;
}

_bor_inline int planOpExtraMAOpIsPrivate(const plan_op_t *op)
{
    return op->extra.ma_op.is_private;
}

_bor_inline void planOpExtraMAOpSetPrivate(plan_op_t *op)
{
    op->extra.ma_op.is_private = 1;
}

_bor_inline const int *planOpExtraMAOpRecvAgents(const plan_op_t *op, int *size)
{
    *size = op->extra.ma_op.recv_agent_size;
    return op->extra.ma_op.recv_agent;
}

_bor_inline int planOpExtraMAProjOpOwner(const plan_op_t *op)
{
    return op->extra.ma_proj_op.owner;
}

_bor_inline void planOpExtraMAProjOpSetOwner(plan_op_t *op, int owner)
{
    op->extra.ma_proj_op.owner = owner;
}

_bor_inline int planOpExtraMAProjOpGlobalId(const plan_op_t *op)
{
    return op->extra.ma_proj_op.global_id;
}

_bor_inline void planOpExtraMAProjOpSetGlobalId(plan_op_t *op, int id)
{
    op->extra.ma_proj_op.global_id = id;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_OPERATOR_H__ */
