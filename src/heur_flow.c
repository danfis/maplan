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

#include "fact_id.h"

#define BOUND_INF 1E30

/** Upper bound table:
 * [is_goal_var][is_mutex_with_goal][is_init][cause_incomplete_op] */
static double upper_bound_table[2][2][2][2] = {
// .is_goal_var                      0
// .is_mutex_with_goal               0
// .is_init:                 0                  1
// .cause_incomplete_op   0     1            0     1
                    { { { 1., BOUND_INF }, { 0., BOUND_INF } },

// .is_goal_var                      0
// .is_mutex_with_goal               1
// .is_init:                 0                  1
// .cause_incomplete_op   0     1            0     1
                      { { 0., BOUND_INF }, { -1., BOUND_INF } } },

// .is_goal_var                      1
// .is_mutex_with_goal               0
// .is_init:                 0                  1
// .cause_incomplete_op   0     1            0     1
                    { { { 1., BOUND_INF }, { 0., BOUND_INF } },
                          

// .is_goal_var                      1
// .is_mutex_with_goal               1
// .is_init:                 0                  1
// .cause_incomplete_op   0     1            0     1
                      { { 0., BOUND_INF }, { -1., BOUND_INF } } },
};

/** Lower bound table:
 * [is_goal_var][is_mutex_with_goal][is_init] */
static double lower_bound_table[2][2][2] = {
// .is_goal_var                  0
// .is_mutex_with_goal    0      |      1
// .is_init:            0    1   |   0    1
                    { { 0., -1. }, { 0., -1. } },

// .is_goal_var                  1
// .is_mutex_with_goal    0      |      1
// .is_init:            0    1   |   0    1
                    { { 1., 0. }, { 0., -1. } },
};

struct _fact_t {
    // Precomputed data that does not ever change:
    int is_mutex_with_goal; /*!< True if the fact is mutex with the goal */
    int is_goal;            /*!< True if the fact is in goal */
    int is_goal_var;        /*!< True if it is one value of a goal variable */
    int cause_incomplete_op;/*!< True if the fact causes incompleteness of
                                 an operator */
    int *constr_idx;        /*!< Index of the constraint variable */
    double *constr_coef;    /*!< Coeficient of the constraint variable */
    int constr_len;         /*!< Number of constraint variables */

    // These values must be changed according to the state for which is
    // computed heuristic value:
    int is_init;            /*!< True if the fact is in initial state */
    double lower_bound;     /*!< Lower bound on constraint */
    double upper_bound;     /*!< Upper bound on constraint */
};
typedef struct _fact_t fact_t;

/**
 * Main structure for Flow heuristic
 */
struct _plan_heur_flow_t {
    plan_heur_t heur;
    plan_fact_id_t fact_id; /*!< Translation from var-val pair to fact ID */
    fact_t *facts;          /*!< Array of fact related structures */
    double *op_cost;        /*!< Cost of each operator */
    int op_size;            /*!< Number of operators */
};
typedef struct _plan_heur_flow_t plan_heur_flow_t;
#define HEUR(parent) \
    bor_container_of((parent), plan_heur_flow_t, heur)

static void heurFlowDel(plan_heur_t *_heur);
static void heurFlow(plan_heur_t *_heur, const plan_state_t *state,
                     plan_heur_res_t *res);

/** Initialize array of facts */
static void factsInit(fact_t *facts, const plan_fact_id_t *fact_id,
                      const plan_var_t *var, int var_size,
                      const plan_part_state_t *goal,
                      const plan_op_t *op, int op_size);
/** Sets facts values corresponding to the given state (.is_init,
 * lower/upper bounds) */
static void factsSetState(fact_t *facts, const plan_fact_id_t *fact_id,
                          const plan_state_t *state);

plan_heur_t *planHeurFlowNew(const plan_var_t *var, int var_size,
                             const plan_part_state_t *goal,
                             const plan_op_t *op, int op_size)
{
    plan_heur_flow_t *hflow;
    int i;

    hflow = BOR_ALLOC(plan_heur_flow_t);
    _planHeurInit(&hflow->heur, heurFlowDel, heurFlow);

    planFactIdInit(&hflow->fact_id, var, var_size);
    hflow->facts = BOR_CALLOC_ARR(fact_t, hflow->fact_id.fact_size);
    factsInit(hflow->facts, &hflow->fact_id, var, var_size, goal, op, op_size);
    hflow->op_cost = BOR_ALLOC_ARR(double, op_size);
    hflow->op_size = op_size;
    for (i = 0; i < op_size; ++i)
        hflow->op_cost[i] = op->cost;

    return &hflow->heur;
}

static void heurFlowDel(plan_heur_t *_heur)
{
    plan_heur_flow_t *hflow = HEUR(_heur);
    int i;

    if (hflow->op_cost)
        BOR_FREE(hflow->op_cost);
    for (i = 0; hflow->facts && i < hflow->fact_id.fact_size; ++i){
        if (hflow->facts[i].constr_idx)
            BOR_FREE(hflow->facts[i].constr_idx);
        if (hflow->facts[i].constr_coef)
            BOR_FREE(hflow->facts[i].constr_coef);
    }
    if (hflow->facts)
        BOR_FREE(hflow->facts);
    planFactIdFree(&hflow->fact_id);
    _planHeurFree(&hflow->heur);
    BOR_FREE(hflow);
}

