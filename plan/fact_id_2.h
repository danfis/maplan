/***
 * maplan
 * -------
 * Copyright (c)2017 Daniel Fiser <danfis@danfis.cz>,
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

#ifndef __PLAN_FACT_ID_2_H__
#define __PLAN_FACT_ID_2_H__

#include <plan/problem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Translator from var/val to IDs corresponding to combinations of up to 2
 * facts.
 */
struct _plan_fact_id_2_t {
    int **var; /*! [var][val] -> fact_id */
    int var_size;
    int fact_size;
    int fact1_size;
    int *fact2;
};
typedef struct _plan_fact_id_2_t plan_fact_id_2_t;

/**
 * Initializes object.
 */
void planFactId2Init(plan_fact_id_2_t *f, const plan_var_t *var, int var_size);

/**
 * Frees allocated memory.
 */
void planFactId2Free(plan_fact_id_2_t *f);

/**
 * Returns ID of a unary fact.
 */
int planFactId2Var(const plan_fact_id_2_t *f,
                   plan_var_id_t var, plan_val_t val);

/**
 * Returns ID corresponding to the combination of two facts.
 * It is always assumed that var1 and var2 differ.
 */
int planFactId2Var2(const plan_fact_id_2_t *f,
                    plan_var_id_t var1, plan_val_t val1,
                    plan_var_id_t var2, plan_val_t val2);

/**
 * Returns ID corresponding to the combination of two facts.
 * It is always assumed that fid1 and fid2 are IDs of unary facts and they
 * does not belong to the same variable.
 */
int planFactId2Fact2(const plan_fact_id_2_t *f, int fid1, int fid2);

/**
 * Returns a list of binary fact IDs corresponding to the state.
 * The returned array always begins with state->size IDs that correspond to
 * unary facts.
 */
int *planFactId2State(const plan_fact_id_2_t *f, const plan_state_t *state,
                      int *size);

/**
 * Same as planFactId2State() but translates a partial state and first
 * part_state->vals_size IDs correspond to unary facts.
 */
int *planFactId2PartState(const plan_fact_id_2_t *f,
                          const plan_part_state_t *part_state, int *size);


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_FACT_ID_2_H__ */
