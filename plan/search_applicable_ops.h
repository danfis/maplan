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

#ifndef __PLAN_SEARCH_APPLICABLE_OPS_H__
#define __PLAN_SEARCH_APPLICABLE_OPS_H__

#include <plan/op.h>
#include <plan/succ_gen.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_search_applicable_ops_t {
    plan_op_t **op;        /*!< Array of applicable operators. This array
                                must be big enough to hold all operators. */
    int op_size;           /*!< Size of .op[] */
    int op_found;          /*!< Number of found applicable operators */
    int op_preferred;      /*!< Number of preferred operators (that are
                                stored at the beggining of .op[] array */
    plan_state_id_t state; /*!< State in which these operators are
                                applicable */
};
typedef struct _plan_search_applicable_ops_t plan_search_applicable_ops_t;

/**
 * Initializes structure.
 */
void planSearchApplicableOpsInit(plan_search_applicable_ops_t *app_ops,
                                 int op_size);

/**
 * Frees resources
 */
void planSearchApplicableOpsFree(plan_search_applicable_ops_t *app_ops);

/**
 * Fills structure with applicable operators.
 * Returns 0 if the result was already cached and no search was performed,
 * and 1 if a new search was performed.
 */
int planSearchApplicableOpsFind(plan_search_applicable_ops_t *app_ops,
                                const plan_state_t *state,
                                plan_state_id_t state_id,
                                const plan_succ_gen_t *succ_gen);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_SEARCH_APPLICABLE_OPS_H__ */
