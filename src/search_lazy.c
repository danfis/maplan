#include <boruvka/alloc.h>
#include "plan/search_lazy.h"

plan_search_lazy_t *planSearchLazyNew(plan_problem_t *prob,
                                      plan_heur_t *heur,
                                      plan_list_lazy_t *list)
{
    plan_search_lazy_t *lazy;

    lazy = BOR_ALLOC(plan_search_lazy_t);
    lazy->prob = prob;
    lazy->heur = heur;
    lazy->state_space = planStateSpaceNew(prob->state_pool);
    lazy->list = list;
    lazy->state = planStateNew(prob->state_pool);
    lazy->succ_gen = planSuccGenNew(prob->op, prob->op_size);
    lazy->succ_op  = BOR_ALLOC_ARR(plan_operator_t *, prob->op_size);
    lazy->goal_state = PLAN_NO_STATE;

    return lazy;
}

void planSearchLazyDel(plan_search_lazy_t *lazy)
{
    if (lazy->succ_gen)
        planSuccGenDel(lazy->succ_gen);
    if (lazy->succ_op)
        BOR_FREE(lazy->succ_op);
    if (lazy->state)
        planStateDel(lazy->prob->state_pool, lazy->state);
    if (lazy->state_space)
        planStateSpaceDel(lazy->state_space);
    BOR_FREE(lazy);
}

static plan_cost_t heuristic(plan_search_lazy_t *lazy,
                             plan_state_id_t state_id)
{
    planStatePoolGetState(lazy->prob->state_pool, state_id, lazy->state);
    return planHeur(lazy->heur, lazy->state);
}

static int planSearchLazyInit(plan_search_lazy_t *lazy)
{
    planListLazyPush(lazy->list, 0, lazy->prob->initial_state, NULL);
    return 0;
}

static int findApplicableOperators(plan_search_lazy_t *lazy,
                                   plan_state_id_t state_id)
{
    // unroll the state into lazy->state struct
    planStatePoolGetState(lazy->prob->state_pool, state_id, lazy->state);

    // get operators to get successors
    return planSuccGenFind(lazy->succ_gen, lazy->state, lazy->succ_op,
                           lazy->prob->op_size);
}

static void addSuccessors(plan_search_lazy_t *lazy,
                          plan_state_id_t cur_state_id,
                          plan_cost_t heur)
{
    int i, op_size;
    plan_operator_t *op;

    // Store applicable operators in lazy->succ_op[]
    op_size = findApplicableOperators(lazy, cur_state_id);

    // go trough all applicable operators
    for (i = 0; i < op_size; ++i){
        op = lazy->succ_op[i];
        planListLazyPush(lazy->list, heur, cur_state_id, op);
    }
}

static int planSearchLazyStep(plan_search_lazy_t *lazy)
{
    plan_state_id_t parent_state_id, cur_state_id;
    plan_operator_t *parent_op;
    plan_cost_t cur_heur;
    plan_state_space_node_t *cur_node;

    // get next node from the list
    if (planListLazyPop(lazy->list, &parent_state_id, &parent_op) != 0){
        // reached dead-end
        fprintf(stderr, "DEAD\n");
        return -1;
    }

    if (parent_op == NULL){
        // use parent state as current state
        cur_state_id = parent_state_id;
        parent_state_id = PLAN_NO_STATE;
    }else{
        // create a new state
        cur_state_id = planOperatorApply(parent_op, parent_state_id);
    }


    // get the node corresponding to the current state
    cur_node = planStateSpaceNode(lazy->state_space, cur_state_id);

    // check whether the state was already visited
    if (!planStateSpaceNodeIsNew(cur_node))
        return 0;

    // compute heuristic value for the current node
    cur_heur = heuristic(lazy, cur_state_id);

    // open and close the node so we can trace the path from goal to the
    // initial state
    cur_node = planStateSpaceOpen2(lazy->state_space, cur_state_id,
                                   parent_state_id, parent_op,
                                   0, cur_heur);
    planStateSpaceClose(lazy->state_space, cur_node);

    // check if the current state is the goal
    if (planProblemCheckGoal(lazy->prob, cur_state_id)){
        lazy->goal_state = cur_state_id;
        return 1;
    }

    addSuccessors(lazy, cur_state_id, cur_heur);

    return 0;
}

static void extractPath(plan_search_lazy_t *lazy,
                        plan_state_id_t goal_state,
                        plan_path_t *path)
{
    plan_state_space_node_t *node;

    planPathInit(path);

    node = planStateSpaceNode(lazy->state_space, goal_state);
    while (node && node->op){
        planPathPrepend(path, node->op);
        node = planStateSpaceNode(lazy->state_space, node->parent_state_id);
    }
}

int planSearchLazyRun(plan_search_lazy_t *lazy, plan_path_t *path)
{
    int res;

    if ((res = planSearchLazyInit(lazy)) == 0){
        while ((res = planSearchLazyStep(lazy)) == 0);
    }

    // Reached dead end:
    if (res == -1)
        return 1;

    extractPath(lazy, lazy->goal_state, path);
    return 0;
}
