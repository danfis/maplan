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

#include "plan/search_applicable_ops.h"


void planSearchApplicableOpsInit(plan_search_applicable_ops_t *app_ops,
                                 int op_size)
{
    app_ops->op = BOR_ALLOC_ARR(plan_op_t *, op_size);
    app_ops->op_size = op_size;
    app_ops->op_found = 0;
    app_ops->state = PLAN_NO_STATE;
}

void planSearchApplicableOpsFree(plan_search_applicable_ops_t *app_ops)
{
    BOR_FREE(app_ops->op);
}

int planSearchApplicableOpsFind(plan_search_applicable_ops_t *app,
                                const plan_state_t *state,
                                plan_state_id_t state_id,
                                const plan_succ_gen_t *succ_gen)
{
    if (state_id == app->state)
        return 0;

    // get operators to get successors
    app->op_found = planSuccGenFind(succ_gen, state, app->op, app->op_size);
    app->op_preferred = 0;

    // remember the corresponding state
    app->state = state_id;
    return 1;
}
