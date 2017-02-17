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
#include <plan/config.h>
#include <plan/heur.h>

#include "plan/lp.h"
#include "plan/fact_id.h"
#include "plan/fa_mutex.h"

#ifdef PLAN_LP

#define BOUND_INF 1E30
#define ROUND_EPS 1E-6

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
    int var;                /*!< Variable ID */
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
    int use_ilp;            /*!< True if ILP instead of LP should be used */
    plan_fact_id_t fact_id; /*!< Translation from var-val pair to fact ID */
    fact_t *facts;          /*!< Array of fact related structures */
    plan_lp_t *lp;          /*!< (I)LP solver */
    plan_heur_t *lm_cut;    /*!< LM-Cut heuristic used for landmarks */
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

static plan_lp_t *lpInit(const fact_t *facts, int facts_size,
                         const plan_op_t *op, int op_size, int use_ilp,
                         unsigned flags);
static plan_cost_t lpSolve(plan_lp_t *lp, const fact_t *facts,
                           int facts_size, int use_ilp,
                           const plan_landmark_set_t *ldms);

static void factsInitFAMutex(fact_t *facts, const plan_problem_t *p,
                             const plan_fact_id_t *fact_id)
{
    plan_mutex_group_set_t ms;
    PLAN_STATE_STACK(state, p->var_size);
    plan_mutex_group_t *m;
    int i, j, fid;

    planStatePoolGetState(p->state_pool, p->initial_state, &state);
    planMutexGroupSetInit(&ms);
    planFAMutexFind(p, &state, &ms, PLAN_FA_MUTEX_ONLY_GOAL);
    for (i = 0; i < ms.group_size; ++i){
        m = ms.group + i;
        for (j = 0; j < m->fact_size; ++j){
            fid = planFactIdVar(fact_id, m->fact[j].var, m->fact[j].val);
            if (!facts[fid].is_goal)
                facts[fid].is_mutex_with_goal = 1;
            facts[fid].is_goal_var = 1;
            facts[fid].cause_incomplete_op = 0;
        }
    }
    planMutexGroupSetFree(&ms);
}

plan_heur_t *planHeurFlowNew(const plan_problem_t *p, unsigned flags)
{
    plan_heur_flow_t *hflow;

    hflow = BOR_ALLOC(plan_heur_flow_t);
    _planHeurInit(&hflow->heur, heurFlowDel, heurFlow, NULL);
    hflow->use_ilp = (flags & PLAN_HEUR_FLOW_ILP);

    planFactIdInit(&hflow->fact_id, p->var, p->var_size, 0);
    hflow->facts = BOR_CALLOC_ARR(fact_t, hflow->fact_id.fact_size);
    factsInit(hflow->facts, &hflow->fact_id, p->var, p->var_size,
              p->goal, p->op, p->op_size);
    if (flags & PLAN_HEUR_FLOW_FA_MUTEX)
        factsInitFAMutex(hflow->facts, p, &hflow->fact_id);

    // Initialize LP solver
    hflow->lp = lpInit(hflow->facts, hflow->fact_id.fact_size,
                       p->op, p->op_size, hflow->use_ilp, flags);

    hflow->lm_cut = NULL;
    if (flags & PLAN_HEUR_FLOW_LANDMARKS_LM_CUT)
        hflow->lm_cut = planHeurLMCutNew(p, flags);

    return &hflow->heur;
}

static void heurFlowDel(plan_heur_t *_heur)
{
    plan_heur_flow_t *hflow = HEUR(_heur);
    int i;

    if (hflow->lm_cut != NULL)
        planHeurDel(hflow->lm_cut);
    planLPDel(hflow->lp);

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
    plan_heur_res_t ldms_res;
    plan_landmark_set_t *ldms = NULL;

    // Compute landmark heuristic to get landmarks.
    if (hflow->lm_cut){
        planHeurResInit(&ldms_res);
        ldms_res.save_landmarks = 1;
        planHeurState(hflow->lm_cut, state, &ldms_res);
        ldms = &ldms_res.landmarks;
    }

    factsSetState(hflow->facts, &hflow->fact_id, state);
    res->heur = lpSolve(hflow->lp, hflow->facts, hflow->fact_id.fact_size,
                        hflow->use_ilp, ldms);

    // Free allocated landmarks
    if (hflow->lm_cut)
        planLandmarkSetFree(ldms);
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
        fid = planFactIdVar(fact_id, var, val);
        if (fid >= 0)
            facts[fid].is_goal = 1;

        // Set .is_goal_var and .is_mutex_with_goal flag
        for (val2 = 0; val2 < vars[var].range; ++val2){
            fid = planFactIdVar(fact_id, var, val2);
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
                        const plan_op_t *op, int op_id,
                        int *cause_incomplete_op)
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
            fid = planFactIdVar(fact_id, eff_var, eff_val);
            factAddProduce(facts + fid, op_id);

            // and consumes the pre_val
            pre_val = pre->vals[prei].val;
            fid = planFactIdVar(fact_id, pre_var, pre_val);
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
            fid = planFactIdVar(fact_id, eff_var, eff_val);
            factAddProduce(facts + fid, op_id);

            // Also set the fact as causing incompletness because this
            // operator only produces and does not consume.
            cause_incomplete_op[facts[fid].var] = 1;

            ++effi;
        }
    }

    // Process the rest of produced values the same way as in
    // (eff_var < pre_var) branch above.
    for (; effi < eff->vals_size; ++effi){
        eff_var = eff->vals[effi].var;
        eff_val = eff->vals[effi].val;
        fid = planFactIdVar(fact_id, eff_var, eff_val);
        factAddProduce(facts + fid, op_id);
        cause_incomplete_op[facts[fid].var] = 1;
    }
}

