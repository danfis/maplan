#include <boruvka/alloc.h>

#include "plan/search_ehc.h"

/** Computes heuristic value of the given state. */
static unsigned heuristic(plan_search_ehc_t *ehc,
                          plan_state_id_t state_id)
{
    planStatePoolGetState(ehc->plan->state_pool, state_id, ehc->state);
    return planHeuristicGoalCount(ehc->heur, ehc->state);
}

/** Fill ehc->succ_op[] with applicable operators in the given state.
 *  Returns how many operators were found. */
static size_t findApplicableOperators(plan_search_ehc_t *ehc,
                                      plan_state_id_t state_id)
{
    // unroll the state into ehc->state struct
    planStatePoolGetState(ehc->plan->state_pool, state_id, ehc->state);

    // get operators to get successors
    return planSuccGenFind(ehc->succ_gen, ehc->state, ehc->succ_op,
                           ehc->plan->op_size);
}

static void printPlan(plan_search_ehc_t *ehc,
                      plan_state_id_t goal_state)
{
    plan_state_space_fifo_node_t *n;

    n = planStateSpaceFifoNode(ehc->state_space, goal_state);
    if (!n->op)
        return;

    printPlan(ehc, n->parent_state_id);
    fprintf(stdout, "(%s)\n", n->op->name);
}

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

void planSearchEHCInit(plan_search_ehc_t *ehc)
{
    // TODO: check goal state
    planStateSpaceFifoOpen2(ehc->state_space, ehc->plan->initial_state,
                            PLAN_NO_STATE, NULL);
    ehc->best_heur = heuristic(ehc, ehc->plan->initial_state);
}

int planSearchEHCStep(plan_search_ehc_t *ehc)
{
    plan_state_space_fifo_node_t *cur_node;
    plan_state_id_t succ_state_id;
    plan_operator_t *op;
    unsigned succ_heur;
    size_t i, op_size;

    // get best node available so far
    cur_node = planStateSpaceFifoPop(ehc->state_space);
    if (cur_node == NULL){
        // TODO
        fprintf(stderr, "No more states to expand.\n");
        return -1;
    }

    // Store applicable operators in ehc->succ_op[]
    op_size = findApplicableOperators(ehc, cur_node->state_id);

    // go trough all applicable operators
    for (i = 0; i < op_size; ++i){
        op = ehc->succ_op[i];

        // create a successor state
        succ_state_id = planOperatorApply(op, cur_node->state_id);

        // skip already visited states
        if (!planStateSpaceFifoNodeIsNew2(ehc->state_space, succ_state_id))
            continue;

        // compute heuristic of the successor state
        succ_heur = heuristic(ehc, succ_state_id);

        // check whether it is a goal
        if (planStateIsGoal(ehc->plan, succ_state_id)){
            // TODO
            fprintf(stderr, "Found solution %d.\n", (int)succ_heur);
            planStateSpaceFifoOpen2(ehc->state_space, succ_state_id,
                                    cur_node->state_id, op);
            planStateSpaceFifoCloseAll(ehc->state_space);
            printPlan(ehc, succ_state_id);
            
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
                                cur_node->state_id, op);
    }

    return 0;
}

int planSearchEHCRun(plan_search_ehc_t *ehc)
{
    planSearchEHCInit(ehc);
    while (planSearchEHCStep(ehc) == 0);
    // TODO: read plan out or report error/dead-end/etc...
    return 0;
}
