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

#include "fact_id.h"

#if defined(PLAN_USE_LP_SOLVE) || defined(PLAN_USE_CPLEX)

#ifdef PLAN_USE_CPLEX
# include <ilcplex/cplex.h>
struct _lp_cplex_t {
    CPXENVptr env;
    CPXLPptr lp;
    int num_constrs;
    int *idx;
    double *rhs;
};
typedef struct _lp_cplex_t lp_cplex_t;
# define LP_STRUCT lp_cplex_t
# define LP_INIT lpCplexInit
# define LP_FREE lpCplexFree
# define LP_SOLVE lpCplexSolve
# define PLAN_HEUR_FLOW_CPLEX

# define CPLEX_NUM_THREADS(flags) (((flags) >> 8u) & 0x3fu)
# define CPLEX_NUM_THREADS_AUTO 0x3fu
#endif /* PLAN_USE_CPLEX */

// Prefer CPLEX over LP_SOVE
#if !defined(PLAN_USE_CPLEX) && defined(PLAN_USE_LP_SOLVE)
# include <lpsolve/lp_lib.h>
# define LP_STRUCT lprec *
# define LP_INIT lpLPSolveInit
# define LP_FREE lpLPSolveFree
# define LP_SOLVE lpLPSolveSolve
# define PLAN_HEUR_FLOW_LP_SOLVE
#endif /* PLAN_USE_LP_SOLVE */


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
    LP_STRUCT lp;           /*!< Pre-initialized (I)LP solver */
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


/* lp_solve solver functions */
#ifdef PLAN_HEUR_FLOW_LP_SOLVE
static void lpLPSolveInit(lprec **lp, const fact_t *facts, int facts_size,
                          const plan_op_t *op, int op_size, int use_ilp,
                          unsigned flags);
static void lpLPSolveFree(lprec **lp);
static plan_cost_t lpLPSolveSolve(lprec **lp, const fact_t *facts,
                                  int facts_size, int use_ilp,
                                  const plan_landmark_set_t *ldms);
#endif /* PLAN_HEUR_FLOW_LP_SOLVE */

#ifdef PLAN_HEUR_FLOW_CPLEX
static void lpCplexInit(lp_cplex_t *lp, const fact_t *facts, int facts_size,
                        const plan_op_t *op, int op_size, int use_ilp,
                        unsigned flags);
static void lpCplexFree(lp_cplex_t *lp);
static plan_cost_t lpCplexSolve(lp_cplex_t *lp, const fact_t *facts,
                                int facts_size, int use_ilp,
                                const plan_landmark_set_t *ldms);
#endif /* PLAN_HEUR_FLOW_CPLEX */

plan_heur_t *planHeurFlowNew(const plan_var_t *var, int var_size,
                             const plan_part_state_t *goal,
                             const plan_op_t *op, int op_size,
                             unsigned flags)
{
    plan_heur_flow_t *hflow;

    hflow = BOR_ALLOC(plan_heur_flow_t);
    _planHeurInit(&hflow->heur, heurFlowDel, heurFlow, NULL);
    hflow->use_ilp = (flags & PLAN_HEUR_FLOW_ILP);

    planFactIdInit(&hflow->fact_id, var, var_size);
    hflow->facts = BOR_CALLOC_ARR(fact_t, hflow->fact_id.fact_size);
    factsInit(hflow->facts, &hflow->fact_id, var, var_size, goal, op, op_size);

    // Initialize LP solver
    LP_INIT(&hflow->lp, hflow->facts, hflow->fact_id.fact_size, op, op_size,
            hflow->use_ilp, flags);

    hflow->lm_cut = NULL;
    if (flags & PLAN_HEUR_FLOW_LANDMARKS_LM_CUT)
        hflow->lm_cut = planHeurLMCutNew(var, var_size, goal, op, op_size);

    return &hflow->heur;
}

