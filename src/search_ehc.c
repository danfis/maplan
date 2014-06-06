#include <boruvka/alloc.h>

#include "plan/search_ehc.h"

/** Computes heuristic value of the given state. */
static unsigned heuristic(plan_search_ehc_t *ehc,
                          plan_state_id_t state_id);

/** Fill ehc->succ_op[] with applicable operators in the given state.
 *  Returns how many operators were found. */
static size_t findApplicableOperators(plan_search_ehc_t *ehc,
                                      plan_state_id_t state_id);

/** Insert successor elements into the list. */
static void addSuccessors(plan_search_ehc_t *ehc,
                          plan_state_id_t cur_state_id);

/** Initializes search. This must be call exactly once. */
static int planSearchEHCInit(plan_search_ehc_t *ehc);
/** Performes one step in the algorithm. */
static int planSearchEHCStep(plan_search_ehc_t *ehc);

/** Extracts a path from the initial state to the goal state as found by
 *  EHC */
static void extractPath(plan_search_ehc_t *ehc,
                        plan_state_id_t goal_state,
                        plan_path_t *path);

plan_search_ehc_t *planSearchEHCNew(plan_t *plan)
{
    plan_search_ehc_t *ehc;

    ehc = BOR_ALLOC(plan_search_ehc_t);
    ehc->plan = plan;
    ehc->state_space = planStateSpaceNew(plan->state_pool);
    ehc->list = planListLazyFifoNew();
    ehc->heur = planHeuristicGoalCountNew(plan->goal);
    ehc->state = planStateNew(plan->state_pool);
    ehc->succ_gen = planSuccGenNew(plan->op, plan->op_size);
    ehc->succ_op  = BOR_ALLOC_ARR(plan_operator_t *, plan->op_size);
    ehc->best_heur = -1;
    ehc->goal_state = PLAN_NO_STATE;
    return ehc;
}

void planSearchEHCDel(plan_search_ehc_t *ehc)
{
    if (ehc->succ_gen)
        planSuccGenDel(ehc->succ_gen);
    if (ehc->succ_op)
        BOR_FREE(ehc->succ_op);
    if (ehc->state)
        planStateDel(ehc->plan->state_pool, ehc->state);
    if (ehc->heur)
        planHeuristicGoalCountDel(ehc->heur);
    if (ehc->list)
        planListLazyFifoDel(ehc->list);
    if (ehc->state_space)
        planStateSpaceDel(ehc->state_space);
    BOR_FREE(ehc);
}

int planSearchEHCRun(plan_search_ehc_t *ehc, plan_path_t *path)
{
    int res;

    if ((res = planSearchEHCInit(ehc)) == 0){
        while ((res = planSearchEHCStep(ehc)) == 0);
    }

    // Reached dead end:
    if (res == -1)
        return 1;

    extractPath(ehc, ehc->goal_state, path);
    return 0;
}

static int planSearchEHCInit(plan_search_ehc_t *ehc)
{
    unsigned heur;
    plan_state_space_node_t *node;

    // compute heuristic for the initial state
    heur = heuristic(ehc, ehc->plan->initial_state);
    ehc->best_heur = heur;

    // create a first node from the initial state
    node = planStateSpaceOpen2(ehc->state_space,
                               ehc->plan->initial_state,
                               PLAN_NO_STATE, NULL, 0, heur);
    planStateSpaceClose(ehc->state_space, node);

    if (planStateIsGoal(ehc->plan, ehc->plan->initial_state)){
        ehc->goal_state = ehc->plan->initial_state;
        return 1;
    }

    // add recepies for successor nodes into list
    addSuccessors(ehc, ehc->plan->initial_state);

    return 0;
}

static int planSearchEHCStep(plan_search_ehc_t *ehc)
{
    plan_state_id_t parent_state_id, cur_state_id;
    plan_operator_t *parent_op;
    plan_state_space_node_t *cur_node;
    unsigned cur_heur;

    // get the next node in list
    if (planListLazyFifoPop(ehc->list, &parent_state_id, &parent_op) != 0){
        // we reached dead end
        return -1;
    }

    // create a new state
    cur_state_id = planOperatorApply(parent_op, parent_state_id);
    cur_node = planStateSpaceNode(ehc->state_space, cur_state_id);

    // check whether the state was already visited
    if (!planStateSpaceNodeIsNew(cur_node))
        return 0;

    // compute heuristic value for the current node
    cur_heur = heuristic(ehc, cur_state_id);

    // open and close the node so we can trace the path from goal to the
    // initial state
    cur_node = planStateSpaceOpen2(ehc->state_space, cur_state_id,
                                   parent_state_id, parent_op,
                                   0, cur_heur);
    planStateSpaceClose(ehc->state_space, cur_node);

    // check if the current state is the goal
    if (planStateIsGoal(ehc->plan, cur_state_id)){
        ehc->goal_state = cur_state_id;
        return 1;
    }

    // If the heuristic for the current state is the best so far, restart
    // EHC algorithm with an empty list.
    if (cur_heur < ehc->best_heur){
        planListLazyFifoClear(ehc->list);
        ehc->best_heur = cur_heur;
    }

    addSuccessors(ehc, cur_state_id);

    return 0;
}

static unsigned heuristic(plan_search_ehc_t *ehc,
                          plan_state_id_t state_id)
{
    planStatePoolGetState(ehc->plan->state_pool, state_id, ehc->state);
    return planHeuristicGoalCount(ehc->heur, ehc->state);
}

static size_t findApplicableOperators(plan_search_ehc_t *ehc,
                                      plan_state_id_t state_id)
{
    // unroll the state into ehc->state struct
    planStatePoolGetState(ehc->plan->state_pool, state_id, ehc->state);

    // get operators to get successors
    return planSuccGenFind(ehc->succ_gen, ehc->state, ehc->succ_op,
                           ehc->plan->op_size);
}

static void addSuccessors(plan_search_ehc_t *ehc,
                          plan_state_id_t cur_state_id)
{
    size_t i, op_size;
    plan_operator_t *op;

    // Store applicable operators in ehc->succ_op[]
    op_size = findApplicableOperators(ehc, cur_state_id);

    // go trough all applicable operators
    for (i = 0; i < op_size; ++i){
        op = ehc->succ_op[i];
        planListLazyFifoPush(ehc->list, cur_state_id, op);
    }
}

static void extractPath(plan_search_ehc_t *ehc,
                        plan_state_id_t goal_state,
                        plan_path_t *path)
{
    plan_state_space_node_t *node;

    planPathInit(path);

    node = planStateSpaceNode(ehc->state_space, goal_state);
    while (node && node->op){
        planPathPrepend(path, node->op);
        node = planStateSpaceNode(ehc->state_space, node->parent_state_id);
    }
}