static void factsInitOps(fact_t *facts, const plan_fact_id_t *fact_id,
                         const plan_op_t *op, int op_size)
{
    int *cause_incomplete_op;
    int i, opi;

    cause_incomplete_op = BOR_CALLOC_ARR(int, fact_id->var_size);
    for (opi = 0; opi < op_size; ++opi)
        factsInitOp(facts, fact_id, op + opi, opi, cause_incomplete_op);

    for (i = 0; i < fact_id->fact_size; ++i){
        if (cause_incomplete_op[facts[i].var])
            facts[i].cause_incomplete_op = 1;
    }
    BOR_FREE(cause_incomplete_op);
}

static void factsInit(fact_t *facts, const plan_fact_id_t *fact_id,
                      const plan_var_t *var, int var_size,
                      const plan_part_state_t *goal,
                      const plan_op_t *op, int op_size)
{
    int vi, ri, fid;

    for (vi = 0; vi < var_size; ++vi){
        if (var[vi].ma_privacy)
            continue;
        for (ri = 0; ri < var[vi].range; ++ri){
            fid = planFactIdVar(fact_id, vi, ri);
            if (fid >= 0)
                facts[fid].var = vi;
        }
    }

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
        fid = planFactIdVar(fact_id, i, planStateGet(state, i));
        if (fid >= 0)
            facts[fid].is_init = 1;
    }

    // Now set upper and lower bounds
    for (i = 0; i < fact_id->fact_size; ++i){
        facts[i].lower_bound = lower_bound_table[facts[i].is_goal_var]
                                                [facts[i].is_mutex_with_goal]
                                                [facts[i].is_init];
        facts[i].upper_bound = upper_bound_table[facts[i].is_goal_var]
                                                [facts[i].is_mutex_with_goal]
                                                [facts[i].is_init]
                                                [facts[i].cause_incomplete_op];
    }
}

static plan_cost_t roundOff(double z)
{
    plan_cost_t v = z;
    if (fabs(z - (double)v) > ROUND_EPS)
        return ceil(z);
    return v;
}

_bor_inline plan_cost_t _cost(plan_cost_t cost, unsigned flags)
{
    if (flags & PLAN_HEUR_OP_UNIT_COST)
        return 1;
    if (flags & PLAN_HEUR_OP_COST_PLUS_ONE)
        return cost + 1;
    return cost;
}

static plan_lp_t *lpInit(const fact_t *facts, int facts_size,
                         const plan_op_t *op, int op_size, int use_ilp,
                         unsigned flags)
{
    plan_lp_t *lp;
    int i, r, c;
    double coef;
    unsigned lp_flags = 0;

    lp_flags |= (flags & (0x3fu << 8u));
    lp = planLPNew(2 * facts_size, op_size, lp_flags);

    // Set up columns
    for (i = 0; i < op_size; ++i){
        if (use_ilp)
            planLPSetVarInt(lp, i);
        planLPSetObj(lp, i, _cost(op[i].cost, flags));
    }

    // Set up rows
    for (r = 0; r < facts_size; ++r){
        for (i = 0; i < facts[r].constr_len; ++i){
            c = facts[r].constr_idx[i];
            coef = facts[r].constr_coef[i];
            planLPSetCoef(lp, 2 * r, c, coef);
            planLPSetCoef(lp, 2 * r + 1, c, coef);
        }
    }

    return lp;
}

static void lpAddLandmarks(plan_lp_t *lp, const plan_landmark_set_t *ldms)
{
    plan_landmark_t *ldm;
    int i, j, row_id;

    if (ldms == NULL || ldms->size == 0)
        return;


    row_id = planLPNumRows(lp);
    planLPAddRows(lp, ldms->size, NULL, NULL);
    for (i = 0; i < ldms->size; ++i, ++row_id){
        ldm = ldms->landmark + i;
        planLPSetRHS(lp, row_id, 1., 'G');
        for (j = 0; j < ldm->size; ++j)
            planLPSetCoef(lp, row_id, ldm->op_id[j], 1.);
    }
}

static void lpDelLandmarks(plan_lp_t *lp, const plan_landmark_set_t *ldms)
{
    int from, to;

    if (ldms == NULL || ldms->size == 0)
        return;

    to = planLPNumRows(lp) - 1;
    from = planLPNumRows(lp) - ldms->size;
    planLPDelRows(lp, from, to);
}

static plan_cost_t lpSolve(plan_lp_t *lp, const fact_t *facts,
                           int facts_size, int use_ilp,
                           const plan_landmark_set_t *ldms)
{
    plan_cost_t h = PLAN_HEUR_DEAD_END;
    int i;
    double z;

    // Add row for each fact
    for (i = 0; i < facts_size; ++i){
        double upper, lower;
        lower = facts[i].lower_bound;
        upper = facts[i].upper_bound;

        if (lower == upper){
            planLPSetRHS(lp, 2 * i, lower, 'E');
            planLPSetRHS(lp, 2 * i + 1, upper, 'E');

        }else{
            planLPSetRHS(lp, 2 * i, lower, 'G');
            planLPSetRHS(lp, 2 * i + 1, upper, 'L');
        }
    }

    // Add landmarks if provided
    lpAddLandmarks(lp, ldms);

    if (planLPSolve(lp, &z, NULL) != 0){
        fprintf(stderr, "Error: LP has no solution!\n");
        exit(-1);
    }
    h = roundOff(z);

    lpDelLandmarks(lp, ldms);
    return h;
}

#else /* PLAN_LP */

plan_heur_t *planHeurFlowNew(const plan_problem_t *p, unsigned flags)
{
    fprintf(stderr, "Error: Cannot create Flow heuristic object because no"
                    " LP-solver was available during compilation!\n");
    fflush(stderr);
    return NULL;
}
#endif /* PLAN_LP */
