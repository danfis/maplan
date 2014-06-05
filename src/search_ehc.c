#include <boruvka/alloc.h>

#include "plan/search_ehc.h"

static void printPlan(plan_search_ehc_t *ehc,
                      plan_state_id_t goal_state)
{
    plan_state_space_node_t *n;
    const char *name;

    n = planStateSpaceNode(ehc->state_space, goal_state);
    name = planStateSpaceNodeOpName(n);
    if (!name){
        return;
    }

    printPlan(ehc, planStateSpaceNodeParentStateId(n));
    fprintf(stdout, "(%s)\n", name);
}

plan_search_ehc_t *planSearchEHCNew(plan_t *plan)
{
    plan_search_ehc_t *ehc;

    ehc = BOR_ALLOC(plan_search_ehc_t);
    ehc->plan = plan;
    ehc->state_space = planStateSpaceNew(plan->state_pool);
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
        planStateSpaceDel(ehc->state_space);
    BOR_FREE(ehc);
}

void planSearchEHCInit(plan_search_ehc_t *ehc)
{
    unsigned heur;

    // TODO: check goal state
    planStatePoolGetState(ehc->plan->state_pool,
                          ehc->plan->initial_state,
                          ehc->state);
    heur = planHeuristicGoalCount(ehc->heur, ehc->state);
    planStateSpaceOpenNode(ehc->state_space,
                           ehc->plan->initial_state,
                           PLAN_NO_STATE,
                           NULL,
                           0);
    ehc->best_heur = heur;
    ehc->counter = 0;
}

int planSearchEHCStep(plan_search_ehc_t *ehc)
{
    plan_state_space_node_t *node;
    plan_state_id_t state_id, succ_state_id;
    plan_operator_t *op;
    unsigned succ_heur;
    size_t i, op_size;

    // get best node available so far
    node = planStateSpaceExtractMin(ehc->state_space);
    if (node == NULL){
        fprintf(stderr, "No more states to expand.\n");
        return -1;
    }
    state_id = planStateSpaceNodeStateId(node);

    // unroll the state into ehc->state struct
    planStatePoolGetState(ehc->plan->state_pool, state_id, ehc->state);

    // get operators to get successors
    op_size = planSuccGenFind(ehc->succ_gen, ehc->state, ehc->succ_op,
                              ehc->plan->op_size);

    // go trough all successors
    for (i = 0; i < op_size; ++i){
        op = ehc->succ_op[i];

        if (!planStatePoolPartStateIsSubset(ehc->plan->state_pool, op->pre,
                                            state_id)){
            fprintf(stderr, "ERR\n");
            return -1;
        }
        // create a successor state
        // TODO: Create function planOperator...
        succ_state_id = planStatePoolApplyPartState(ehc->plan->state_pool,
                                                    op->eff, state_id);

        // skip already visited states
        if (!planStateSpaceNodeIsNew2(ehc->state_space, succ_state_id)){
            fprintf(stderr, "continue %d\n",
                    planStateSpaceNodeIsOpen2(ehc->state_space,
                        succ_state_id));
            continue;
        }

        // compute heuristic of the successor state
        planStatePoolGetState(ehc->plan->state_pool, succ_state_id,
                              ehc->state);
        succ_heur = planHeuristicGoalCount(ehc->heur, ehc->state);

        // check whether it is a goal
        // TODO: Create function planCheckGoal(...)
        if (planStatePoolPartStateIsSubset(ehc->plan->state_pool,
                                           ehc->plan->goal,
                                           succ_state_id)){
            // TODO
            fprintf(stderr, "Found solution %d.\n", (int)succ_heur);
            planStateSpaceOpenNode(ehc->state_space, succ_state_id,
                                   state_id, op, ehc->counter++);
            planStateSpaceCloseNode(ehc->state_space, succ_state_id);
            printPlan(ehc, succ_state_id);
            
            return 1;
        }

        if (succ_heur < ehc->best_heur){
            // The discovered node is best so far. Clear open-list and
            // remember the best heuristic value.
            planStateSpaceClearOpenNodes(ehc->state_space);
            ehc->best_heur = succ_heur;
        }

        // Insert node into open list
        planStateSpaceOpenNode(ehc->state_space, succ_state_id,
                               state_id, op, ehc->counter++);
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
