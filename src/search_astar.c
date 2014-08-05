#include <boruvka/alloc.h>

#include "plan/search.h"
#include "plan/list.h"

struct _plan_search_astar_t {
    plan_search_t search;

    plan_list_t *list; /*!< Open-list */
    int pathmax;       /*!< Use pathmax correction */
};
typedef struct _plan_search_astar_t plan_search_astar_t;

#define SEARCH_FROM_PARENT(parent) \
    bor_container_of((parent), plan_search_astar_t, search)

/** Frees allocated resorces */
static void planSearchAStarDel(plan_search_t *_search);
/** Initializes search. This must be call exactly once. */
static int planSearchAStarInit(plan_search_t *_search);
/** Performes one step in the algorithm. */
static int planSearchAStarStep(plan_search_t *_search,
                               plan_search_step_change_t *change);
/** Injects a new state into open-list */
static int planSearchAStarInjectState(plan_search_t *_search,
                                      plan_state_id_t state_id,
                                      plan_cost_t cost,
                                      plan_cost_t heuristic);


void planSearchAStarParamsInit(plan_search_astar_params_t *p)
{
    bzero(p, sizeof(*p));
    planSearchParamsInit(&p->search);
}

plan_search_t *planSearchAStarNew(const plan_search_astar_params_t *params)
{
    plan_search_astar_t *search;

    search = BOR_ALLOC(plan_search_astar_t);

    _planSearchInit(&search->search, &params->search,
                    planSearchAStarDel,
                    planSearchAStarInit,
                    planSearchAStarStep,
                    planSearchAStarInjectState);

    search->list     = planListTieBreaking(2);
    search->pathmax  = params->pathmax;

    return &search->search;
}

static void planSearchAStarDel(plan_search_t *_search)
{
    plan_search_astar_t *search = SEARCH_FROM_PARENT(_search);

    _planSearchFree(&search->search);
    if (search->list)
        planListDel(search->list);
    BOR_FREE(search);
}

static void astarInsertState(plan_search_astar_t *search,
                             plan_state_id_t state_id,
                             plan_state_space_node_t *node,
                             plan_state_id_t parent_state_id,
                             plan_operator_t *op,
                             plan_cost_t g_cost,
                             plan_cost_t parent_h)
{
    plan_cost_t cost[2];
    plan_cost_t h;

    // Force to open the node and compute heuristic if necessary
    if (planStateSpaceNodeIsNew(node)){
        planStateSpaceOpen(search->search.state_space, node);
        h = _planSearchHeuristic(&search->search, state_id, NULL);

        if (search->pathmax && op != NULL){
            h = BOR_MAX(h, parent_h - op->cost);
        }

    }else{
        if (planStateSpaceNodeIsClosed(node)){
            planStateSpaceReopen(search->search.state_space, node);
        }

        // The node was already opened, so we have already computed
        // heuristic
        h = node->heuristic;
    }

    // Skip dead-end states
    if (h == PLAN_HEUR_DEAD_END)
        return;

    // Set up costs for open-list -- ties are broken by heuristic value
    cost[0] = g_cost + h; // f-value: f() = g() + h()
    cost[1] = h; // tie-breaking value

    // Set correct values
    node->state_id        = state_id;
    node->parent_state_id = parent_state_id;
    node->op              = op;
    node->cost            = g_cost;
    node->heuristic       = h;

    // Insert into open-list
    planListPush(search->list, cost, state_id);
    planSearchStatIncGeneratedStates(&search->search.stat);
}

static int planSearchAStarInit(plan_search_t *_search)
{
    plan_search_astar_t *search = SEARCH_FROM_PARENT(_search);
    plan_state_id_t init_state;
    plan_state_space_node_t *node;

    init_state = search->search.params.prob->initial_state;
    node = planStateSpaceNode(search->search.state_space, init_state);
    astarInsertState(search, init_state, node, 0, NULL, 0, 0);
    return PLAN_SEARCH_CONT;
}

static int planSearchAStarStep(plan_search_t *_search,
                             plan_search_step_change_t *change)
{
    plan_search_astar_t *search = SEARCH_FROM_PARENT(_search);
    plan_cost_t cost[2], g_cost;
    plan_state_id_t cur_state, next_state;
    plan_state_space_node_t *cur_node, *next_node;
    int i, op_size;
    plan_operator_t **op;

    if (change)
        planSearchStepChangeReset(change);

    // Get next state from open list
    if (planListPop(search->list, &cur_state, cost) != 0)
        return PLAN_SEARCH_NOT_FOUND;

    // Get corresponding state space node
    cur_node = planStateSpaceNode(search->search.state_space, cur_state);

    // Skip already closed nodes
    if (!planStateSpaceNodeIsOpen(cur_node))
        return PLAN_SEARCH_CONT;

    // Close the current state node
    planStateSpaceClose(search->search.state_space, cur_node);
    if (change)
        planSearchStepChangeAddClosedNode(change, cur_node);

    // Check whether it is a goal
    if (_planSearchCheckGoal(&search->search, cur_state))
        return PLAN_SEARCH_FOUND;

    // Find all applicable operators
    _planSearchFindApplicableOps(&search->search, cur_state);
    planSearchStatIncExpandedStates(&search->search.stat);

    // Add states created by applicable operators
    op      = search->search.applicable_ops.op;
    op_size = search->search.applicable_ops.op_found;
    for (i = 0; i < op_size; ++i){
        // Create a new state
        next_state = planOperatorApply(op[i], cur_state);
        // Compute its g() value
        g_cost = cur_node->cost + op[i]->cost;

        // Obtain corresponding node from state space
        next_node = planStateSpaceNode(search->search.state_space,
                                       next_state);

        // Decide whether to insert the state into open-list
        if (planStateSpaceNodeIsNew(next_node)
                || next_node->cost > g_cost){
            astarInsertState(search, next_state, next_node,
                             cur_state, op[i], g_cost, cur_node->heuristic);
        }
    }
    return PLAN_SEARCH_CONT;
}

static int planSearchAStarInjectState(plan_search_t *_search, plan_state_id_t state_id,
                                    plan_cost_t cost, plan_cost_t heuristic)
{
    plan_search_astar_t *search = SEARCH_FROM_PARENT(_search);
    plan_state_space_node_t *node;

    node = planStateSpaceNode(search->search.state_space, state_id);
    if (planStateSpaceNodeIsNew(node) || node->cost > cost){
        astarInsertState(search, state_id, node, 0, NULL, cost, 0);
        return 0;
    }

    return -1;
}
