#include <pthread.h>

#include "plan/ma_search.h"

/** Maximal time (in ms) for which is multi-agent thread blocked when
 *  dead-end is reached. This constant is here basicaly to prevent
 *  dead-lock when all threads are in dead-end. So, it should be set to
 *  fairly high number. */
#define SEARCH_MA_MAX_BLOCK_TIME (600 * 1000) // 10 minutes


/**** TODO: A* --> refactor ****/
#include "plan/list.h"
#include "plan/statespace.h"
#include "search_applicable_ops.h"
struct _plan_search_astar_t {
    plan_state_id_t initial_state;
    const plan_part_state_t *goal;
    plan_state_pool_t *state_pool;
    plan_state_space_t *state_space;
    plan_state_t *state;
    plan_state_id_t state_id;
    plan_heur_t *heur;
    const plan_succ_gen_t *succ_gen;

    plan_search_applicable_ops_t app_ops;

    plan_list_t *list; /*!< Open-list */
    int pathmax;       /*!< Use pathmax correction */
};
typedef struct _plan_search_astar_t plan_search_astar_t;

static void astarInit(plan_search_astar_t *search,
                      plan_state_id_t initial_state,
                      const plan_part_state_t *goal,
                      plan_state_pool_t *state_pool,
                      plan_heur_t *heur,
                      const plan_succ_gen_t *succ_gen);
static void astarFree(plan_search_astar_t *search);
static int astarInitStep(plan_search_astar_t *search);
static int astarStep(plan_search_astar_t *search);



struct _plan_ma_search_th_t {
    pthread_t thread; /*!< Thread with running search algorithm */

    // TODO: Replace this with some lock-free option!
    bor_fifo_t msg_queue;
    pthread_mutex_t msg_queue_lock;

    int final_state; /*!< State in which the algorithm ended */

    plan_search_astar_t search;
};
typedef struct _plan_ma_search_th_t plan_ma_search_th_t;

/** Initializes and frees thread structure */
static void searchThreadInit(plan_ma_search_th_t *th, plan_ma_search_t *search);
static void searchThreadFree(plan_ma_search_th_t *th);
/** Main search-thread function */
static void *searchThread(void *);
static int searchThreadInitStep(plan_ma_search_th_t *th);
static int searchThreadStep(plan_ma_search_th_t *th);
static int searchThreadProcessMsgs(plan_ma_search_th_t *th);

void planMASearchParamsInit(plan_ma_search_params_t *params)
{
    bzero(params, sizeof(*params));
}

void planMASearchInit(plan_ma_search_t *search,
                      const plan_ma_search_params_t *params)
{
    search->comm = params->comm;
    search->terminated = 0;

    search->params = params;
}

void planMASearchFree(plan_ma_search_t *search)
{
}

int planMASearchRun(plan_ma_search_t *search, plan_path_t *path)
{
    plan_ma_search_th_t search_thread;
    plan_ma_msg_t *msg;

    astarInit(&search_thread.search,
              search->params->initial_state,
              search->params->goal,
              search->params->state_pool,
              search->params->heur,
              search->params->succ_gen);

    // Initialize search structure
    search->terminated = 0;

    // Initialize thread with search algorithm
    searchThreadInit(&search_thread, search);

    // Run search-thread
    pthread_create(&search_thread.thread, NULL, searchThread,
                   &search_thread);

    // Process all messages
    while (!search->terminated){
        // Wait for next message
        msg = planMACommRecvBlockTimeout(search->comm, SEARCH_MA_MAX_BLOCK_TIME);
        if (msg == NULL){
            // Timeout -- terminate ma-search
            search->terminated = 1;
        }else{
            // TODO: Call handler
            planMAMsgDel(msg);
        }
    }

    // Wait for search-thread to finish
    pthread_join(search_thread.thread, NULL);

    // Free resources allocated for search thread
    searchThreadFree(&search_thread);

    astarFree(&search_thread.search);
    return PLAN_SEARCH_ABORT;
}


static void searchThreadInit(plan_ma_search_th_t *th, plan_ma_search_t *search)
{
    borFifoInit(&th->msg_queue, sizeof(plan_ma_msg_t *));
    pthread_mutex_init(&th->msg_queue_lock, NULL);
}

static void searchThreadFree(plan_ma_search_th_t *th)
{
    borFifoFree(&th->msg_queue);
    pthread_mutex_destroy(&th->msg_queue_lock);
}

static void *searchThread(void *_th)
{
    plan_ma_search_th_t *th = (plan_ma_search_th_t *)_th;
    int state;

    state = searchThreadInitStep(th);
    while (state == PLAN_SEARCH_CONT){
        state = searchThreadProcessMsgs(th);
        if (state == PLAN_SEARCH_CONT)
            state = searchThreadStep(th);
    }

    // Remember the final state
    th->final_state = state;
    fprintf(stderr, "Final state: %d\n", state);
    fflush(stderr);
    // TODO: Let the parent thread know that we ended (using some internal
    // message or by closing communication channel)

    return NULL;
}

static int searchThreadInitStep(plan_ma_search_th_t *th)
{
    return astarInitStep(&th->search);
}

static int searchThreadStep(plan_ma_search_th_t *th)
{
    return astarStep(&th->search);
}

static int searchThreadProcessMsgs(plan_ma_search_th_t *th)
{
    return PLAN_SEARCH_CONT;
}





_bor_inline void planSearchLoadState(plan_search_astar_t *search,
                                     plan_state_id_t state_id)
{
    if (search->state_id == state_id)
        return;
    planStatePoolGetState(search->state_pool, state_id, search->state);
    search->state_id = state_id;
}

