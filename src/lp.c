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
#include "plan/config.h"
#include "plan/lp.h"

#ifdef PLAN_LP

#if defined(PLAN_USE_LP_SOLVE) && defined(PLAN_USE_CPLEX)
# error "Only one LP solver can be defined!"
#endif /* defined(PLAN_USE_LP_SOLVE) && defined(PLAN_USE_CPLEX) */

#ifdef PLAN_USE_LP_SOLVE
# include <lpsolve/lp_lib.h>

struct _plan_lp_t {
    lprec lp;
};

static int lpSense(char sense)
{
    if (sense == 'L'){
        return LE;
    }else if (sense == 'G'){
        return GE;
    }else if (sense == 'E'){
        return EQ;
    }else{
        fprintf(stderr, "LP Error: Unkown sense: %c\n", sense);
        return EQ;
    }
}

#endif /* PLAN_USE_LP_SOLVE */


#ifdef PLAN_USE_CPLEX
# include <ilcplex/cplex.h>

# define CPLEX_NUM_THREADS(flags) (((flags) >> 8u) & 0x3fu)
# define CPLEX_NUM_THREADS_AUTO 0x3fu

struct _plan_lp_t {
    CPXENVptr env;
    CPXLPptr lp;
    int mip;
};

static void cplexErr(plan_lp_t *lp, int status, const char *s)
{
    char errmsg[1024];
    CPXgeterrorstring(lp->env, status, errmsg);
    fprintf(stderr, "Error: CPLEX: %s: %s\n", s, errmsg);
    exit(-1);
}

static double cplexObjVal(plan_lp_t *lp)
{
    int st, solst;
    double z;

    solst = CPXgetstat(lp->env, lp->lp);
    if (solst != 0){
        st = CPXgetobjval(lp->env, lp->lp, &z);
        if (st == 0){
            return z;
        }else{
            cplexErr(lp, st, "Could not get objective value.");
        }
    }

    return CPX_INFBOUND;
}

#endif /* PLAN_USE_CPLEX */

plan_lp_t *planLPNew(int rows, int cols, unsigned flags)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *lp;

    lp = make_lp(rows, cols);
    if ((flags & 0x1u) == 0){
        set_minim(lp);
    }else{
        set_maxim(lp);
    }

    return (plan_lp_t *)lp;
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    plan_lp_t *lp;
    int st, num_threads;

    lp = BOR_ALLOC(plan_lp_t);
    lp->mip = 0;

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

    lp->lp = CPXcreateprob(lp->env, &st, "");
    if (lp->lp == NULL)
        cplexErr(lp, st, "Could not create CPLEX problem");

    // Set up minimaztion
    if ((flags & 0x1u) == 0){
        CPXchgobjsen(lp->env, lp->lp, CPX_MIN);
    }else{
        CPXchgobjsen(lp->env, lp->lp, CPX_MAX);
    }

    st = CPXnewcols(lp->env, lp->lp, cols, NULL, NULL, NULL, NULL, NULL);
    if (st != 0)
        cplexErr(lp, st, "Could not initialize variables");

    st = CPXnewrows(lp->env, lp->lp, rows, NULL, NULL, NULL, NULL);
    if (st != 0)
        cplexErr(lp, st, "Could not initialize constraints");

    return lp;
#endif /* PLAN_USE_CPLEX */
}


void planLPDel(plan_lp_t *lp)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *l = (lprec *)lp;
    delete_lp(l);
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    if (lp->lp)
        CPXfreeprob(lp->env, &lp->lp);
    if (lp->env)
        CPXcloseCPLEX(&lp->env);
#endif /* PLAN_USE_CPLEX */
    BOR_FREE(lp);
}

void planLPSetObj(plan_lp_t *lp, int i, double coef)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *l = (lprec *)lp;
    set_obj(l, i + 1, coef);
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    int st;

    st = CPXchgcoef(lp->env, lp->lp, -1, i, coef);
    if (st != 0)
        cplexErr(lp, st, "Could not set objective coeficient.");
#endif /* PLAN_USE_CPLEX */
}

void planLPSetVarRange(plan_lp_t *lp, int i, double lb, double ub)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *l = (lprec *)lp;
    set_lowbo(l, i + 1, lb);
    set_upbo(l, i + 1, ub);
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    static const char lu[2] = { 'L', 'U' };
    double bd[2] = { lb, ub };
    int ind[2];
    int st;

    ind[0] = ind[1] = i;
    st = CPXchgbds(lp->env, lp->lp, 2, ind, lu, bd);
    if (st != 0)
        cplexErr(lp, st, "Could not set variable as free.");
#endif /* PLAN_USE_CPLEX */
}

void planLPSetVarFree(plan_lp_t *lp, int i)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *l = (lprec *)lp;
    set_unbounded(l, i + 1);
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    planLPSetVarRange(lp, i, -CPX_INFBOUND, CPX_INFBOUND);
#endif /* PLAN_USE_CPLEX */
}

void planLPSetVarInt(plan_lp_t *lp, int i)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *l = (lprec *)lp;
    set_int(l, i + 1, 1);
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    static char type = CPX_INTEGER;
    int st;

    st = CPXchgctype(lp->env, lp->lp, 1, &i, &type);
    if (st != 0)
        cplexErr(lp, st, "Could not set variable as integer.");
    lp->mip = 1;
