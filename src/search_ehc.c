#include <boruvka/alloc.h>

#include "plan/search.h"

struct _plan_search_ehc_t {
    plan_search_t search;

    plan_list_lazy_t *list;          /*!< List to keep track of the states */
    plan_heur_t *heur;               /*!< Heuristic function */
    plan_cost_t best_heur;           /*!< Value of the best heuristic
                                          value found so far */
};
typedef struct _plan_search_ehc_t plan_search_ehc_t;

/** Frees allocated resorces */
static void planSearchEHCDel(void *_ehc);
/** Initializes search. This must be call exactly once. */
static int planSearchEHCInit(void *);
/** Performes one step in the algorithm. */
static int planSearchEHCStep(void *, plan_search_step_change_t *change);


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
                    planSearchEHCStep);

    ehc->list        = planListLazyFifoNew();
    ehc->heur        = params->heur;
    ehc->best_heur   = PLAN_COST_MAX;

    return &ehc->search;
}

static void planSearchEHCDel(void *_ehc)
{
    plan_search_ehc_t *ehc = _ehc;

    _planSearchFree(&ehc->search);
    if (ehc->list)
        planListLazyDel(ehc->list);
    BOR_FREE(ehc);
}

static int planSearchEHCInit(void *_ehc)
{
    plan_search_ehc_t *ehc = _ehc;
    plan_cost_t heur;

    // compute heuristic for the initial state
    heur = _planSearchHeuristic(&ehc->search,
                                ehc->search.params.prob->initial_state,
                                ehc->heur);
    ehc->best_heur = heur;

    // create a first node from the initial state
    _planSearchNodeOpenClose(&ehc->search,
                             ehc->search.params.prob->initial_state,
                             PLAN_NO_STATE, NULL, 0, heur);

    if (_planSearchCheckGoal(&ehc->search,
                             ehc->search.params.prob->initial_state)){
        return PLAN_SEARCH_FOUND;
    }

    // add recepies for successor nodes into list
    _planSearchAddLazySuccessors(&ehc->search,
                                 ehc->search.params.prob->initial_state,
                                 0, ehc->list);

    return PLAN_SEARCH_CONT;
}

static int planSearchEHCStep(void *_ehc, plan_search_step_change_t *change)
{
    plan_search_ehc_t *ehc = _ehc;
    plan_state_id_t parent_state_id, cur_state_id;
    plan_operator_t *parent_op;
    plan_cost_t cur_heur;
    plan_state_space_node_t *node;

    if (change)
        planSearchStepChangeReset(change);

    // get the next node in list
    if (planListLazyPop(ehc->list, &parent_state_id, &parent_op) != 0){
        // we reached dead end
        _planSearchReachedDeadEnd(&ehc->search);
        return PLAN_SEARCH_NOT_FOUND;
    }

    planSearchStatIncExpandedStates(&ehc->search.stat);

    // Create a new state and check whether the state was already visited
    if (_planSearchNewState(&ehc->search, parent_op, parent_state_id,
                            &cur_state_id, NULL) != 0)
        return PLAN_SEARCH_CONT;

    // compute heuristic value for the current node
    cur_heur = _planSearchHeuristic(&ehc->search, cur_state_id, ehc->heur);

    // open and close the node so we can trace the path from goal to the
    // initial state
    node = _planSearchNodeOpenClose(&ehc->search,
                                    cur_state_id, parent_state_id,
                                    parent_op, 0, cur_heur);
    if (change)
        planSearchStepChangeAddClosedNode(change, node);

    // check if the current state is the goal
    if (_planSearchCheckGoal(&ehc->search, cur_state_id))
        return PLAN_SEARCH_FOUND;

    // If the heuristic for the current state is the best so far, restart
    // EHC algorithm with an empty list.
    if (cur_heur < ehc->best_heur){
        planListLazyClear(ehc->list);
        ehc->best_heur = cur_heur;
    }

    _planSearchAddLazySuccessors(&ehc->search, cur_state_id, 0, ehc->list);

    return PLAN_SEARCH_CONT;
}
