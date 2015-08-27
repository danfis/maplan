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

#ifndef __PLAN_POT_H__
#define __PLAN_POT_H__

#include <plan/lp.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef PLAN_LP

struct _plan_pot_constr_t {
    int *var_id;    /*!< Non-zer variable IDs */
    int *coef;      /*!< Non-zero coeficient (1 or -1) */
    int coef_size;  /*!< Number of non-zero coeficients */
    int rhs;        /*!< Right hand side of the constraint */
    int op_id;      /*!< Corresponding operator ID */
};
typedef struct _plan_pot_constr_t plan_pot_constr_t;

struct _plan_pot_prob_t {
    plan_pot_constr_t goal;    /*!< Goal constraint */
    plan_pot_constr_t *op;     /*!< Operators constriants */
    int op_size;               /*!< Number of operators */
    plan_pot_constr_t *maxpot; /*!< Max-pot constraints */
    int maxpot_size;           /*!< Number of max-pot constraints */
    unsigned lp_flags;         /*!< Prepared LP flags */
};
typedef struct _plan_pot_prob_t plan_pot_prob_t;

struct _plan_pot_var_t {
    int *lp_var_id;    /*!< LP-variable ID for each variable value */
    int range;         /*!< Number of possible values */
    int lp_max_var_id; /*!< ID of variable corresponding to the max-pot */
    int is_private;    /*!< True if the variable is private */
};
typedef struct _plan_pot_var_t plan_pot_var_t;

struct _plan_pot_t {
    plan_pot_var_t *var; /*!< Description of each variable */
    int var_size;        /*!< Number of variables */
    int lp_var_size;     /*!< Number of LP variables */
    int lp_var_private;  /*!< First ID of the private LP variables */
    int ma_privacy_var;  /*!< Which variable is ma-privacy */

    plan_pot_prob_t prob; /*!< Saved problem in sparse format */

    double *pot;         /*!< Potential for each lp-variable */
};
typedef struct _plan_pot_t plan_pot_t;

/**
 * Initializes structure.
 */
void planPotInit(plan_pot_t *pot,
                 const plan_var_t *var, int var_size,
                 const plan_part_state_t *goal,
                 const plan_op_t *op, int op_size,
                 unsigned heur_flags);

/**
 * Frees allocated resources.
 */
void planPotFree(plan_pot_t *pot);

/**
 * Compute potentials for the specified state.
 */
void planPotCompute(plan_pot_t *pot, const plan_state_t *state);

/**
 * Convers state to the list of LP variable IDs.
 * Returns number of facts stored in var_ids array.
 */
int planPotToVarIds(const plan_pot_t *pot, const plan_state_t *state,
                    int *var_ids);

/**
 * Returns computed potential for given variable-value pair.
 */
_bor_inline double planPotPot(const plan_pot_t *pot, int var, int val);

/**
 * Returns potential of the state.
 */
_bor_inline double planPotStatePot(const plan_pot_t *pot,
                                   const plan_state_t *state);

/**** INLINES: ****/
_bor_inline double planPotPot(const plan_pot_t *pot, int var, int val)
{
    if (pot->pot == NULL || pot->ma_privacy_var == var)
        return 0.;
    return pot->pot[pot->var[var].lp_var_id[val]];
}

_bor_inline double planPotStatePot(const plan_pot_t *pot,
                                   const plan_state_t *state)
{
    int i;
    double p = 0.;

    for (i = 0; i < pot->var_size; ++i)
        p += planPotPot(pot, i, planStateGet(state, i));
    return p;
}

#else /* PLAN_LP */

void planNOPot(void);
#endif /* PLAN_LP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_POT_H__ */
