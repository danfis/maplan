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

#ifndef __PLAN_LP_H__
#define __PLAN_LP_H__

#include <plan/config.h>
#include <plan/op.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef PLAN_LP

/** Forward declaration */
typedef struct _plan_lp_t plan_lp_t;

/**
 * Sets the number of parallel threads that will be invoked by a CPLEX
 * parallel optimizer.
 * By default one thread is used.
 * Set num threads to -1 to switch to auto mode.
 */
#define PLAN_LP_CPLEX_NUM_THREADS(num) \
    ((((unsigned)(num)) & 0x3fu) << 8u)

/**
 * Sets minimization (default).
 */
#define PLAN_LP_MIN 0x0

/**
 * Sets maximization.
 */
#define PLAN_LP_MAX 0x1


/**
 * Creates a new LP problem with specified number of rows and columns.
 */
plan_lp_t *planLPNew(int rows, int cols, unsigned flags);

/**
 * Deletes the LP object.
 */
void planLPDel(plan_lp_t *lp);

/**
 * Sets objective coeficient for i'th variable.
 */
void planLPSetObj(plan_lp_t *lp, int i, double coef);

/**
 * Sets i'th variable's range.
 */
void planLPSetVarRange(plan_lp_t *lp, int i, double lb, double ub);

/**
 * Sets i'th variable as free.
 */
void planLPSetVarFree(plan_lp_t *lp, int i);

/**
 * Sets i'th variable as integer.
 */
void planLPSetVarInt(plan_lp_t *lp, int i);

/**
 * Sets i'th variable as binary.
 */
void planLPSetVarBinary(plan_lp_t *lp, int i);

/**
 * Sets coefficient for row's constraint and col's variable.
 */
void planLPSetCoef(plan_lp_t *lp, int row, int col, double coef);

/**
 * Sets right hand side of the row'th constraint.
 * Also sense of the constraint must be defined:
 *      - 'L' <=
 *      - 'G' >=
 *      - 'E' =
 */
void planLPSetRHS(plan_lp_t *lp, int row, double rhs, char sense);

/**
 * Adds cnt rows to the model.
 */
void planLPAddRows(plan_lp_t *lp, int cnt, const double *rhs, const char *sense);

/**
 * Deletes rows with indexes between begin and end including both limits,
 * i.e., first deleted row has index {begin} the last deleted row has index
 * {end}.
 */
void planLPDelRows(plan_lp_t *lp, int begin, int end);

/**
 * Returns number of rows in model.
 */
int planLPNumRows(const plan_lp_t *lp);

/**
 * Solves (I)LP problem.
 * Return 0 if problem was solved, -1 if the problem has no solution.
 * Objective value is returned via argument val and values of each variable
 * via argument obj if non-NULL.
 */
int planLPSolve(plan_lp_t *lp, double *val, double *obj);


void planLPWrite(plan_lp_t *lp, const char *fn);

#else /* PLAN_LP */

void planNOLP(void);
#endif /* PLAN_LP */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_LP_H__ */
