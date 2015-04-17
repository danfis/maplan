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

struct _plan_search_ehc_t {
    plan_search_lazy_base_t lazy;
    plan_cost_t best_heur;  /*!< Value of the best heuristic value found so far */
};
typedef struct _plan_search_ehc_t plan_search_ehc_t;

#define LAZY(parent) \
    bor_container_of((parent), plan_search_lazy_base_t, search)
#define EHC(parent) \
    bor_container_of(LAZY(parent), plan_search_ehc_t, lazy)

/** Frees allocated resorces */
static void planSearchEHCDel(plan_search_t *_ehc);
/** Initializes search. This must be call exactly once. */
static int planSearchEHCInit(plan_search_t *);
/** Performes one step in the algorithm. */
static int planSearchEHCStep(plan_search_t *);


void planSearchEHCParamsInit(plan_search_ehc_params_t *p)
{
    bzero(p, sizeof(*p));
    planSearchParamsInit(&p->search);
}

plan_search_t *planSearchEHCNew(const plan_search_ehc_params_t *params)
{
    plan_search_ehc_t *ehc;

    ehc = BOR_ALLOC(plan_search_ehc_t);

    _planSearchInit(&ehc->lazy.search, &params->search,
                    planSearchEHCDel,
                    planSearchEHCInit,
                    planSearchEHCStep,
                    planSearchLazyBaseInsertNode,
                    NULL);

    // Note that lazy-fifo list ignores cost during insertion
    planSearchLazyBaseInit(&ehc->lazy, planListLazyFifoNew(), 1,
                           params->use_preferred_ops);

    ehc->best_heur = PLAN_COST_MAX;

    return &ehc->lazy.search;
}

static void planSearchEHCDel(plan_search_t *_ehc)
{
    plan_search_ehc_t *ehc = EHC(_ehc);

    _planSearchFree(&ehc->lazy.search);
    planSearchLazyBaseFree(&ehc->lazy);
    BOR_FREE(ehc);
}

static int planSearchEHCInit(plan_search_t *search)
{
    plan_search_ehc_t *ehc = EHC(search);
    ehc->best_heur = PLAN_COST_MAX;
    return planSearchLazyBaseInitStep(search);
}

static int planSearchEHCStep(plan_search_t *search)
{
    plan_search_ehc_t *ehc = EHC(search);
    plan_state_space_node_t *cur_node;
    int res;

    res = planSearchLazyBaseNext(&ehc->lazy, &cur_node);
    if (res != PLAN_SEARCH_CONT || cur_node == NULL)
        return res;

    if (cur_node->heuristic != PLAN_HEUR_DEAD_END){
        // If the heuristic for the current state is the best so far, restart
        // EHC algorithm with an empty list.
        if (cur_node->heuristic < ehc->best_heur){
            planListLazyClear(ehc->lazy.list);
            ehc->best_heur = cur_node->heuristic;
        }
        planSearchLazyBaseExpand(&ehc->lazy, cur_node);
    }

    return PLAN_SEARCH_CONT;
}
