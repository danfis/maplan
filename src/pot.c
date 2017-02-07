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

static plan_lp_t *lpNew(const plan_pot_prob_t *prob)
{
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
                   prob->var_size, lp_flags);

    // Set all variables as free
    for (i = 0; i < prob->var_size; ++i)
        planLPSetVarRange(lp, i, -1E30, 1E7);

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

static int gcd(int x, int y)
{
    if (x == 0)
        return y;

    while (y != 0){
        if (x > y) {
            x = x - y;
        }else{
            y = y - x;
        }
    }

    return x;
}

static int lcm(int x, int y)
{
    return (x * y) / gcd(x, y);
}

static void probInitAllSyntacticStatesRange(plan_pot_prob_t *prob, int R,
                                            const plan_pot_t *pot)
{
    int i, j, c, r;

    for (i = 0; i < pot->var_size; ++i){
        if (i == pot->ma_privacy_var)
            continue;

        r = R / pot->var[i].range;
        for (j = 0; j < pot->var[i].range; ++j){
            c = pot->var[i].lp_var_id[j];
            prob->state_coef[c] = r;
        }
    }
}

static void probInitAllSyntacticStates(plan_pot_prob_t *prob,
                                       const plan_pot_t *pot)
{
    int i, R;

    R = 1;
    for (i = 0; i < pot->var_size; ++i){
        if (i == pot->ma_privacy_var)
            continue;

        R = lcm(R, pot->var[i].range);
    }

    probInitAllSyntacticStatesRange(prob, R, pot);
}

