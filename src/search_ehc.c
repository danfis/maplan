#include <boruvka/alloc.h>

#include "plan/search_ehc.h"

/** Computes heuristic value of the given state. */
static unsigned heuristic(plan_search_ehc_t *ehc,
                          plan_state_id_t state_id);

/** Fill ehc->succ_op[] with applicable operators in the given state.
 *  Returns how many operators were found. */
static size_t findApplicableOperators(plan_search_ehc_t *ehc,
                                      plan_state_id_t state_id);

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
    ehc->state_space = planStateSpaceFifoNew(plan->state_pool);
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
    if (ehc->state_space)
        planStateSpaceFifoDel(ehc->state_space);
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

    heur = heuristic(ehc, ehc->plan->initial_state);
    planStateSpaceFifoOpen2(ehc->state_space, ehc->plan->initial_state,
                            PLAN_NO_STATE, NULL, 0, heur);
    ehc->best_heur = heur;

    if (planStateIsGoal(ehc->plan, ehc->plan->initial_state)){
        planStateSpaceFifoCloseAll(ehc->state_space);
        ehc->goal_state = ehc->plan->initial_state;
        return 1;
    }

    return 0;
}

static int planSearchEHCStep(plan_search_ehc_t *ehc)
{
    plan_state_space_fifo_node_t *cur_node;
    plan_state_id_t cur_state_id, succ_state_id;
    plan_operator_t *op;
    unsigned succ_heur;
    size_t i, op_size;

    // get best node available so far
    cur_node = planStateSpaceFifoPop(ehc->state_space);
    if (cur_node == NULL){
        // we reached dead end
        return -1;
    }
    cur_state_id = cur_node->node.state_id;

    // Store applicable operators in ehc->succ_op[]
    op_size = findApplicableOperators(ehc, cur_state_id);

    // go trough all applicable operators
    for (i = 0; i < op_size; ++i){
        op = ehc->succ_op[i];

        // create a successor state
        succ_state_id = planOperatorApply(op, cur_state_id);

        // skip already visited states
        if (!planStateSpaceFifoNodeIsNew2(ehc->state_space, succ_state_id))
            continue;

        // compute heuristic of the successor state
        succ_heur = heuristic(ehc, succ_state_id);

        // check whether it is a goal
        if (planStateIsGoal(ehc->plan, succ_state_id)){
            planStateSpaceFifoOpen2(ehc->state_space, succ_state_id,
                                    cur_state_id, op,
                                    cur_node->node.cost + op->cost, succ_heur);
            planStateSpaceFifoCloseAll(ehc->state_space);
            ehc->goal_state = succ_state_id;
            return 1;
        }

        if (succ_heur < ehc->best_heur){
            // The discovered node is best so far. Clear open-list and
            // remember the best heuristic value.
            planStateSpaceFifoClear(ehc->state_space);
            ehc->best_heur = succ_heur;
        }

        // Insert node into open list
        planStateSpaceFifoOpen2(ehc->state_space, succ_state_id,
                                cur_state_id, op,
                                cur_node->node.cost + op->cost, succ_heur);
    }

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

static void extractPath(plan_search_ehc_t *ehc,
                        plan_state_id_t goal_state,
                        plan_path_t *path)
{
    plan_state_space_fifo_node_t *node;

    planPathInit(path);

    node = planStateSpaceFifoNode(ehc->state_space, goal_state);
    while (node && node->node.op){
        planPathPrepend(path, node->node.op);
        node = planStateSpaceFifoNode(ehc->state_space,
                                      node->node.parent_state_id);
    }
}
