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

#include "plan/search.h"
#include "search_lazy_base.h"

typedef plan_search_lazy_base_t plan_search_lazy_t;

#define LAZY(parent) \
    bor_container_of((parent), plan_search_lazy_t, search)

static void planSearchLazyDel(plan_search_t *_lazy);
static int planSearchLazyStep(plan_search_t *_lazy);

void planSearchLazyParamsInit(plan_search_lazy_params_t *p)
{
    bzero(p, sizeof(*p));
    planSearchParamsInit(&p->search);
}

plan_search_t *planSearchLazyNew(const plan_search_lazy_params_t *params)
{
    plan_search_lazy_t *lazy;

    lazy = BOR_ALLOC(plan_search_lazy_t);
    _planSearchInit(&lazy->search, &params->search,
                    planSearchLazyDel,
                    planSearchLazyBaseInitStep,
                    planSearchLazyStep,
                    planSearchLazyBaseInsertNode,
                    NULL);
    planSearchLazyBaseInit(lazy, params->list, params->list_del,
                           params->use_preferred_ops);

    return &lazy->search;
}

static void planSearchLazyDel(plan_search_t *_lazy)
{
    plan_search_lazy_t *lazy = LAZY(_lazy);
    _planSearchFree(&lazy->search);
    planSearchLazyBaseFree(lazy);
    BOR_FREE(lazy);
}

static int planSearchLazyStep(plan_search_t *search)
{
    plan_search_lazy_t *lazy = LAZY(search);
    plan_state_space_node_t *cur_node;
    int res;

    res = planSearchLazyBaseNext(lazy, &cur_node);
    if (res != PLAN_SEARCH_CONT || cur_node == NULL)
        return res;

    if (cur_node->heuristic != PLAN_HEUR_DEAD_END){
        planSearchLazyBaseExpand(lazy, cur_node);
    }

    return PLAN_SEARCH_CONT;
}
