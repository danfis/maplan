#include <boruvka/alloc.h>

#include "plan/search_ehc.h"

/** Computes heuristic value of the given state. */
static plan_cost_t heuristic(plan_search_ehc_t *ehc,
                             plan_state_id_t state_id);

/** Fill ehc->succ_op[] with applicable operators in the given state.
 *  Returns how many operators were found. */
static int findApplicableOperators(plan_search_ehc_t *ehc,
                                   plan_state_id_t state_id);

/** Insert successor elements into the list. */
static void addSuccessors(plan_search_ehc_t *ehc,
                          plan_state_id_t cur_state_id);

/** Frees allocated resorces */
static void planSearchEHCDel(void *_ehc);
/** Initializes search. This must be call exactly once. */
static int planSearchEHCInit(void *);
/** Performes one step in the algorithm. */
static int planSearchEHCStep(void *);


void planSearchEHCParamsInit(plan_search_ehc_params_t *p)
{
    bzero(p, sizeof(*p));
    planSearchParamsInit(&p->search);
}

plan_search_t *planSearchEHCNew(const plan_search_ehc_params_t *params)
{
    plan_search_ehc_t *ehc;

    ehc = BOR_ALLOC(plan_search_ehc_t);

    planSearchInit(&ehc->search, &params->search,
                   planSearchEHCDel,
                   planSearchEHCInit,
                   planSearchEHCStep);

    ehc->list        = planListLazyFifoNew();
    ehc->heur        = params->heur;
    ehc->state       = planStateNew(ehc->search.params.prob->state_pool);
    ehc->best_heur   = PLAN_COST_MAX;

    return &ehc->search;
}

static void planSearchEHCDel(void *_ehc)
{
    plan_search_ehc_t *ehc = _ehc;

    planSearchFree(&ehc->search);
    if (ehc->state)
        planStateDel(ehc->search.params.prob->state_pool, ehc->state);
    if (ehc->list)
        planListLazyDel(ehc->list);
    BOR_FREE(ehc);
}

static int planSearchEHCInit(void *_ehc)
{
    plan_search_ehc_t *ehc = _ehc;
    plan_cost_t heur;
    plan_state_space_node_t *node;

    // compute heuristic for the initial state
    heur = heuristic(ehc, ehc->search.params.prob->initial_state);
    ehc->best_heur = heur;

    // create a first node from the initial state
    node = planStateSpaceOpen2(ehc->search.state_space,
                               ehc->search.params.prob->initial_state,
                               PLAN_NO_STATE, NULL, 0, heur);
    planStateSpaceClose(ehc->search.state_space, node);

    if (planProblemCheckGoal(ehc->search.params.prob,
                             ehc->search.params.prob->initial_state)){
        ehc->search.goal_state = ehc->search.params.prob->initial_state;
        return 1;
    }

    // add recepies for successor nodes into list
    addSuccessors(ehc, ehc->search.params.prob->initial_state);

    return 0;
}

static int planSearchEHCStep(void *_ehc)
{
    plan_search_ehc_t *ehc = _ehc;
    plan_state_id_t parent_state_id, cur_state_id;
    plan_operator_t *parent_op;
    plan_state_space_node_t *cur_node;
    plan_cost_t cur_heur;

    // get the next node in list
    if (planListLazyPop(ehc->list, &parent_state_id, &parent_op) != 0){
        // we reached dead end
        planSearchStatSetDeadEnd(&ehc->search.stat);
        return -1;
    }

    // create a new state
    cur_state_id = planOperatorApply(parent_op, parent_state_id);
    cur_node = planStateSpaceNode(ehc->search.state_space, cur_state_id);
    planSearchStatIncGeneratedStates(&ehc->search.stat);

    // check whether the state was already visited
    if (!planStateSpaceNodeIsNew(cur_node))
        return 0;

    // compute heuristic value for the current node
    cur_heur = heuristic(ehc, cur_state_id);

    // open and close the node so we can trace the path from goal to the
    // initial state
    cur_node = planStateSpaceOpen2(ehc->search.state_space, cur_state_id,
                                   parent_state_id, parent_op,
                                   0, cur_heur);
    planStateSpaceClose(ehc->search.state_space, cur_node);

    // check if the current state is the goal
    if (planProblemCheckGoal(ehc->search.params.prob, cur_state_id)){
        ehc->search.goal_state = cur_state_id;
        planSearchStatSetFoundSolution(&ehc->search.stat);
        return 1;
    }

    // If the heuristic for the current state is the best so far, restart
    // EHC algorithm with an empty list.
    if (cur_heur < ehc->best_heur){
        planListLazyClear(ehc->list);
        ehc->best_heur = cur_heur;
    }

    planSearchStatIncExpandedStates(&ehc->search.stat);
    addSuccessors(ehc, cur_state_id);

    return 0;
}

static plan_cost_t heuristic(plan_search_ehc_t *ehc,
                             plan_state_id_t state_id)
{
    planStatePoolGetState(ehc->search.params.prob->state_pool,
                          state_id, ehc->state);
    planSearchStatIncEvaluatedStates(&ehc->search.stat);
    return planHeur(ehc->heur, ehc->state);
}

static int findApplicableOperators(plan_search_ehc_t *ehc,
                                   plan_state_id_t state_id)
{
    // unroll the state into ehc->state struct
    planStatePoolGetState(ehc->search.params.prob->state_pool,
                          state_id, ehc->state);

    // get operators to get successors
    return planSuccGenFind(ehc->search.params.prob->succ_gen,
                           ehc->state, ehc->search.succ_op,
                           ehc->search.params.prob->op_size);
}

static void addSuccessors(plan_search_ehc_t *ehc,
                          plan_state_id_t cur_state_id)
{
    int i, op_size;
    plan_operator_t *op;

    // Store applicable operators in ehc->succ_op[]
    op_size = findApplicableOperators(ehc, cur_state_id);

    // go trough all applicable operators
    for (i = 0; i < op_size; ++i){
        op = ehc->search.succ_op[i];
        planListLazyPush(ehc->list, 0, cur_state_id, op);
    }
}
