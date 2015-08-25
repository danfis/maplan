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

#include <boruvka/alloc.h>

#include <plan/heur.h>
#include "pot.h"


#ifdef PLAN_LP

static void lpSetOp(plan_lp_t *lp, const plan_pot_t *pot, int row_id,
                    const plan_op_t *op, unsigned flags)
{
    plan_var_id_t var;
    plan_val_t val, pre_val;
    int i, eff_id, pre_id;
    double cost;

    PLAN_PART_STATE_FOR_EACH(op->eff, i, var, val){
        eff_id = pot->var[var].lp_var_id[val];
        pre_val = planPartStateGet(op->pre, var);
        if (pre_val == PLAN_VAL_UNDEFINED){
            pre_id = pot->var[var].lp_max_var_id;
        }else{
            pre_id = pot->var[var].lp_var_id[pre_val];
        }

        if (pre_id < 0 || eff_id < 0){
            fprintf(stderr, "Error: Invalid variable IDs!\n");
            exit(-1);
        }

        planLPSetCoef(lp, row_id, eff_id, -1.);
        planLPSetCoef(lp, row_id, pre_id, 1.);
    }

    cost = op->cost;
    if (flags & PLAN_HEUR_OP_UNIT_COST){
        cost = 1.;
    }else if (flags & PLAN_HEUR_OP_COST_PLUS_ONE){
        cost = op->cost + 1.;
    }
    planLPSetRHS(lp, row_id, cost, 'L');
}

static void lpSetGoal(plan_lp_t *lp, const plan_pot_t *pot, int row_id,
                      const plan_part_state_t *goal)
{
    int var, val, col;

    for (var = 0; var < pot->var_size; ++var){
        if (var == pot->ma_privacy_var)
            continue;

        val = planPartStateGet(goal, var);
        if (val == PLAN_VAL_UNDEFINED){
            col = pot->var[var].lp_max_var_id;
        }else{
            col = pot->var[var].lp_var_id[val];
        }
        planLPSetCoef(lp, row_id, col, 1.);
    }
    planLPSetRHS(lp, row_id, 0., 'L');
}

static void lpSetMaxPot(plan_lp_t *lp, int row_id, int col_id, int maxpot_id)
{
    planLPSetCoef(lp, row_id, col_id, 1.);
    planLPSetCoef(lp, row_id, maxpot_id, -1.);
    planLPSetRHS(lp, row_id, 0., 'L');
}

static plan_lp_t *lpNew(const plan_pot_t *pot,
                        const plan_var_t *var, int var_size,
                        const plan_part_state_t *goal,
                        const plan_op_t *op, int op_size,
                        unsigned heur_flags)
{
    plan_lp_t *lp;
    unsigned lp_flags = 0u;
    int i, j, size, row_id;
    const plan_pot_var_t *pv;

    // Copy cplex-num-threads flags
    lp_flags |= (heur_flags & (0x3fu << 8u));
    // Set maximalization
    lp_flags |= PLAN_LP_MAX;

    // Number of rows is one per operator + one for the goal + (pot <
    // maxpot) constraints.
    // Number of columns was detected before in fact-map.
    size = op_size + 1;
    for (i = 0; i < pot->var_size; ++i){
        if (pot->var[i].lp_max_var_id >= 0)
            size += pot->var[i].range;
    }
    lp = planLPNew(size, pot->lp_var_size, lp_flags);

    // Set all variables as free
    for (i = 0; i < pot->lp_var_size; ++i)
        planLPSetVarFree(lp, i);

    // Set operator constraints
    for (row_id = 0; row_id < op_size; ++row_id)
        lpSetOp(lp, pot, row_id, op + row_id, heur_flags);

    // Set goal constraint
    lpSetGoal(lp, pot, row_id++, goal);

    // Set maxpot constraints
    for (i = 0; i < pot->var_size; ++i){
        pv = pot->var + i;
        if (pv->lp_max_var_id < 0)
            continue;

        for (j = 0; j < pv->range; ++j)
            lpSetMaxPot(lp, row_id++, pv->lp_var_id[j], pv->lp_max_var_id);
    }

    return lp;
}

