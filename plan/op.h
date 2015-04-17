/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#ifndef __PLAN_OP_H__
#define __PLAN_OP_H__

#include <plan/common.h>
#include <plan/state_pool.h>

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
 * Struct represention one operator.
 */
struct _plan_op_t {
    char *name;                    /*!< String representation of the operator */
    plan_part_state_t *pre;        /*!< Precondition */
    plan_part_state_t *eff;        /*!< Effect */
    plan_op_cond_eff_t *cond_eff;  /*!< Conditional effects */
    int cond_eff_size;             /*!< Number of conditional effects */
    plan_cost_t cost;              /*!< Cost of the operator */

    int global_id;       /*!< Globally unique ID of the operator */
    int owner;           /*!< ID of the owner agent */
    uint64_t ownerarr;   /*!< Bit array of owners of the operator */
    int is_private;      /*!< True if the operator is private for the owner agent */
};
typedef struct _plan_op_t plan_op_t;


/**
 * Initializes an empty operator.
 */
void planOpInit(plan_op_t *op, int var_size);

/**
 * Frees resources allocated within operator.
 */
void planOpFree(plan_op_t *op);

/**
 * Copies operator src to the dst. The operator dst must be already
 * initialized with state-pool object.
 */
void planOpCopy(plan_op_t *dst, const plan_op_t *src);

/**
 * Copies only private values from src to dst.
 */
void planOpCopyPrivate(plan_op_t *dst, const plan_op_t *src,
                       const plan_var_t *var);

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
plan_state_id_t planOpApply(const plan_op_t *op,
                            plan_state_pool_t *state_pool,
                            plan_state_id_t state_id);

/**
 * Adds the agent to the owner list
 */
void planOpAddOwner(plan_op_t *op, int agent_id);

/**
 * Returns true if the agent is an owner of the operator.
 */
int planOpIsOwner(const plan_op_t *op, int agent_id);

/**
 * Copies first set owner from .ownerarr to .owner.
 */
void planOpSetFirstOwner(plan_op_t *op);

/**
 * Pack all part-states.
 */
void planOpPack(plan_op_t *op, plan_state_packer_t *packer);

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

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_OPERATOR_H__ */
