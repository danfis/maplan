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

#include "plan/heur.h"
#include "plan/pot.h"


#ifdef PLAN_LP

static void lpSetConstr(plan_lp_t *lp, int row_id,
                        const plan_pot_constr_t *constr)
{
    int i;

    for (i = 0; i < constr->coef_size; ++i)
        planLPSetCoef(lp, row_id, constr->var_id[i], constr->coef[i]);
    planLPSetRHS(lp, row_id, constr->rhs, 'L');
}

static plan_lp_t *lpNew(const plan_pot_t *pot)
{
    const plan_pot_prob_t *prob = &pot->prob;
    plan_lp_t *lp;
    unsigned lp_flags = 0u;
    int i, row_id;

    // Copy cplex-num-threads flags
    lp_flags |= prob->lp_flags;
    // Set maximalization
    lp_flags |= PLAN_LP_MAX;

    // Number of rows is one per operator + one for the goal + (pot <
    // maxpot) constraints.
    // Number of columns was detected before in fact-map.
    lp = planLPNew(prob->op_size + prob->maxpot_size + 1,
                   pot->lp_var_size, lp_flags);

    // Set all variables as free
    for (i = 0; i < pot->lp_var_size; ++i)
        planLPSetVarFree(lp, i);

    // Set operator constraints
    for (row_id = 0; row_id < prob->op_size; ++row_id)
        lpSetConstr(lp, row_id, prob->op + row_id);

    // Set goal constraint
    lpSetConstr(lp, row_id++, &prob->goal);

    // Set maxpot constraints
    for (i = 0; i < prob->maxpot_size; ++i)
        lpSetConstr(lp, row_id++, prob->maxpot + i);

    return lp;
}

static void probInitGoal(plan_pot_constr_t *constr,
                         const plan_pot_t *pot,
                         const plan_part_state_t *goal)
{
    int var, val, col;

    constr->coef = BOR_ALLOC_ARR(int, pot->var_size);
    constr->var_id = BOR_ALLOC_ARR(int, pot->var_size);
    constr->coef_size = 0;
    constr->rhs = 0;
    constr->op_id = -1;

    for (var = 0; var < pot->var_size; ++var){
        if (var == pot->ma_privacy_var)
            continue;

        val = planPartStateGet(goal, var);
        if (val == PLAN_VAL_UNDEFINED){
            col = pot->var[var].lp_max_var_id;
        }else{
            col = pot->var[var].lp_var_id[val];
        }

        constr->coef[constr->coef_size] = 1;
        constr->var_id[constr->coef_size++] = col;
    }
}

static void probInitOp(plan_pot_constr_t *constr,
                       const plan_pot_t *pot,
                       const plan_op_t *op,
                       unsigned heur_flags)
{
    plan_var_id_t var;
    plan_val_t val, pre_val;
    int i, eff_id, pre_id, cost;

    constr->var_id = BOR_ALLOC_ARR(int, 2 * pot->var_size);
    constr->coef = BOR_ALLOC_ARR(int, 2 * pot->var_size);
    constr->coef_size = 0;

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

        constr->coef[constr->coef_size]     = -1;
        constr->var_id[constr->coef_size++] = eff_id;

        constr->coef[constr->coef_size]     = 1;
        constr->var_id[constr->coef_size++] = pre_id;
    }

    cost = op->cost;
    if (heur_flags & PLAN_HEUR_OP_UNIT_COST){
        cost = 1;
    }else if (heur_flags & PLAN_HEUR_OP_COST_PLUS_ONE){
        cost = op->cost + 1;
    }
    constr->rhs = cost;
    constr->op_id = op->global_id;

    if (constr->coef_size < 2 * pot->var_size){
        constr->var_id = BOR_REALLOC_ARR(constr->var_id, int, constr->coef_size);
        constr->coef = BOR_REALLOC_ARR(constr->coef, int, constr->coef_size);
    }
}

static void probInitOps(plan_pot_prob_t *prob,
                        const plan_pot_t *pot,
                        const plan_op_t *op, int op_size,
                        unsigned heur_flags)
{
    int i;

    prob->op_size = op_size;
    prob->op = BOR_CALLOC_ARR(plan_pot_constr_t, op_size);
    for (i = 0; i < op_size; ++i)
        probInitOp(prob->op + i, pot, op + i, heur_flags);
}

