#include <boruvka/alloc.h>

#include "plan/search.h"

struct _plan_search_lazy_t {
    plan_search_t search;

    plan_list_lazy_t *list; /*!< List to keep track of the states */
    int list_del;           /*!< True if .list should be deleted */
    int use_preferred_ops;  /*!< True if preferred operators from heuristic
                                 should be used. */
    plan_search_applicable_ops_t *pref_ops; /*!< This pointer is set to
                                                 &search->applicable_ops in
                                                 case preferred operators
                                                 are enabled. */
};
typedef struct _plan_search_lazy_t plan_search_lazy_t;

#define LAZY(parent) \
    bor_container_of((parent), plan_search_lazy_t, search)

static void addSuccessors(plan_search_lazy_t *lazy,
                          plan_state_id_t state_id,
                          plan_cost_t heur_val);

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

    lazy->use_preferred_ops = params->use_preferred_ops;
    lazy->list              = params->list;
    lazy->list_del          = params->list_del;

    lazy->pref_ops = NULL;
    if (lazy->use_preferred_ops)
        lazy->pref_ops = &lazy->search.app_ops;

    return &lazy->search;
}

static void planSearchLazyDel(plan_search_t *_lazy)
{
    plan_search_lazy_t *lazy = LAZY(_lazy);
    _planSearchFree(&lazy->search);
    if (lazy->list_del)
        planListLazyDel(lazy->list);
    BOR_FREE(lazy);
}

static int planSearchLazyInit(plan_search_t *search)
{
    plan_search_lazy_t *lazy = LAZY(search);
    return _planSearchLazyInitStep(search, lazy->list, 1);
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
        cur_node = _planSearchLazyExpandNode(&lazy->search, parent_state_id,
                                             parent_op, lazy->use_preferred_ops,
                                             &res);
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
            _planSearchFindApplicableOps(search, cur_node->state_id);
            if (lazy->pref_ops){
                plan_cost_t h;
                _planSearchHeuristic(&lazy->search, cur_node->state_id,
                                     &h, lazy->pref_ops);
            }
        }
        _planSearchLazyAddSuccessors(search, cur_node->state_id,
                                     cur_node->heuristic,
                                     lazy->list, lazy->use_preferred_ops);
    }

    return PLAN_SEARCH_CONT;
}

static void planSearchLazyInsertNode(plan_search_t *search,
                                     plan_state_space_node_t *node)
{
    plan_search_lazy_t *lazy = LAZY(search);
    _planSearchLazyInsertNode(search, node, 0, lazy->list);
}