#endif /* PLAN_USE_CPLEX */
}

void planLPSetVarBinary(plan_lp_t *lp, int i)
{
#ifdef PLAN_USE_LP_SOLVE
    planLPSetVarRange(lp, i, 0, 1);
    planLPSetVarInt(lp, i);
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    static char type = CPX_BINARY;
    int st;

    st = CPXchgctype(lp->env, lp->lp, 1, &i, &type);
    if (st != 0)
        cplexErr(lp, st, "Could not set variable as binary.");
    lp->mip = 1;
#endif /* PLAN_USE_CPLEX */
}

void planLPSetCoef(plan_lp_t *lp, int row, int col, double coef)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *l = (lprec *)lp;
    set_mat(l, row + 1, col + 1, coef);
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    int st;

    st = CPXchgcoef(lp->env, lp->lp, row, col, coef);
    if (st != 0)
        cplexErr(lp, st, "Could not set constraint coeficient.");
#endif /* PLAN_USE_CPLEX */
}

void planLPSetRHS(plan_lp_t *lp, int row, double rhs, char sense)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *l = (lprec *)lp;
    set_rh(l, row + 1, rhs);
    set_constr_type(l, row + 1, lpSense(sense));
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    int st;

    st = CPXchgcoef(lp->env, lp->lp, row, -1, rhs);
    if (st != 0)
        cplexErr(lp, st, "Could not set right-hand-side.");

    st = CPXchgsense(lp->env, lp->lp, 1, &row, &sense);
    if (st != 0)
        cplexErr(lp, st, "Could not set right-hand-side sense.");
#endif /* PLAN_USE_CPLEX */
}

void planLPAddRows(plan_lp_t *lp, int cnt, const double *rhs, const char *sense)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *l = (lprec *)lp;
    int i, vsen = EQ;
    double vrhs = 0.;

    for (i = 0; i < cnt; ++i){
        if (rhs)
            vrhs = rhs[i];
        if (sense)
            vsen = lpSense(sense[i]);

        add_constraintex(l, 0, NULL, NULL, vsen, vrhs);
    }
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    int st;

    st = CPXnewrows(lp->env, lp->lp, cnt, rhs, sense, NULL, NULL);
    if (st != 0)
        cplexErr(lp, st, "Could not add new rows.");
#endif /* PLAN_USE_CPLEX */
}

void planLPDelRows(plan_lp_t *lp, int begin, int end)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *l = (lprec *)lp;
    int i;

    for (i = begin; i <= end; ++i)
        del_constraint(l, begin + 1);
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    int st;

    st = CPXdelrows(lp->env, lp->lp, begin, end);
    if (st != 0)
        cplexErr(lp, st, "Could not delete rows.");
#endif /* PLAN_USE_CPLEX */
}

int planLPNumRows(const plan_lp_t *lp)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *l = (lprec *)lp;
    return get_Nrows(l);
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    return CPXgetnumrows(lp->env, lp->lp);
#endif /* PLAN_USE_CPLEX */
}

int planLPSolve(plan_lp_t *lp, double *val, double *obj)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *l = (lprec *)lp;
    int ret;

    set_verbose(l, NEUTRAL);
    ret = solve(l);
    if (ret == OPTIMAL || ret == SUBOPTIMAL){
        if (val != NULL)
            *val = get_objective(l);
        if (obj != NULL)
            get_variables(l, obj);

        return 0;

    }else{
        if (obj != NULL)
            bzero(obj, sizeof(double) * get_Ncolumns(l));
        if (val != NULL)
            *val = 0.;
        return -1;
    }
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    int st;

    if (lp->mip){
        st = CPXmipopt(lp->env, lp->lp);
    }else{
        st = CPXlpopt(lp->env, lp->lp);
    }
    if (st != 0)
        cplexErr(lp, st, "Failed to optimize LP");

    st = CPXsolution(lp->env, lp->lp, NULL, val, obj, NULL, NULL, NULL);
    if (st == CPXERR_NO_SOLN){
        if (obj != NULL)
            bzero(obj, sizeof(double) * CPXgetnumcols(lp->env, lp->lp));
        if (val != NULL)
            *val = 0.;
        return -1;

    }else if (st != 0){
        cplexErr(lp, st, "Cannot retrieve solution");
        return -1;
    }
    return 0;
#endif /* PLAN_USE_CPLEX */
}


void planLPWrite(plan_lp_t *lp, const char *fn)
{
#ifdef PLAN_USE_LP_SOLVE
    lprec *l = (lprec *)lp;
    write_lp(l, (char *)fn);
#endif /* PLAN_USE_LP_SOLVE */

#ifdef PLAN_USE_CPLEX
    int st;

    st = CPXwriteprob(lp->env, lp->lp, fn, "LP");
    if (st != 0)
        cplexErr(lp, st, "Failed to optimize ILP");
#endif /* PLAN_USE_CPLEX */
}

#else /* PLAN_LP */

#warning "LP Warning: No LP library defined!"
void planNOLP(void)
{
}
#endif /* PLAN_LP */