static void determineMaxpotFromGoal(plan_pot_t *pot,
                                    const plan_part_state_t *goal)
{
    int i;

    for (i = 0; i < pot->var_size; ++i){
        if (pot->ma_privacy_var != i){
            if (!planPartStateIsSet(goal, i))
                pot->var[i].lp_max_var_id = 0;
        }
    }
}

static void determineMaxpotFromOps(plan_pot_t *pot,
                                   const plan_op_t *op, int op_size)
{
    plan_var_id_t op_var;
    int i, j;

    for (i = 0; i < op_size; ++i){
        PLAN_PART_STATE_FOR_EACH_VAR(op[i].eff, j, op_var){
            if (pot->var[op_var].lp_max_var_id >= 0)
                continue;
            if (!planPartStateIsSet(op[i].pre, op_var))
                pot->var[op_var].lp_max_var_id = 0;
        }
    }
}

static void allocLPVarIDs(plan_pot_t *pot, int is_private)

{
    int i, j;

    for (i = 0; i < pot->var_size; ++i){
        if (i == pot->ma_privacy_var)
            continue;
        if (pot->var[i].is_private != is_private)
            continue;

        for (j = 0; j < pot->var[i].range; ++j)
            pot->var[i].lp_var_id[j] = pot->lp_var_size++;
        if (pot->var[i].lp_max_var_id >= 0)
            pot->var[i].lp_max_var_id = pot->lp_var_size++;
    }
}

void planPotInit(plan_pot_t *pot,
                 const plan_var_t *var, int var_size,
                 const plan_part_state_t *goal,
                 const plan_op_t *op, int op_size,
                 unsigned heur_flags)
{
    int i;

    bzero(pot, sizeof(*pot));
    pot->lp_var_size = 0;
    pot->ma_privacy_var = -1;

    // Allocate variable data
    pot->var_size = var_size;
    pot->var = BOR_CALLOC_ARR(plan_pot_var_t, var_size);
    for (i = 0; i < var_size; ++i){
        pot->var[i].range = var[i].range;
        pot->var[i].lp_max_var_id = -1;
        pot->var[i].is_private = var[i].is_private;

        if (var[i].ma_privacy){
            pot->ma_privacy_var = i;
        }else{
            pot->var[i].lp_var_id = BOR_ALLOC_ARR(int, var[i].range);
        }
    }

    // Determine which max-pot variables we need.
    // First from the goal.
    determineMaxpotFromGoal(pot, goal);

    // Then from the missing preconditions of operators
    determineMaxpotFromOps(pot, op, op_size);

    // Now allocate LP variable IDs, first for public vars, then for
    // private ones.
    allocLPVarIDs(pot, 0);
    pot->lp_var_private = pot->lp_var_size;
    allocLPVarIDs(pot, 1);

    // Prepare LP problem
    pot->lp = lpNew(pot, var, var_size, goal, op, op_size, heur_flags);
}

void planPotFree(plan_pot_t *pot)
{
    int i;

    for (i = 0; i < pot->var_size; ++i){
        if (pot->var[i].lp_var_id != NULL)
            BOR_FREE(pot->var[i].lp_var_id);
    }
    if (pot->var != NULL)
        BOR_FREE(pot->var);

    if (pot->lp != NULL)
        planLPDel(pot->lp);
    if (pot->pot != NULL)
        BOR_FREE(pot->pot);
}

void planPotCompute(plan_pot_t *pot, const plan_state_t *state)
{
    int i, col;

    if (pot->pot == NULL)
        pot->pot = BOR_ALLOC_ARR(double, pot->lp_var_size);

    // First zeroize objective
    for (i = 0; i < pot->lp_var_size; ++i){
        pot->pot[i] = 0.;
        planLPSetObj(pot->lp, i, 0.);
    }

    // Then set simple objective
    for (i = 0; i < pot->var_size; ++i){
        if (i == pot->ma_privacy_var)
            continue;

        col = planStateGet(state, i);
        col = pot->var[i].lp_var_id[col];
        planLPSetObj(pot->lp, col, 1.);
    }

    planLPSolve(pot->lp, pot->pot);
}

#else /* PLAN_LP */

void planNOPot(void)
{
}
#endif /* PLAN_LP */
