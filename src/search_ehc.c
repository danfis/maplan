#include <boruvka/alloc.h>

#include "plan/search.h"
#include "plan/ma_agent.h"

struct _plan_search_ehc_t {
    plan_search_t search;

    plan_list_lazy_t *list; /*!< List to keep track of the states */
    plan_heur_t *heur;      /*!< Heuristic function */
    int heur_del;           /*!< True if .heur should be deleted */
    int use_preferred_ops;  /*!< True if preffered operators should be used */
    plan_cost_t best_heur;  /*!< Value of the best heuristic value found so far */
    plan_search_applicable_ops_t *pref_ops; /*!< This pointer is set to
                                                 &search->applicable_ops in
                                                 case preferred operators
                                                 are enabled. */
};
typedef struct _plan_search_ehc_t plan_search_ehc_t;

#define SEARCH_FROM_PARENT(parent) \
    bor_container_of((parent), plan_search_ehc_t, search)

/** Adds successors to the open list, the operators should already be
 *  filled in ehc->search.applicable_ops */
static void addSuccessors(plan_search_ehc_t *ehc, plan_state_id_t state_id);
/** Process the given state */
static int processState(plan_search_ehc_t *ehc,
                        plan_state_id_t cur_state_id,
                        plan_state_id_t parent_state_id,
                        plan_operator_t *parent_op,
                        plan_state_space_node_t **closed_node);

/** Frees allocated resorces */
static void planSearchEHCDel(plan_search_t *_ehc);
/** Initializes search. This must be call exactly once. */
static int planSearchEHCInit(plan_search_t *);
/** Performes one step in the algorithm. */
static int planSearchEHCStep(plan_search_t *,
                             plan_search_step_change_t *change);
static int planSearchEHCMAInit(plan_search_t *, plan_ma_agent_t *agent);
static int planSearchEHCMAStep(plan_search_t *, plan_ma_agent_t *agent);
static int planSearchEHCMAUpdate(plan_search_t *, plan_ma_agent_t *agent,
                                 const plan_ma_msg_t *msg);
/** Injects a new state into open-list */
static int planSearchEHCInjectState(plan_search_t *, plan_state_id_t state_id,
                                    plan_cost_t cost, plan_cost_t heuristic);


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
                    planSearchEHCStep,
                    planSearchEHCMAInit,
                    planSearchEHCMAStep,
                    planSearchEHCMAUpdate,
                    planSearchEHCInjectState);

    ehc->list              = planListLazyFifoNew();
    ehc->heur              = params->heur;
    ehc->heur_del          = params->heur_del;
    ehc->use_preferred_ops = params->use_preferred_ops;
    ehc->best_heur         = PLAN_COST_MAX;

    // Set up pointer to applicable operators for later
    ehc->pref_ops = NULL;
    if (ehc->use_preferred_ops){
        ehc->pref_ops = &ehc->search.applicable_ops;
    }

    return &ehc->search;
}

static void planSearchEHCDel(plan_search_t *_ehc)
{
    plan_search_ehc_t *ehc = SEARCH_FROM_PARENT(_ehc);

    _planSearchFree(&ehc->search);
    if (ehc->list)
        planListLazyDel(ehc->list);
    if (ehc->heur_del)
        planHeurDel(ehc->heur);
    BOR_FREE(ehc);
}

static int planSearchEHCInit(plan_search_t *_ehc)
{
    plan_search_ehc_t *ehc = SEARCH_FROM_PARENT(_ehc);
    plan_state_id_t init_state;

    init_state = ehc->search.params.prob->initial_state;

    ehc->best_heur = PLAN_COST_MAX;
    return processState(ehc, init_state, 0, NULL, NULL);
}

_bor_inline int step(plan_search_ehc_t *ehc,
                     plan_state_space_node_t **closed_node)
{
    plan_state_id_t parent_state_id, cur_state_id;
    plan_operator_t *parent_op;

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

    return processState(ehc, cur_state_id, parent_state_id, parent_op,
                        closed_node);
}

static int planSearchEHCStep(plan_search_t *_ehc,
                             plan_search_step_change_t *change)
{
    plan_search_ehc_t *ehc = SEARCH_FROM_PARENT(_ehc);
    return step(ehc, NULL);
}