static void heurFlowDel(plan_heur_t *_heur)
{
    plan_heur_flow_t *hflow = HEUR(_heur);
    int i;

    if (hflow->lm_cut != NULL)
        planHeurDel(hflow->lm_cut);
    LP_FREE(&hflow->lp);

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
    res->heur = LP_SOLVE(&hflow->lp, hflow->facts, hflow->fact_id.fact_size,
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
            cause_incomplete_op[facts[fid].var] = 1;

            ++effi;
        }
    }

    // Process the rest of produced values the same way as in
    // (eff_var < pre_var) branch above.
    for (; effi < eff->vals_size; ++effi){
        eff_var = eff->vals[effi].var;
        eff_val = eff->vals[effi].val;
        fid = planFactId(fact_id, eff_var, eff_val);
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
            fid = planFactId(fact_id, vi, ri);
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
        fid = planFactId(fact_id, i, planStateGet(state, i));
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

#ifdef PLAN_HEUR_FLOW_LP_SOLVE
static void lpLPSolveInit(lprec **_lp, const fact_t *facts, int facts_size,
                          const plan_op_t *op, int op_size, int use_ilp,
                          unsigned flags)
{
    lprec *lp;
    int i, r, c;
    double coef;

    lp = make_lp(2 * facts_size, op_size);
    set_minim(lp);

    // Set up columns
    for (i = 0; i < op_size; ++i){
        if (use_ilp)
            set_int(lp, i + 1, 1);
        set_obj(lp, i + 1, op[i].cost);
    }

    // Set up rows
    for (r = 0; r < facts_size; ++r){
        for (i = 0; i < facts[r].constr_len; ++i){
            c = facts[r].constr_idx[i] + 1;
            coef = facts[r].constr_coef[i];
            set_mat(lp, 2 * r + 1, c, coef);
            set_mat(lp, 2 * r + 2, c, coef);
        }
    }

    *_lp = lp;
}

static void lpLPSolveFree(lprec **lp)
{
    if (*lp)
        delete_lp(*lp);
}

static void lpSolveAddLandmarks(lprec *lp, const plan_landmark_set_t *ldms)
{
    plan_landmark_t *ldm;
    REAL *row = NULL;
    int *colno = NULL;
    int rowsize = 0, i, j;

    if (ldms == NULL)
        return;

    for (i = 0; i < ldms->size; ++i){
        ldm = ldms->landmark + i;
        if (rowsize < ldm->size){
            row = BOR_REALLOC_ARR(row, REAL, ldm->size);
            colno = BOR_REALLOC_ARR(colno, int, ldm->size);
            for (; rowsize < ldm->size; ++rowsize)
                row[rowsize] = 1.;
        }
        for (j = 0; j < ldm->size; ++j)
            colno[j] = ldm->op_id[j] + 1;
        add_constraintex(lp, ldm->size, row, colno, GE, 1.);
    }

    if (row != NULL)
        BOR_FREE(row);
    if (colno != NULL)
        BOR_FREE(colno);
}

static void lpSolveDelLandmarks(lprec *lp, const plan_landmark_set_t *ldms)
{
    int i;

    if (ldms == NULL)
        return;

    for (i = 0; i < ldms->size; ++i){
        del_constraint(lp, get_Nrows(lp));
    }
}

static plan_cost_t lpLPSolveSolve(lprec **_lp, const fact_t *facts,
                                  int facts_size, int use_ilp,
                                  const plan_landmark_set_t *ldms)
{
    lprec *lp = *_lp;
    plan_cost_t h = PLAN_HEUR_DEAD_END;
    int i, ret;

    // Add row for each fact
    for (i = 0; i < facts_size; ++i){
        double upper, lower;
        lower = facts[i].lower_bound;
        upper = facts[i].upper_bound;

        if (lower == upper){
            set_rh(lp, 2 * i + 1, lower);
            set_rh(lp, 2 * i + 2, upper);
            set_constr_type(lp, 2 * i + 1, EQ);
            set_constr_type(lp, 2 * i + 2, EQ);

        }else if (upper == BOUND_INF){
            set_rh(lp, 2 * i + 1, lower);
            set_constr_type(lp, 2 * i + 1, GE);
            set_constr_type(lp, 2 * i + 2, FR);

        }else{
            set_rh(lp, 2 * i + 1, lower);
            set_rh(lp, 2 * i + 2, upper);
            set_constr_type(lp, 2 * i + 1, GE);
            set_constr_type(lp, 2 * i + 2, LE);
        }
    }

    // Add landmarks if provided
    lpSolveAddLandmarks(lp, ldms);

    set_verbose(lp, NEUTRAL);
    ret = solve(lp);
    if (ret == OPTIMAL || ret == SUBOPTIMAL)
        h = roundOff(get_objective(lp));

    lpSolveDelLandmarks(lp, ldms);
    return h;
}
#endif /* PLAN_HEUR_FLOW_LP_SOLVE */


#ifdef PLAN_HEUR_FLOW_CPLEX
static void cplexErr(lp_cplex_t *lp, int status, const char *s)
{
    char errmsg[1024];
    CPXgeterrorstring(lp->env, status, errmsg);
    fprintf(stderr, "Error: CPLEX: %s: %s\n", s, errmsg);
    exit(-1);
}

static void lpCplexInit(lp_cplex_t *lp, const fact_t *facts, int facts_size,
                        const plan_op_t *op, int op_size, int use_ilp,
                        unsigned flags)
{
    int st, *cbeg, *cind, num_constrs, num_nz, cur, i, j;
    double *obj, *lb, *cval;
    char *sense, *ctype = NULL;
    int num_threads;

    // Initialize CPLEX structures
    lp->env = CPXopenCPLEX(&st);
    if (lp->env == NULL)
        cplexErr(lp, st, "Could not open CPLEX environment");

    // Set number of processing threads
    num_threads = CPLEX_NUM_THREADS(flags);
    if (num_threads == CPLEX_NUM_THREADS_AUTO){
        num_threads = 0;
    }else if (num_threads == 0){
        num_threads = 1;
    }
    st = CPXsetintparam(lp->env, CPX_PARAM_THREADS, num_threads);
    if (st != 0)
        cplexErr(lp, st, "Could not set number of threads");

    lp->lp = CPXcreateprob(lp->env, &st, "heurflow");
    if (lp->lp == NULL)
        cplexErr(lp, st, "Could not create CPLEX problem");

    // Set up minimaztion
    CPXchgobjsen(lp->env, lp->lp, CPX_MIN);

    // Set up variables
    obj = BOR_ALLOC_ARR(double, op_size);
    lb = BOR_CALLOC_ARR(double, op_size);
    for (i = 0; i < op_size; ++i)
        obj[i] = op[i].cost;

    if (use_ilp){
        ctype = BOR_ALLOC_ARR(char, op_size);
        for (i = 0; i < op_size; ++i)
            ctype[i] = 'I';
    }

    st = CPXnewcols(lp->env, lp->lp, op_size, obj, lb, NULL, ctype, NULL);
    if (st != 0)
        cplexErr(lp, st, "Could not initialize variables");
    BOR_FREE(obj);
    BOR_FREE(lb);
    if (ctype)
        BOR_FREE(ctype);

    // Prepare data for constraints matrix
    num_constrs = 2 * facts_size;
    num_nz = 0;
    for (i = 0; i < facts_size; ++i)
        num_nz += facts[i].constr_len;
    num_nz *= 2;

    cbeg = BOR_ALLOC_ARR(int, num_constrs);
    cind = BOR_ALLOC_ARR(int, num_nz);
    cval = BOR_ALLOC_ARR(double, num_nz);
    sense = BOR_ALLOC_ARR(char, num_constrs);

    for (cur = 0, i = 0; i < facts_size; ++i){
        cbeg[2 * i] = cur;
        for (j = 0; j < facts[i].constr_len; ++j){
            cind[cur] = facts[i].constr_idx[j];
            cval[cur] = facts[i].constr_coef[j];
            ++cur;
        }

        cbeg[2 * i + 1] = cur;
        for (j = 0; j < facts[i].constr_len; ++j){
            cind[cur] = facts[i].constr_idx[j];
            cval[cur] = facts[i].constr_coef[j];
            ++cur;
        }

        sense[2 * i] = 'G';
        sense[2 * i + 1] = 'L';
    }

    st = CPXaddrows(lp->env, lp->lp, 0, num_constrs, num_nz, NULL, sense,
                    cbeg, cind, cval, NULL, NULL);
    if (st != 0)
        cplexErr(lp, st, "Could not initialize constriant matrix");

    BOR_FREE(cbeg);
    BOR_FREE(cind);
    BOR_FREE(cval);
    BOR_FREE(sense);

    lp->num_constrs = num_constrs;
    lp->idx = BOR_ALLOC_ARR(int, num_constrs);
    lp->rhs = BOR_ALLOC_ARR(double, num_constrs);
    for (i = 0; i < num_constrs; ++i)
        lp->idx[i] = i;
}

static void lpCplexFree(lp_cplex_t *lp)
{
    if (lp->lp)
        CPXfreeprob(lp->env, &lp->lp);
    if (lp->env)
        CPXcloseCPLEX(&lp->env);
    if (lp->idx)
        BOR_FREE(lp->idx);
    if (lp->rhs)
        BOR_FREE(lp->rhs);
}


static void lpCplexAddLandmarks(lp_cplex_t *lp, const plan_landmark_set_t *ldms)
{
    plan_landmark_t *ldm;
    int num_constrs, num_nz, i, row, ins, st;
    double *rhs;
    char *sense;
    int *rbeg, *rind;
    double *val;

    if (ldms == NULL || ldms->size == 0)
        return;

    num_constrs = ldms->size;
    for (num_nz = 0, i = 0; i < num_constrs; ++i)
        num_nz += ldms->landmark[i].size;

    rhs = BOR_ALLOC_ARR(double, num_constrs);
    sense = BOR_ALLOC_ARR(char, num_constrs);
    rbeg = BOR_ALLOC_ARR(int, num_constrs);
    rind = BOR_ALLOC_ARR(int, num_nz);
    val = BOR_ALLOC_ARR(double, num_nz);

    for (ins = 0, row = 0; row < num_constrs; ++row){
        ldm = ldms->landmark + row;

        rbeg[row] = ins;
        rhs[row] = 1.;
        sense[row] = 'G';
        for (i = 0; i < ldm->size; ++i){
            rind[ins] = ldm->op_id[i];
            val[ins] = 1.;
            ++ins;
        }
    }

    st = CPXaddrows(lp->env, lp->lp, 0, num_constrs, num_nz, rhs, sense,
                    rbeg, rind, val, NULL, NULL);
    if (st != 0){
        fprintf(stderr, "Error: Could not add constraints to the CPLEX.\n");
        exit(-1);
    }

    BOR_FREE(rhs);
    BOR_FREE(sense);
    BOR_FREE(rbeg);
    BOR_FREE(rind);
    BOR_FREE(val);
}

static void lpCplexDelLandmarks(lp_cplex_t *lp, const plan_landmark_set_t *ldms)
{
    int from, to;

    if (ldms == NULL || ldms->size == 0)
        return;

    from = lp->num_constrs;
    to = from + ldms->size - 1;
    if (CPXdelrows(lp->env, lp->lp, from, to) != 0)
        fprintf(stderr, "ERR\n");
}

static plan_cost_t lpCplexSolve(lp_cplex_t *lp, const fact_t *facts,
                                int facts_size, int use_ilp,
                                const plan_landmark_set_t *ldms)
{
    plan_cost_t h = PLAN_HEUR_DEAD_END;
    int i, st, solst;
    double z;

    // Change lower and upper bounds
    for (i = 0; i < facts_size; ++i){
        lp->rhs[2 * i] = facts[i].lower_bound;
        lp->rhs[2 * i + 1] = facts[i].upper_bound;
    }

    st = CPXchgrhs(lp->env, lp->lp, lp->num_constrs, lp->idx, lp->rhs);
    if (st != 0)
        cplexErr(lp, st, "Could not update constraint matrix");

    // Add landmark constraints if requested
    lpCplexAddLandmarks(lp, ldms);

    // Optimize
    if (use_ilp){
        st = CPXmipopt(lp->env, lp->lp);
    }else{
        st = CPXlpopt(lp->env, lp->lp);
    }
    if (st != 0)
        cplexErr(lp, st, "Failed to optimize LP");

    // Read out solution
    solst = CPXgetstat(lp->env, lp->lp);
    if (solst != 0){
        st = CPXgetobjval(lp->env, lp->lp, &z);
        if (st == 0)
            h = roundOff(z);
    }

    // Remove landmark constraints if they were added
    lpCplexDelLandmarks(lp, ldms);
    //CPXwriteprob(lp->env, lp->lp, "cplex", "LP");
    return h;
}
#endif /* PLAN_HEUR_FLOW_CPLEX */

#else /* defined(PLAN_USE_LP_SOLVE) || defined(PLAN_USE_CPLEX) */

plan_heur_t *planHeurFlowNew(const plan_var_t *var, int var_size,
                             const plan_part_state_t *goal,
                             const plan_op_t *op, int op_size,
                             unsigned flags)
{
    fprintf(stderr, "Error: Cannot create Flow heuristic object because no"
                    " LP-solver was available during compilation!\n");
    fflush(stderr);
    return NULL;
}
#endif /* defined(PLAN_USE_LP_SOLVE) || defined(PLAN_USE_CPLEX) */
