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

/** Initializes search. This must be call exactly once. */
static int planSearchEHCInit(plan_search_ehc_t *ehc);
/** Performes one step in the algorithm. */
static int planSearchEHCStep(plan_search_ehc_t *ehc);

/** Extracts a path from the initial state to the goal state as found by
 *  EHC */
static void extractPath(plan_search_ehc_t *ehc,
                        plan_state_id_t goal_state,
                        plan_path_t *path);

void planSearchEHCParamsInit(plan_search_ehc_params_t *p)
{
    bzero(p, sizeof(*p));
}

plan_search_ehc_t *planSearchEHCNew(const plan_search_ehc_params_t *params)
{
    plan_search_ehc_t *ehc;

    ehc = BOR_ALLOC(plan_search_ehc_t);
    ehc->params = *params;

    planSearchStatInit(&ehc->stat);

    ehc->prob        = params->prob;
    ehc->state_space = planStateSpaceNew(ehc->prob->state_pool);
    ehc->list        = planListLazyFifoNew();
    ehc->heur        = params->heur;
    ehc->state       = planStateNew(ehc->prob->state_pool);
    ehc->succ_gen    = planSuccGenNew(ehc->prob->op, ehc->prob->op_size);
    ehc->succ_op     = BOR_ALLOC_ARR(plan_operator_t *, ehc->prob->op_size);
    ehc->best_heur   = PLAN_COST_MAX;
    ehc->goal_state  = PLAN_NO_STATE;

    return ehc;
}

void planSearchEHCDel(plan_search_ehc_t *ehc)
{
    if (ehc->succ_gen)
        planSuccGenDel(ehc->succ_gen);
    if (ehc->succ_op)
        BOR_FREE(ehc->succ_op);
    if (ehc->state)
        planStateDel(ehc->prob->state_pool, ehc->state);
    if (ehc->list)
        planListLazyDel(ehc->list);
    if (ehc->state_space)
        planStateSpaceDel(ehc->state_space);
    BOR_FREE(ehc);
}

int planSearchEHCRun(plan_search_ehc_t *ehc, plan_path_t *path)
{
    int res;
    long steps = 0;

    borTimerStart(&ehc->timer);
    if ((res = planSearchEHCInit(ehc)) == 0){
        while ((res = planSearchEHCStep(ehc)) == 0){
            ++steps;
            if (ehc->params.progress_fn
                    && steps == ehc->params.progress_freq){
                ehc->stat.steps += steps;

                borTimerStop(&ehc->timer);
                ehc->stat.elapsed_time = borTimerElapsedInSF(&ehc->timer);
                planSearchStatUpdatePeakMemory(&ehc->stat);
                if (ehc->params.progress_fn(&ehc->stat) != PLAN_SEARCH_CONTINUE){
                    res = -1;
                    break;
                }
                steps = 0;
            }
        }
    }

    if (ehc->params.progress_fn){
        ehc->stat.steps += steps;
        ehc->stat.elapsed_time = borTimerElapsedInSF(&ehc->timer);
        planSearchStatUpdatePeakMemory(&ehc->stat);
        ehc->params.progress_fn(&ehc->stat);
    }

    // Reached dead end:
    if (res == -1)
        return 1;

    extractPath(ehc, ehc->goal_state, path);
    return 0;
}

static int planSearchEHCInit(plan_search_ehc_t *ehc)
{
    plan_cost_t heur;
    plan_state_space_node_t *node;

    // compute heuristic for the initial state
    heur = heuristic(ehc, ehc->prob->initial_state);
    ehc->best_heur = heur;

    // create a first node from the initial state
    node = planStateSpaceOpen2(ehc->state_space,
                               ehc->prob->initial_state,
                               PLAN_NO_STATE, NULL, 0, heur);
    planStateSpaceClose(ehc->state_space, node);

    if (planProblemCheckGoal(ehc->prob, ehc->prob->initial_state)){
        ehc->goal_state = ehc->prob->initial_state;
        return 1;
    }

    // add recepies for successor nodes into list
    addSuccessors(ehc, ehc->prob->initial_state);

    return 0;
}

static int planSearchEHCStep(plan_search_ehc_t *ehc)
{
    plan_state_id_t parent_state_id, cur_state_id;
    plan_operator_t *parent_op;
    plan_state_space_node_t *cur_node;
    plan_cost_t cur_heur;

    // get the next node in list
    if (planListLazyPop(ehc->list, &parent_state_id, &parent_op) != 0){
        // we reached dead end
        ehc->stat.dead_end = 1;
        return -1;
    }

    // create a new state
    cur_state_id = planOperatorApply(parent_op, parent_state_id);
    cur_node = planStateSpaceNode(ehc->state_space, cur_state_id);
    ++ehc->stat.generated_states;

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
    if (planProblemCheckGoal(ehc->prob, cur_state_id)){
        ehc->goal_state = cur_state_id;
        ehc->stat.found_solution = 1;
        return 1;
    }

    // If the heuristic for the current state is the best so far, restart
    // EHC algorithm with an empty list.
    if (cur_heur < ehc->best_heur){
        planListLazyClear(ehc->list);
        ehc->best_heur = cur_heur;
    }

    ++ehc->stat.expanded_states;
    addSuccessors(ehc, cur_state_id);

    return 0;
}

static plan_cost_t heuristic(plan_search_ehc_t *ehc,
                             plan_state_id_t state_id)
{
    planStatePoolGetState(ehc->prob->state_pool, state_id, ehc->state);
    ++ehc->stat.evaluated_states;
    return planHeur(ehc->heur, ehc->state);
}

static int findApplicableOperators(plan_search_ehc_t *ehc,
                                   plan_state_id_t state_id)
{
    // unroll the state into ehc->state struct
    planStatePoolGetState(ehc->prob->state_pool, state_id, ehc->state);

    // get operators to get successors
    return planSuccGenFind(ehc->succ_gen, ehc->state, ehc->succ_op,
                           ehc->prob->op_size);
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
        op = ehc->succ_op[i];
        planListLazyPush(ehc->list, 0, cur_state_id, op);
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