static void heurFlow(plan_heur_t *_heur, const plan_state_t *state,
                     plan_heur_res_t *res)
{
    plan_heur_flow_t *hflow = HEUR(_heur);

    factsSetState(hflow->facts, &hflow->fact_id, state);
}

static void factsInitGoal(fact_t *facts, const plan_fact_id_t *fact_id,
                          const plan_part_state_t *goal,
                          const plan_var_t *vars)
{
    plan_var_id_t var;
    plan_val_t val, val2;
    int i, fid;

    PLAN_PART_STATE_FOR_EACH(goal, i, var, val){
        // Set .is_goal flag
        fid = planFactId(fact_id, var, val);
        if (fid >= 0)
            facts[fid].is_goal = 1;

        // Set .is_goal_var and .is_mutex_with_goal flag
        for (val2 = 0; val2 < vars[var].range; ++val2){
            fid = planFactId(fact_id, var, val2);
            if (fid >= 0){
                facts[fid].is_goal_var = 1;
                if (val2 != val)
                    facts[fid].is_mutex_with_goal = 1;
            }
        }
    }

    // TODO: Check mutexes
}

static void factAddConstr(fact_t *fact, int op_id, double coef)
{
    ++fact->constr_len;
    fact->constr_idx = BOR_REALLOC_ARR(fact->constr_idx, int,
                                       fact->constr_len);
    fact->constr_coef = BOR_REALLOC_ARR(fact->constr_coef, double,
                                        fact->constr_len);
    fact->constr_idx[fact->constr_len - 1] = op_id;
    fact->constr_coef[fact->constr_len - 1] = coef;
}

static void factAddProduce(fact_t *fact, int op_id)
{
    factAddConstr(fact, op_id, 1.);
}

static void factAddConsume(fact_t *fact, int op_id)
{
    factAddConstr(fact, op_id, -1.);
}

static void factsInitOp(fact_t *facts, const plan_fact_id_t *fact_id,
                        const plan_op_t *op, int op_id)
{
    int prei, effi, fid;
    const plan_part_state_t *pre, *eff;
    plan_var_id_t pre_var, eff_var;
    plan_val_t pre_val, eff_val;

    pre = op->pre;
    eff = op->eff;
    for (prei = 0, effi = 0; prei < pre->vals_size && effi < eff->vals_size;){
        pre_var = pre->vals[prei].var;
        eff_var = eff->vals[effi].var;
        if (pre_var == eff_var){
            // pre_val -> eff_val

            // The operator produces the eff_val
            eff_val = eff->vals[effi].val;
            fid = planFactId(fact_id, eff_var, eff_val);
            factAddProduce(facts + fid, op_id);

            // and consumes the pre_val
            pre_val = pre->vals[prei].val;
            fid = planFactId(fact_id, pre_var, pre_val);
            factAddConsume(facts + fid, op_id);

            ++prei;
            ++effi;

        }else if (pre_var < eff_var){
            // This is just prevail -- which can be ignored
            ++prei;
            continue;

        }else{ // eff_var < pre_var
            // (null) -> eff_val

            // The eff_val is produced
            eff_val = eff->vals[effi].val;
            fid = planFactId(fact_id, eff_var, eff_val);
            factAddProduce(facts + fid, op_id);

            // Also set the fact as causing incompletness because this
            // operator only produces and does not consume.
            facts[fid].cause_incomplete_op = 1;

            ++effi;
        }
    }

    // Process the rest of produced values the same way as in
    // (eff_var < pre_var) branch above.
    for (; effi < eff->vals_size; ++effi){
        eff_val = eff->vals[effi].val;
        fid = planFactId(fact_id, eff_var, eff_val);
        factAddProduce(facts + fid, op_id);
        facts[fid].cause_incomplete_op = 1;
    }
}

static void factsInitOps(fact_t *facts, const plan_fact_id_t *fact_id,
                         const plan_op_t *op, int op_size)
{
    int opi;

    for (opi = 0; opi < op_size; ++opi)
        factsInitOp(facts, fact_id, op + opi, opi);
}

static void factsInit(fact_t *facts, const plan_fact_id_t *fact_id,
                      const plan_var_t *var, int var_size,
                      const plan_part_state_t *goal,
                      const plan_op_t *op, int op_size)
{
    factsInitGoal(facts, fact_id, goal, var);
    factsInitOps(facts, fact_id, op, op_size);
}

static void factsSetState(fact_t *facts, const plan_fact_id_t *fact_id,
                          const plan_state_t *state)
{
    int i, fid;

    // First set .is_init flag
    for (i = 0; i < fact_id->fact_size; ++i)
        facts[i].is_init = 0;
    for (i = 0; i < fact_id->var_size; ++i){
        fid = planFactId(fact_id, i, planStateGet(state, i));
        if (fid >= 0)
            facts[fid].is_init = 1;
    }

    // Now set upper and lower bounds
    for (i = 0; i < fact_id->fact_size; ++i){
        if (facts[i].cause_incomplete_op){
            facts[i].lower_bound = lower_bound_table[facts[i].is_goal_var]
                                                    [facts[i].is_mutex_with_goal]
                                                    [facts[i].is_init];
            facts[i].upper_bound = upper_bound_table[facts[i].is_goal_var]
                                                    [facts[i].is_mutex_with_goal]
                                                    [facts[i].is_init]
                                                    [facts[i].cause_incomplete_op];
        }
    }
}
