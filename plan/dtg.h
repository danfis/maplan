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

#ifndef __PLAN_DTG_H__
#define __PLAN_DTG_H__

#include <plan/var.h>
#include <plan/op.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Srtruct representing transition between pair of values.
 */
struct _plan_dtg_trans_t {
    const plan_op_t **ops;
    int ops_size;
};
typedef struct _plan_dtg_trans_t plan_dtg_trans_t;

/**
 * Domain transition graph for one variable.
 */
struct _plan_dtg_var_t {
    plan_var_id_t var;   /*!< ID of the variable */
    plan_val_t val_size; /*!< Number of values */
    plan_dtg_trans_t *trans;  /*!< Incidence matrix containing transition
                              operators for each pair of values */
};
typedef struct _plan_dtg_var_t plan_dtg_var_t;

/**
 * Domain transition graph
 */
struct _plan_dtg_t {
    int var_size;   /*!< Number of variables */
    plan_dtg_var_t *dtg; /*!< DTG for each variable */
};
typedef struct _plan_dtg_t plan_dtg_t;

struct _plan_dtg_path_t {
    int length; /*!< Length of the path */
    const plan_dtg_trans_t **trans; /*!< Transitions on the path */
};
typedef struct _plan_dtg_path_t plan_dtg_path_t;

void planDTGPathInit(plan_dtg_path_t *path);
void planDTGPathFree(plan_dtg_path_t *path);

/**
 * Constructs domain transition graph for all variables.
 */
void planDTGInit(plan_dtg_t *dtg, const plan_var_t *var, int var_size,
                 const plan_op_t *op, int op_size);

/**
 * Same as planDTGInit() but adds one value to each variable as placeholder
 * for unknown value.
 */
void planDTGInitUnknown(plan_dtg_t *dtg, const plan_var_t *var, int var_size,
                        const plan_op_t *op, int op_size);

/**
 * Frees allocated resources.
 */
void planDTGFree(plan_dtg_t *dtg);

/**
 * Adds transition to var's DTG between the two specified values.
 */
void planDTGAddTrans(plan_dtg_t *dtg, plan_var_id_t var, plan_val_t from,
                     plan_val_t to, const plan_op_t *op);

/**
 * Returns true if op is on transition between from and to.
 */
int planDTGHasTrans(const plan_dtg_t *dtg, plan_var_id_t var,
                    plan_val_t from, plan_val_t to,
                    const plan_op_t *op);

/**
 * Returns a transition between two values.
 */
_bor_inline const plan_dtg_trans_t *planDTGTrans(const plan_dtg_t *dtg,
                                                 plan_var_id_t var,
                                                 plan_val_t from,
                                                 plan_val_t to);

/**
 * Finds shortest path between two values of a variable.
 * Returns 0 if such a path was found, -1 otherwise.
 */
int planDTGPath(const plan_dtg_t *dtg, plan_var_id_t var,
                plan_val_t from, plan_val_t to,
                plan_dtg_path_t *path);

/**
 * For debuf purposes
 */
void planDTGPrint(const plan_dtg_t *dtg, FILE *fout);


/**** INLINES: ****/
_bor_inline const plan_dtg_trans_t *planDTGTrans(const plan_dtg_t *dtg,
                                                 plan_var_id_t var,
                                                 plan_val_t from,
                                                 plan_val_t to)
{
    return dtg->dtg[var].trans + (from * dtg->dtg[var].val_size) + to;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_DTG_H__ */
