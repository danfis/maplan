#include <boruvka/alloc.h>

#include "plan/search.h"
#include "search_lazy_base.h"

typedef plan_search_lazy_base_t plan_search_lazy_t;

#define LAZY(parent) \
    bor_container_of((parent), plan_search_lazy_t, search)

static void planSearchLazyDel(plan_search_t *_lazy);
static int planSearchLazyInit(plan_search_t *_lazy);
static int planSearchLazyStep(plan_search_t *_lazy);
static void planSearchLazyInsertNode(plan_search_t *search,
                                     plan_state_space_node_t *node);

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
                    planSearchLazyInit,
                    planSearchLazyStep,
                    planSearchLazyInsertNode,
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

static int planSearchLazyInit(plan_search_t *search)
{
    plan_search_lazy_t *lazy = LAZY(search);
    return planSearchLazyBaseInitStep(lazy, 1);
}

static int planSearchLazyStep(plan_search_t *search)
{
    plan_search_lazy_t *lazy = LAZY(search);
    plan_state_id_t parent_state_id;
    plan_op_t *parent_op;
    plan_state_space_node_t *cur_node;
    int res;

    // get next node from the list
    if (planListLazyPop(lazy->list, &parent_state_id, &parent_op) != 0)
        return PLAN_SEARCH_NOT_FOUND;

    if (parent_op){
        cur_node = planSearchLazyBaseExpandNode(lazy, parent_state_id,
                                                parent_op, &res);
        if (cur_node == NULL)
            return res;

    }else{
        cur_node = planStateSpaceNode(search->state_space, parent_state_id);
    }

    // check if the current state is the goal
    if (_planSearchCheckGoal(&lazy->search, cur_node))
        return PLAN_SEARCH_FOUND;
    _planSearchExpandedNode(search, cur_node);

    if (cur_node->heuristic != PLAN_HEUR_DEAD_END){
        if (!parent_op){
            // TODO: Remove this
            _planSearchFindApplicableOps(search, cur_node->state_id);
            /*
            if (lazy->use_preferred_ops){
                plan_cost_t h;
                _planSearchHeuristic(&lazy->search, cur_node->state_id,
                                     &h, lazy->pref_ops);
            }
            */
        }
        planSearchLazyBaseAddSuccessors(lazy, cur_node->state_id,
                                        cur_node->heuristic);
    }

    return PLAN_SEARCH_CONT;
}

static void planSearchLazyInsertNode(plan_search_t *search,
                                     plan_state_space_node_t *node)
{
    plan_search_lazy_t *lazy = LAZY(search);
    planSearchLazyBaseInsertNode(lazy, node, node->heuristic);
}

