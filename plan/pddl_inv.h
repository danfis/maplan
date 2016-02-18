/***
 * maplan
 * -------
 * Copyright (c)2016 Daniel Fiser <danfis@danfis.cz>,
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

#ifndef __PLAN_PDDL_INV_H__
#define __PLAN_PDDL_INV_H__

#include <boruvka/list.h>
#include <boruvka/htable.h>
#include <plan/pddl_ground.h>
#include <plan/lp.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_inv_t {
    int *fact;
    int size;
    bor_list_t list;
};
typedef struct _plan_pddl_inv_t plan_pddl_inv_t;

/**
 * Free all invariants from the list.
 */
void planPDDLInvFreeList(bor_list_t *list);

/**
 * Finds invariants in grounded problem and adds them in the output list
 * inv that should be already initialized using borListInit().
 * Returns number of found invariants.
 */
int planPDDLInvFind(const plan_pddl_ground_t *g, bor_list_t *inv);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_INV_H__ */