static plan_cost_t planSearchHeur(plan_search_astar_t *search,
                                  plan_state_id_t state_id)
{
    plan_heur_res_t res;

    planSearchLoadState(search, state_id);
    planHeurResInit(&res);
    planHeur(search->heur, search->state, &res);
    return res.heur;
}

static int planSearchCheckGoal(plan_search_astar_t *search,
                               plan_state_space_node_t *node)
{
    int res = planStatePoolPartStateIsSubset(search->state_pool,
                                             search->goal,
                                             node->state_id);
    return res;
}

static void planSearchFindApplicableOps(plan_search_astar_t *search,
                                        plan_state_id_t state_id)
{
    planSearchLoadState(search, state_id);
    planSearchApplicableOpsFind(&search->app_ops, search->state, state_id,
                                search->succ_gen);
}



static void astarInit(plan_search_astar_t *search,
                      plan_state_id_t initial_state,
                      const plan_part_state_t *goal,
                      plan_state_pool_t *state_pool,
                      plan_heur_t *heur,
                      const plan_succ_gen_t *succ_gen)
{
    search->initial_state = initial_state;
    search->goal = goal;
    search->state_pool = state_pool;
    search->state_space = planStateSpaceNew(state_pool);
    search->heur = heur;
    search->succ_gen = succ_gen;
    search->state_id = PLAN_NO_STATE;
    search->state = planStateNew(state_pool);

    planSearchApplicableOpsInit(&search->app_ops, succ_gen->num_operators);

    search->list = planListTieBreaking(2);
    search->pathmax = 0;
}

static void astarFree(plan_search_astar_t *search)
{
    if (search->list)
        planListDel(search->list);
    if (search->state)
        planStateDel(search->state);
    if (search->state_space)
        planStateSpaceDel(search->state_space);
    planSearchApplicableOpsFree(&search->app_ops);
}


static int astarInsertState(plan_search_astar_t *search,
                            plan_state_space_node_t *node,
                            plan_op_t *op,
                            plan_state_space_node_t *parent_node)
{
    plan_cost_t cost[2];
    plan_cost_t heur, g_cost = 0;
    plan_state_id_t parent_state_id = PLAN_NO_STATE;

    // Force to open the node and compute heuristic if necessary
    if (planStateSpaceNodeIsNew(node)){
        planStateSpaceOpen(search->state_space, node);

        // TODO: handle re-computing heuristic and max() of heuristics
        // etc...
        heur = planSearchHeur(search, node->state_id);
        if (search->pathmax && op != NULL && parent_node != NULL){
            heur = BOR_MAX(heur, parent_node->heuristic - op->cost);
        }

    }else{
        if (planStateSpaceNodeIsClosed(node)){
            planStateSpaceReopen(search->state_space, node);
        }

        // The node was already opened, so we have already computed
        // heuristic
        heur = node->heuristic;
    }

    // Skip dead-end states
    if (heur == PLAN_HEUR_DEAD_END)
        return PLAN_SEARCH_CONT;

    if (parent_node){
        g_cost += parent_node->cost;
        parent_state_id = parent_node->state_id;
    }

    if (op){
        g_cost += op->cost;
    }

    // Set up costs for open-list -- ties are broken by heuristic value
    cost[0] = g_cost + heur; // f-value: f() = g() + h()
    cost[1] = heur; // tie-breaking value

    // Set correct values
    node->parent_state_id = parent_state_id;
    node->op              = op;
    node->cost            = g_cost;
    node->heuristic       = heur;

    // Insert into open-list
    planListPush(search->list, cost, node->state_id);
    // TODO: planSearchStatIncGeneratedStates(&search->search.stat);

    return PLAN_SEARCH_CONT;
}

static int astarInitStep(plan_search_astar_t *search)
{
    plan_state_id_t init_state;
    plan_state_space_node_t *node;

    init_state = search->initial_state;
    node = planStateSpaceNode(search->state_space, init_state);
    return astarInsertState(search, node, NULL, NULL);
}

static int astarStep(plan_search_astar_t *search)
{
    plan_cost_t cost[2], g_cost;
    plan_state_id_t cur_state, next_state;
    plan_state_space_node_t *cur_node, *next_node;
    int i, op_size, res;
    plan_op_t **op;

    // Get next state from open list
    if (planListPop(search->list, &cur_state, cost) != 0)
        return PLAN_SEARCH_NOT_FOUND;

    // Get corresponding state space node
    cur_node = planStateSpaceNode(search->state_space, cur_state);

    // Skip already closed nodes
    if (!planStateSpaceNodeIsOpen(cur_node))
        return PLAN_SEARCH_CONT;

    // Close the current state node
    planStateSpaceClose(search->state_space, cur_node);

    // Check whether it is a goal
    if (planSearchCheckGoal(search, cur_node)){
        return PLAN_SEARCH_FOUND;
    }

    // Find all applicable operators
    planSearchFindApplicableOps(search, cur_state);
    // TODO: planSearchStatIncExpandedStates(&search->search.stat);

    // Useful only in MA node
    // TODO: _planSearchMASendState(&search->search, cur_node);

    // Add states created by applicable operators
    op      = search->app_ops.op;
    op_size = search->app_ops.op_found;
    for (i = 0; i < op_size; ++i){
        // Create a new state
        next_state = planOpApply(op[i], cur_state);
        // Compute its g() value
        g_cost = cur_node->cost + op[i]->cost;

        // Obtain corresponding node from state space
        next_node = planStateSpaceNode(search->state_space,
                                       next_state);

        // Decide whether to insert the state into open-list
        if (planStateSpaceNodeIsNew(next_node)
                || next_node->cost > g_cost){
            res = astarInsertState(search, next_node, op[i], cur_node);
            if (res != PLAN_SEARCH_CONT)
                return res;
        }
    }
    return PLAN_SEARCH_CONT;
}
