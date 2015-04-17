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

#include "plan/list_lazy.h"

void planListLazyInit(plan_list_lazy_t *l,
                      plan_list_lazy_del_fn del_fn,
                      plan_list_lazy_push_fn push_fn,
                      plan_list_lazy_pop_fn pop_fn,
                      plan_list_lazy_clear_fn clear_fn)
{
    l->del_fn   = del_fn;
    l->push_fn  = push_fn;
    l->pop_fn   = pop_fn;
    l->clear_fn = clear_fn;
}

void planListLazyFree(plan_list_lazy_t *l)
{
}