static int probInitMaxpot1(plan_pot_prob_t *prob,
                           const plan_pot_t *pot, int var_id, int ins)
{
    const plan_pot_var_t *pv;
    plan_pot_constr_t *c;
    int i;

    pv = pot->var + var_id;
    for (i = 0; i < pv->range; ++i){
        c = prob->maxpot + ins++;

        c->var_id = BOR_ALLOC_ARR(int, 2);
        c->coef = BOR_ALLOC_ARR(int, 2);
        c->coef_size = 2;
        c->var_id[0] = pv->lp_var_id[i];
        c->coef[0] = 1;
        c->var_id[1] = pv->lp_max_var_id;
        c->coef[1] = -1;

        c->rhs = 0;
        c->op_id = -1;
    }

    return ins;
}

static void probInitMaxpot(plan_pot_prob_t *prob,
                           const plan_pot_t *pot)
{
    int i, size, ins;

    for (size = 0, i = 0; i < pot->var_size; ++i){
        if (pot->var[i].lp_max_var_id >= 0)
            size += pot->var[i].range;
    }

    prob->maxpot_size = size;
    prob->maxpot = BOR_CALLOC_ARR(plan_pot_constr_t, size);
    for (ins = 0, i = 0; i < pot->var_size; ++i){
        if (pot->var[i].lp_max_var_id >= 0)
            ins = probInitMaxpot1(prob, pot, i, ins);
    }
}

static void probInitState(plan_pot_prob_t *prob,
                          const plan_pot_t *pot,
                          const plan_state_t *state)
{
    int i, c;

    for (i = 0; i < pot->var_size; ++i){
        if (i == pot->ma_privacy_var)
            continue;

        c = planStateGet(state, i);
        c = pot->var[i].lp_var_id[c];
        prob->state_coef[c] = 1;
    }
}

static void probInit(plan_pot_prob_t *prob,
                     const plan_pot_t *pot,
                     const plan_part_state_t *goal,
                     const plan_op_t *op, int op_size,
                     const plan_state_t *state,
                     unsigned heur_flags)
{
    bzero(prob, sizeof(*prob));
    prob->var_size = pot->lp_var_size;
    probInitGoal(&prob->goal, pot, goal);
    probInitOps(prob, pot, op, op_size, heur_flags);
    probInitMaxpot(prob, pot);
    prob->state_coef = BOR_CALLOC_ARR(int, prob->var_size);
    if (state != NULL)
        probInitState(prob, pot, state);
    prob->lp_flags  = 0;
    prob->lp_flags |= (heur_flags & (0x3fu << 8u));
}

static void probConstrFree(plan_pot_constr_t *constr)
{
    if (constr->var_id != NULL)
        BOR_FREE(constr->var_id);
    if (constr->coef != NULL)
        BOR_FREE(constr->coef);
}

void planPotProbFree(plan_pot_prob_t *prob)
{
    int i;

    probConstrFree(&prob->goal);

    for (i = 0; i < prob->op_size; ++i)
        probConstrFree(prob->op + i);
    if (prob->op != NULL)
        BOR_FREE(prob->op);

    for (i = 0; i < prob->maxpot_size; ++i)
        probConstrFree(prob->maxpot + i);
    if (prob->maxpot != NULL)
        BOR_FREE(prob->maxpot);

    if (prob->state_coef)
        BOR_FREE(prob->state_coef);
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
                 const plan_state_t *state,
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

    // Construct problem
    probInit(&pot->prob, pot, goal, op, op_size, state, heur_flags);
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

    planPotProbFree(&pot->prob);
    if (pot->pot != NULL)
        BOR_FREE(pot->pot);
}

void planPotCompute(plan_pot_t *pot)
{
    int i;
    plan_lp_t *lp;

    if (pot->pot == NULL)
        pot->pot = BOR_ALLOC_ARR(double, pot->lp_var_size);

    lp = lpNew(pot);

    // First zeroize potentials
    for (i = 0; i < pot->lp_var_size; ++i)
        pot->pot[i] = 0.;

    // Then set simple objective
    for (i = 0; i < pot->prob.var_size; ++i)
        planLPSetObj(lp, i, pot->prob.state_coef[i]);

    planLPSolve(lp, pot->pot);
    planLPDel(lp);
}

int planPotToVarIds(const plan_pot_t *pot, const plan_state_t *state,
                    int *var_ids)
{
    int i, size = 0, v;

    for (i = 0; i < pot->var_size; ++i){
        if (i == pot->ma_privacy_var)
            continue;

        v = planStateGet(state, i);
        v = pot->var[i].lp_var_id[v];
        var_ids[size++] = v;
    }

    return size;
}

#else /* PLAN_LP */

void planNOPot(void)
{
}
#endif /* PLAN_LP */
