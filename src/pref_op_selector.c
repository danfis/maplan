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

#include "pref_op_selector.h"

static int sortPreferredOpsByPtrCmp(const void *a, const void *b)
{
    const plan_op_t *op1 = *(const plan_op_t **)a;
    const plan_op_t *op2 = *(const plan_op_t **)b;
    return op1 - op2;
}

void planPrefOpSelectorInit(plan_pref_op_selector_t *sel,
                            plan_heur_res_t *pref_ops,
                            const plan_op_t *base_op)
{
    sel->pref_ops = pref_ops;
    sel->base_op  = (plan_op_t *)base_op;

    // Sort array of operators by their pointers.
    if (pref_ops->pref_op_size > 0){
        qsort(pref_ops->pref_op, pref_ops->pref_op_size,
              sizeof(plan_op_t *), sortPreferredOpsByPtrCmp);
    }

    // Set up cursors
    sel->cur = pref_ops->pref_op;
    sel->end = pref_ops->pref_op + pref_ops->pref_op_size;
    sel->ins = pref_ops->pref_op;
}

void planPrefOpSelectorSelect(plan_pref_op_selector_t *sel, int op_id)
{
    plan_op_t *op = sel->base_op + op_id;
    plan_op_t *op_swp;

    // Skip operators with lower ID
    for (; sel->cur < sel->end && *sel->cur < op; ++sel->cur);
    if (sel->cur == sel->end)
        return;

    if (*sel->cur == op){
        // If we found corresponding operator -- move it to the beggining
        // of the .op[] array.
        if (sel->ins != sel->cur){
            BOR_SWAP(*sel->ins, *sel->cur, op_swp);
        }
        ++sel->ins;
        ++sel->cur;
    }
}

void planPrefOpSelectorFinalize(plan_pref_op_selector_t *sel)
{
    // Compute number of preferred operators from .ins pointer
    sel->pref_ops->pref_size = sel->ins - sel->pref_ops->pref_op;
}
