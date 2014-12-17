#include <boruvka/alloc.h>

#include "plan/search.h"

struct _plan_search_ehc_t {
    plan_search_t search;

    plan_list_lazy_t *list; /*!< List to keep track of the states */
    int use_preferred_ops;  /*!< True if preffered operators should be used */
    plan_cost_t best_heur;  /*!< Value of the best heuristic value found so far */
    plan_search_applicable_ops_t *pref_ops; /*!< This pointer is set to
                                                 &search->applicable_ops in
                                                 case preferred operators
                                                 are enabled. */
};
typedef struct _plan_search_ehc_t plan_search_ehc_t;

#define EHC(parent) \
    bor_container_of((parent), plan_search_ehc_t, search)

/** Adds successors to the open list, the operators should already be
 *  filled in ehc->search.applicable_ops */
static void addSuccessors(plan_search_ehc_t *ehc, plan_state_id_t state_id);
/** Process the given state */
static int processState(plan_search_ehc_t *ehc,
                        plan_state_id_t cur_state_id,
                        plan_state_id_t parent_state_id,
                        plan_op_t *parent_op);

/** Frees allocated resorces */
static void planSearchEHCDel(plan_search_t *_ehc);
/** Initializes search. This must be call exactly once. */
static int planSearchEHCInit(plan_search_t *);
/** Performes one step in the algorithm. */
static int planSearchEHCStep(plan_search_t *);
/** Inserts node into open-list */
static void planSearchEHCInsertNode(plan_search_t *search,
                                    plan_state_space_node_t *node);


void planSearchEHCParamsInit(plan_search_ehc_params_t *p)
{
    bzero(p, sizeof(*p));
    planSearchParamsInit(&p->search);
}

plan_search_t *planSearchEHCNew(const plan_search_ehc_params_t *params)
{
    plan_search_ehc_t *ehc;

    ehc = BOR_ALLOC(plan_search_ehc_t);

    _planSearchInit(&ehc->search, &params->search,
                    planSearchEHCDel,
                    planSearchEHCInit,
                    planSearchEHCStep,
                    planSearchEHCInsertNode,
                    NULL);

    ehc->list              = planListLazyFifoNew();
    ehc->use_preferred_ops = params->use_preferred_ops;
    ehc->best_heur         = PLAN_COST_MAX;

    // Set up pointer to applicable operators for later
    ehc->pref_ops = NULL;
    if (ehc->use_preferred_ops){
        ehc->pref_ops = &ehc->search.app_ops;
    }

    return &ehc->search;
}

static void planSearchEHCDel(plan_search_t *_ehc)
{
    plan_search_ehc_t *ehc = EHC(_ehc);

    _planSearchFree(&ehc->search);
    if (ehc->list)
        planListLazyDel(ehc->list);
    BOR_FREE(ehc);
}

static int planSearchEHCInit(plan_search_t *search)
{
    plan_search_ehc_t *ehc = EHC(search);
    plan_state_id_t init_state;
    plan_state_space_node_t *node;
    int res;

    ehc->best_heur = PLAN_COST_MAX;

    init_state = ehc->search.initial_state;
    node = planStateSpaceNode(search->state_space, init_state);
    planStateSpaceOpen(search->state_space, node);
    planStateSpaceClose(search->state_space, node);
    node->parent_state_id = PLAN_NO_STATE;
    node->op = NULL;
    node->cost = 0;

    res = _planSearchHeuristic(&ehc->search, init_state, &node->heuristic, NULL);
    if (res != PLAN_SEARCH_CONT)
        return res;

    planListLazyPush(ehc->list, 0, init_state, NULL);
    return PLAN_SEARCH_CONT;
}

static plan_state_space_node_t *expandNode(plan_search_ehc_t *ehc,
                                           plan_state_id_t parent_state_id,
                                           plan_op_t *parent_op,
                                           int *r)
{
    plan_state_id_t cur_state_id;
    plan_state_space_node_t *cur_node, *parent_node;
    plan_cost_t cur_heur;
    int res;

    // Create a new state and check whether the state was already visited
    cur_state_id = planOpApply(parent_op, parent_state_id);
    cur_node = planStateSpaceNode(ehc->search.state_space, cur_state_id);
    if (!planStateSpaceNodeIsNew(cur_node)){
        *r = PLAN_SEARCH_CONT;
        return NULL;
    }

    // find applicable operators in the current state
    _planSearchFindApplicableOps(&ehc->search, cur_state_id);

    // compute heuristic value for the current node
    res = _planSearchHeuristic(&ehc->search, cur_state_id,
                               &cur_heur, ehc->pref_ops);
    if (res != PLAN_SEARCH_CONT){
        *r = res;
        return NULL;
    }

    // get parent node for path cost computation
    parent_node = planStateSpaceNode(ehc->search.state_space, parent_state_id);

    // Update current node's data
    planStateSpaceOpen(ehc->search.state_space, cur_node);
    planStateSpaceClose(ehc->search.state_space, cur_node);
    cur_node->parent_state_id = parent_state_id;
    cur_node->op = parent_op;
    cur_node->cost = parent_node->cost + parent_op->cost;
    cur_node->heuristic = cur_heur;

    return cur_node;
}

static int planSearchEHCStep(plan_search_t *search)
{
    plan_search_ehc_t *ehc = EHC(search);
    plan_state_id_t parent_state_id;
    plan_state_space_node_t *cur_node;
    plan_op_t *parent_op;
    int res;

    // get the next node in list
    if (planListLazyPop(ehc->list, &parent_state_id, &parent_op) != 0)
        return PLAN_SEARCH_NOT_FOUND;

    planSearchStatIncExpandedStates(&search->stat);

    if (parent_op){
        cur_node = _planSearchLazyExpandNode(&ehc->search, parent_state_id,
                                             parent_op, ehc->use_preferred_ops,
                                             &res);
        if (cur_node == NULL)
            return res;

    }else{
        cur_node = planStateSpaceNode(search->state_space, parent_state_id);
    }

    if (_planSearchCheckGoal(search, cur_node))
        return PLAN_SEARCH_FOUND;
    _planSearchExpandedNode(search, cur_node);

    if (cur_node->heuristic != PLAN_HEUR_DEAD_END){
        // If the heuristic for the current state is the best so far, restart
        // EHC algorithm with an empty list.
        if (cur_node->heuristic < ehc->best_heur){
            planListLazyClear(ehc->list);
            ehc->best_heur = cur_node->heuristic;
        }

        if (!parent_op){
            // If the current node wasn't generate we need to find
            // applicable operators and optionally the preferred operators.
            _planSearchFindApplicableOps(&ehc->search, cur_node->state_id);
            if (ehc->pref_ops){
                plan_cost_t h;
                _planSearchHeuristic(&ehc->search, cur_node->state_id,
                                     &h, ehc->pref_ops);
            }
        }
        _planSearchLazyAddSuccessors(search, cur_node->state_id, 0,
                                     ehc->list, ehc->use_preferred_ops);
    }

    return PLAN_SEARCH_CONT;
}

static void planSearchEHCInsertNode(plan_search_t *search,
                                    plan_state_space_node_t *node)
{
    plan_search_ehc_t *ehc = EHC(search);
    _planSearchLazyInsertNode(search, node, 0, ehc->list);
}