static int planSearchEHCMAInit(plan_search_t *search, plan_ma_agent_t *agent)
{
    return planSearchEHCInit(search);
}

_bor_inline int sendPublicState(plan_ma_agent_t *agent,
                                const plan_state_space_node_t *node)
{
    plan_ma_msg_t *msg;
    const void *statebuf;
    int res;

    statebuf = planStatePoolGetPackedState(agent->state_pool, node->state_id);
    if (statebuf == NULL)
        return -1;

    msg = planMAMsgNew();
    planMAMsgSetPublicState(msg, agent->comm->node_id,
                            statebuf, agent->packed_state_size,
                            node->state_id,
                            node->cost, node->heuristic);
    res = planMACommQueueSendToAll(agent->comm, msg);
    planMAMsgDel(msg);

    return res;
}

static int planSearchEHCMAStep(plan_search_t *_ehc, plan_ma_agent_t *agent)
{
    plan_search_ehc_t *ehc = SEARCH_FROM_PARENT(_ehc);
    plan_state_space_node_t *closed_node = NULL;
    int res;

    res = step(ehc, &closed_node);

    if (res == PLAN_SEARCH_CONT && closed_node != NULL){
        // Check whether the closed node is public state and if so, send it
        // to the peers
        if (closed_node->op != NULL
                && !planOperatorIsPrivate(closed_node->op)){
            sendPublicState(agent, closed_node);
        }
    }

    return res;
}

static int planSearchEHCMAUpdate(plan_search_t *search, plan_ma_agent_t *agent,
                                 const plan_ma_msg_t *msg)
{
    // TODO: Inject public state
    return PLAN_SEARCH_CONT;
}

static int planSearchEHCInjectState(plan_search_t *_ehc, plan_state_id_t state_id,
                                    plan_cost_t cost, plan_cost_t heuristic)
{
    plan_search_ehc_t *ehc = SEARCH_FROM_PARENT(_ehc);
    return _planSearchLazyInjectState(&ehc->search, ehc->heur, ehc->list,
                                      state_id, cost, heuristic);
}

static void addSuccessors(plan_search_ehc_t *ehc, plan_state_id_t state_id)
{
    if (ehc->use_preferred_ops == PLAN_SEARCH_PREFERRED_ONLY){
        // Use only preferred operators
        _planSearchAddLazySuccessors(&ehc->search, state_id,
                                     ehc->search.applicable_ops.op,
                                     ehc->search.applicable_ops.op_preferred,
                                     0, ehc->list);
    }else{
        // Use all operators the preferred were already sorted as first
        _planSearchAddLazySuccessors(&ehc->search, state_id,
                                     ehc->search.applicable_ops.op,
                                     ehc->search.applicable_ops.op_found,
                                     0, ehc->list);
    }
}

static int processState(plan_search_ehc_t *ehc,
                        plan_state_id_t cur_state_id,
                        plan_state_id_t parent_state_id,
                        plan_operator_t *parent_op,
                        plan_state_space_node_t **closed_node)
{
    plan_cost_t cur_heur;
    plan_state_space_node_t *node;

    // find applicable operators in the current state
    _planSearchFindApplicableOps(&ehc->search, cur_state_id);

    // compute heuristic value for the current node
    cur_heur = _planSearchHeuristic(&ehc->search, cur_state_id, ehc->heur,
                                    ehc->pref_ops);

    // open and close the node so we can trace the path from goal to the
    // initial state
    node = _planSearchNodeOpenClose(&ehc->search,
                                    cur_state_id, parent_state_id,
                                    parent_op, 0, cur_heur);
    if (closed_node)
        *closed_node = node;

    // check if the current state is the goal
    if (_planSearchCheckGoal(&ehc->search, cur_state_id))
        return PLAN_SEARCH_FOUND;

    if (cur_heur != PLAN_HEUR_DEAD_END){
        // If the heuristic for the current state is the best so far, restart
        // EHC algorithm with an empty list.
        if (cur_heur < ehc->best_heur){
            planListLazyClear(ehc->list);
            ehc->best_heur = cur_heur;
        }

        addSuccessors(ehc, cur_state_id);
    }

    return PLAN_SEARCH_CONT;
}