void planPotProbSetAllSyntacticStatesFromFactRange(plan_pot_prob_t *prob,
                                                   const int *fact_range,
                                                   int fact_range_size)
{
    int i, R, r;

    R = 1;
    for (i = 0; i < fact_range_size; ++i){
        if (fact_range[i] > 0)
            R = lcm(R, fact_range[i]);
    }

    for (i = 0; i < fact_range_size; ++i){
        if (fact_range[i] > 0){
            r = R / fact_range[i];
            prob->state_coef[i] = r;
        }
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

    if (heur_flags & PLAN_HEUR_POT_ALL_SYNTACTIC_STATES){
        probInitAllSyntacticStates(prob, pot);
    }else if (state != NULL){
        probInitState(prob, pot, state);
    }

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

void planPotConstrPrint(const plan_pot_constr_t *c, FILE *fout)
{
    int i;
    fprintf(fout, "C: RHS = %d, op_id = %d\n", c->rhs, c->op_id);
    for (i = 0; i < c->coef_size; ++i){
        fprintf(fout, "    [%d] var_id: %d, coef: %d\n",
                i, c->var_id[i], c->coef[i]);
    }
}

void planPotProbPrint(const plan_pot_prob_t *prob, FILE *fout)
{
    int i;

    fprintf(fout, "var_size: %d\n", prob->var_size);
    fprintf(fout, "goal:\n");
    planPotConstrPrint(&prob->goal, fout);
    fprintf(fout, "op:\n");
    for (i = 0; i < prob->op_size; ++i)
        planPotConstrPrint(prob->op + i, fout);
    fprintf(fout, "maxpot:\n");
    for (i = 0; i < prob->maxpot_size; ++i)
        planPotConstrPrint(prob->maxpot + i, fout);

    fprintf(fout, "State:");
    for (i = 0; i < prob->var_size; ++i)
        fprintf(fout, " %d:%d", i, prob->state_coef[i]);
    fprintf(fout, "\n");
    fprintf(fout, "LP-flags: %x\n", prob->lp_flags);
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
                 unsigned heur_flags,
                 unsigned pot_flags)
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

    // Consider private variables only in a case of ma-privacy
    for (i = 0; pot->ma_privacy_var == -1 && i < var_size; ++i)
        pot->var[i].is_private = 0;

    // Determine which max-pot variables we need.
    // First from the goal.
    determineMaxpotFromGoal(pot, goal);

    // Then from the missing preconditions of operators
    determineMaxpotFromOps(pot, op, op_size);

    // In case of multi-agent mode, add maxpot to all public variables,
    // because we cannot know whether they are set by other agents or not
    //  -- so add them everywhere (this can make LP program unnecessary
    //  bigger but still correct).
    if (pot_flags & PLAN_POT_MA){
        for (i = 0; i < var_size; ++i){
            if (i != pot->ma_privacy_var && !pot->var[i].is_private)
                pot->var[i].lp_max_var_id = 0;
        }
    }

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

void planPotCompute2(const plan_pot_prob_t *prob, double *pot)
{
    int i;
    plan_lp_t *lp;

    lp = lpNew(prob);

    // First zeroize potentials
    for (i = 0; i < prob->var_size; ++i)
        pot[i] = 0.;

    // Then set simple objective
    for (i = 0; i < prob->var_size; ++i)
        planLPSetObj(lp, i, prob->state_coef[i]);

    if (planLPSolve(lp, NULL, pot) != 0){
        fprintf(stderr, "Error: LP has no solution!\n");
        exit(-1);
    }
    planLPDel(lp);
}

void planPotCompute(plan_pot_t *pot)
{
    const plan_pot_prob_t *prob = &pot->prob;

    if (pot->pot == NULL)
        pot->pot = BOR_ALLOC_ARR(double, prob->var_size);

    planPotCompute2(prob, pot->pot);
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

void planPotSetAllSyntacticStatesRange(plan_pot_t *pot, int fact_range)
{
    probInitAllSyntacticStatesRange(&pot->prob, fact_range, pot);
}


static void submatrixInit(plan_pot_submatrix_t *s, int cols, int rows)
{
    s->cols = cols;
    s->rows = rows;
    if (cols * rows > 0)
        s->coef = BOR_CALLOC_ARR(int, cols * rows);
}

static void submatrixAdd(plan_pot_submatrix_t *s, int c, int r, int coef)
{
    s->coef[s->cols * r + c] = coef;
}

static void submatrixFree(plan_pot_submatrix_t *sm)
{
    if (sm->coef != NULL)
        BOR_FREE(sm->coef);
}

static void potAgentSetOps(const plan_pot_t *pot, int var_size, int offset,
                           plan_pot_agent_t *agent_pot)
{
    const plan_pot_constr_t *cstr;
    int i, j, private;

    private = 1;
    if (pot->lp_var_size == pot->lp_var_private)
        private = 0;

    // First set up number of rows and columns for operators
    submatrixInit(&agent_pot->pub_op,
                  pot->lp_var_private, pot->prob.op_size);
    submatrixInit(&agent_pot->priv_op, private * var_size, pot->prob.op_size);
    agent_pot->op_cost = BOR_CALLOC_ARR(int, agent_pot->pub_op.rows);

    // Set sparse matrices and RHS (operator costs)
    for (i = 0; i < pot->prob.op_size; ++i){
        cstr = pot->prob.op + i;
        for (j = 0; j < cstr->coef_size; ++j){
            if (cstr->var_id[j] < pot->lp_var_private){
                submatrixAdd(&agent_pot->pub_op, cstr->var_id[j], i,
                             cstr->coef[j]);
            }else if (private){
                submatrixAdd(&agent_pot->priv_op,
                             cstr->var_id[j] - pot->lp_var_private + offset, i,
                             cstr->coef[j]);
            }
        }

        agent_pot->op_cost[i] = cstr->rhs;
    }
}

static void potAgentSetMaxpot(const plan_pot_t *pot, int var_size, int offset,
                              plan_pot_agent_t *agent_pot)
{
    const plan_pot_constr_t *cstr;
    int *private, private_size, use_private, i, j;

    if (pot->lp_var_size == pot->lp_var_private)
        return;

    private = BOR_ALLOC_ARR(int, pot->prob.maxpot_size);
    private_size = 0;
    for (i = 0; i < pot->prob.maxpot_size; ++i){
        cstr = pot->prob.maxpot + i;
        for (j = 0; j < cstr->coef_size; ++j){
            if (cstr->var_id[j] >= pot->lp_var_private){
                private[private_size++] = i;
                break;
            }
        }
    }

    use_private = 1;
    if (private_size == 0)
        use_private = 0;

    submatrixInit(&agent_pot->maxpot, use_private * var_size, private_size);
    for (i = 0; i < private_size; ++i){
        cstr = pot->prob.maxpot + private[i];
        for (j = 0; j < cstr->coef_size; ++j){
            if (cstr->var_id[j] >= pot->lp_var_private){
                submatrixAdd(&agent_pot->maxpot,
                             cstr->var_id[j] - pot->lp_var_private + offset, i,
                             cstr->coef[j]);
            }
        }
    }

    BOR_FREE(private);
}

static void potAgentSetGoal(const plan_pot_t *pot, int var_size, int offset,
                            plan_pot_agent_t *agent_pot)
{
    const plan_pot_constr_t *goal;
    int i, id;

    goal = &pot->prob.goal;
    agent_pot->goal = BOR_CALLOC_ARR(int, var_size);
    for (i = 0; i < goal->coef_size; ++i){
        if (goal->var_id[i] >= pot->lp_var_private){
            id = goal->var_id[i] - pot->lp_var_private + offset;
            agent_pot->goal[id] = goal->coef[i];
        }
    }
}

static void potAgentSetCoefAllSyntStates(const plan_pot_t *pot,
                                         int var_size, int offset,
                                         int range_lcm,
                                         plan_pot_agent_t *agent_pot)
{
    int i, j, c, id;

    agent_pot->coef = BOR_CALLOC_ARR(int, var_size);
    for (i = 0; i < pot->var_size; ++i){
        if (i == pot->ma_privacy_var || !pot->var[i].is_private)
            continue;

        c = range_lcm / pot->var[i].range;
        for (j = 0; j < pot->var[i].range; ++j){
            id = pot->var[i].lp_var_id[j] - pot->lp_var_private + offset;
            agent_pot->coef[id] = c;
        }
    }
}

static void potAgentSetCoefState(const plan_pot_t *pot,
                                 int var_size, int offset,
                                 const plan_state_t *state,
                                 plan_pot_agent_t *agent_pot)
{
    int i, id;

    agent_pot->coef = BOR_CALLOC_ARR(int, var_size);
    for (i = 0; i < pot->var_size; ++i){
        if (i == pot->ma_privacy_var || !pot->var[i].is_private)
            continue;

        id = planStateGet(state, i);
        id = pot->var[i].lp_var_id[id] - pot->lp_var_private + offset;
        agent_pot->coef[id] = 1;
    }
}

void planPotAgentInit(const plan_pot_t *pot,
                      int lp_var_size, int var_offset,
                      const plan_state_t *state,
                      int fact_range_lcm,
                      plan_pot_agent_t *agent_pot)
{
    bzero(agent_pot, sizeof(*agent_pot));
    potAgentSetOps(pot, lp_var_size, var_offset, agent_pot);
    potAgentSetMaxpot(pot, lp_var_size, var_offset, agent_pot);
    potAgentSetGoal(pot, lp_var_size, var_offset, agent_pot);

    if (state != NULL){
        potAgentSetCoefState(pot, lp_var_size, var_offset, state, agent_pot);
    }else{
        potAgentSetCoefAllSyntStates(pot, lp_var_size, var_offset,
                                     fact_range_lcm, agent_pot);
    }
}

void planPotAgentFree(plan_pot_agent_t *agent_pot)
{
    submatrixFree(&agent_pot->pub_op);
    submatrixFree(&agent_pot->priv_op);
    submatrixFree(&agent_pot->maxpot);

    if (agent_pot->op_cost != NULL)
        BOR_FREE(agent_pot->op_cost);
    if (agent_pot->goal != NULL)
        BOR_FREE(agent_pot->goal);
    if (agent_pot->coef != NULL)
        BOR_FREE(agent_pot->coef);
}

static void agentPrintSubmatrix(int id, const char *name,
                                const plan_pot_submatrix_t *s,
                                FILE *fout)
{
    int i, j;

    fprintf(fout, "[%d] %s: cols: %d, rows: %d\n", id, name, s->cols,
            s->rows);
    if (s->rows * s->cols > 0)
    for (i = 0; i < s->rows; ++i){
        fprintf(fout, "[%d] %02d:", id, i);
        for (j = 0; j < s->cols; ++j){
            fprintf(fout, " %d", s->coef[i * s->cols + j]);
        }
        fprintf(fout, "\n");
    }
}

void planPotAgentPrint(const plan_pot_agent_t *pa, int id, FILE *fout)
{
    int i;
    fprintf(fout, "[%d]PA:\n", id);

    agentPrintSubmatrix(id, "pub_op", &pa->pub_op, fout);
    agentPrintSubmatrix(id, "priv_op", &pa->priv_op, fout);
    agentPrintSubmatrix(id, "maxpot", &pa->maxpot, fout);

    fprintf(fout, "[%d] op_cost:", id);
    for (i = 0; i < pa->pub_op.rows; ++i)
        fprintf(fout, " %d", pa->op_cost[i]);
    fprintf(fout, "\n");

    fprintf(fout, "[%d] goal:", id);
    for (i = 0; i < pa->priv_op.cols; ++i)
        fprintf(fout, " %d", pa->goal[i]);
    fprintf(fout, "\n");

    fprintf(fout, "[%d] coef:", id);
    for (i = 0; i < pa->priv_op.cols; ++i)
        fprintf(fout, " %d", pa->coef[i]);
    fprintf(fout, "\n");
}

#else /* PLAN_LP */

void planNOPot(void)
{
}
#endif /* PLAN_LP */
