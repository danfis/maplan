#include <sys/resource.h>
#include <boruvka/alloc.h>
#include <boruvka/timer.h>

#include "plan/search.h"

/** Reference data for the received public states */
struct _ma_pub_state_data_t {
    int agent_id;             /*!< ID of the source agent */
    plan_cost_t cost;         /*!< Cost of the path to this state as found
                                   by the remote agent. */
    plan_state_id_t state_id; /*!< ID of the state in remote agent's state
                                   pool. This is used for back-tracking. */
};
typedef struct _ma_pub_state_data_t ma_pub_state_data_t;

static void extractPath(plan_state_space_t *state_space,
                        plan_state_id_t goal_state,
                        plan_path_t *path);

/** Initializes and destroys a struct for holding applicable operators */
static void planSearchApplicableOpsInit(plan_search_t *search, int op_size);
static void planSearchApplicableOpsFree(plan_search_t *search);
/** Fills search->applicable_ops with operators applicable in specified
 *  state */
_bor_inline void planSearchApplicableOpsFind(plan_search_t *search,
                                             plan_state_id_t state_id);

/** TODO */
static int maSendPublicState(plan_search_t *search,
                             const plan_state_space_node_t *node);


void planSearchStatInit(plan_search_stat_t *stat)
{
    stat->elapsed_time = 0.f;
    stat->steps = 0L;
    stat->evaluated_states = 0L;
    stat->expanded_states = 0L;
    stat->generated_states = 0L;
    stat->peak_memory = 0L;
    stat->found = 0;
    stat->not_found = 0;
}

void planSearchStepChangeInit(plan_search_step_change_t *step_change)
{
    bzero(step_change, sizeof(*step_change));
}

void planSearchStepChangeFree(plan_search_step_change_t *step_change)
{
    if (step_change->closed_node)
        BOR_FREE(step_change->closed_node);
    bzero(step_change, sizeof(*step_change));
}

void planSearchStepChangeReset(plan_search_step_change_t *step_change)
{
    step_change->closed_node_size = 0;
}

void planSearchStepChangeAddClosedNode(plan_search_step_change_t *sc,
                                       plan_state_space_node_t *node)
{
    if (sc->closed_node_size + 1 > sc->closed_node_alloc){
        sc->closed_node_alloc = sc->closed_node_size + 1;
        sc->closed_node = BOR_REALLOC_ARR(sc->closed_node,
                                          plan_state_space_node_t *,
                                          sc->closed_node_alloc);
    }

    sc->closed_node[sc->closed_node_size++] = node;
}

void planSearchStatUpdatePeakMemory(plan_search_stat_t *stat)
{
    struct rusage usg;

    if (getrusage(RUSAGE_SELF, &usg) == 0){
        stat->peak_memory = usg.ru_maxrss;
    }
}

void planSearchParamsInit(plan_search_params_t *params)
{
    bzero(params, sizeof(*params));
}

void _planSearchInit(plan_search_t *search,
                     const plan_search_params_t *params,
                     plan_search_del_fn del_fn,
                     plan_search_init_fn init_fn,
                     plan_search_step_fn step_fn,
                     plan_search_inject_state_fn inject_state_fn)
{
    ma_pub_state_data_t msg_init;

    search->del_fn  = del_fn;
    search->init_fn = init_fn;
    search->step_fn = step_fn;
    search->inject_state_fn = inject_state_fn;
    search->params = *params;

    planSearchStatInit(&search->stat);

    search->state_pool  = params->prob->state_pool;
    search->state_space = planStateSpaceNew(search->state_pool);
    search->state       = planStateNew(search->state_pool);
    search->goal_state  = PLAN_NO_STATE;

    planSearchApplicableOpsInit(search, search->params.prob->op_size);

    search->ma = 0;
    search->ma_comm = NULL;

    msg_init.agent_id = -1;
    msg_init.cost = PLAN_COST_MAX;
    search->ma_pub_state_reg = planStatePoolDataReserve(search->state_pool,
                                                        sizeof(ma_pub_state_data_t),
                                                        NULL, &msg_init);
    search->ma_terminated = 0;
}

void _planSearchFree(plan_search_t *search)
{
    planSearchApplicableOpsFree(search);
    if (search->state)
        planStateDel(search->state);
    if (search->state_space)
        planStateSpaceDel(search->state_space);
}

void _planSearchFindApplicableOps(plan_search_t *search,
                                  plan_state_id_t state_id)
{
    planSearchApplicableOpsFind(search, state_id);
}

plan_cost_t _planSearchHeuristic(plan_search_t *search,
                                 plan_state_id_t state_id,
                                 plan_heur_t *heur,
                                 plan_search_applicable_ops_t *preferred_ops)
{
    plan_heur_res_t res;

    planStatePoolGetState(search->state_pool, state_id, search->state);
    planSearchStatIncEvaluatedStates(&search->stat);

    planHeurResInit(&res);
    if (preferred_ops){
        res.pref_op = preferred_ops->op;
        res.pref_op_size = preferred_ops->op_found;
    }

    planHeur(heur, search->state, &res);

    if (preferred_ops){
        preferred_ops->op_preferred = res.pref_size;
    }

    return res.heur;
}

void _planSearchAddLazySuccessors(plan_search_t *search,
                                  plan_state_id_t state_id,
                                  plan_operator_t **op, int op_size,
                                  plan_cost_t cost,
                                  plan_list_lazy_t *list)
{
    int i;

    for (i = 0; i < op_size; ++i){
        planListLazyPush(list, cost, state_id, op[i]);
        planSearchStatIncGeneratedStates(&search->stat);
    }
}

int _planSearchLazyInjectState(plan_search_t *search,
                               plan_heur_t *heur,
                               plan_list_lazy_t *list,
                               plan_state_id_t state_id,
                               plan_cost_t cost, plan_cost_t heur_val)
{
    plan_state_space_node_t *node;

    // Retrieve node corresponding to the state
    node = planStateSpaceNode(search->state_space, state_id);

    // If the node was not discovered yet insert it into open-list
    if (planStateSpaceNodeIsNew(node)){
        // Compute heuristic value
        if (heur){
            heur_val = _planSearchHeuristic(search, state_id, heur, NULL);
        }

        // Set node to closed state with appropriate cost and heuristic
        // value
        node = planStateSpaceOpen2(search->state_space, state_id,
                                   PLAN_NO_STATE, NULL,
                                   cost, heur_val);
        planStateSpaceClose(search->state_space, node);

        // Add node's successor to the open-list
        _planSearchFindApplicableOps(search, state_id);
        _planSearchAddLazySuccessors(search, state_id,
                                     search->applicable_ops.op,
                                     search->applicable_ops.op_found,
                                     heur_val, list);
    }

    return 0;
}

void _planSearchReachedDeadEnd(plan_search_t *search)
{
    if (!search->ma)
        planSearchStatSetNotFound(&search->stat);
}

int _planSearchNewState(plan_search_t *search,
                        plan_operator_t *operator,
                        plan_state_id_t parent_state,
                        plan_state_id_t *new_state_id,
                        plan_state_space_node_t **new_node)
{
    plan_state_id_t state_id;
    plan_state_space_node_t *node;

    state_id = planOperatorApply(operator, parent_state);
    node     = planStateSpaceNode(search->state_space, state_id);

    if (new_state_id)
        *new_state_id = state_id;
    if (new_node)
        *new_node = node;

    if (planStateSpaceNodeIsNew(node))
        return 0;
    return -1;
}

plan_state_space_node_t *_planSearchNodeOpenClose(plan_search_t *search,
                                                  plan_state_id_t state,
                                                  plan_state_id_t parent_state,
                                                  plan_operator_t *parent_op,
                                                  plan_cost_t cost,
                                                  plan_cost_t heur)
{
    plan_state_space_node_t *node;

    node = planStateSpaceOpen2(search->state_space, state,
                               parent_state, parent_op,
                               cost, heur);
    planStateSpaceClose(search->state_space, node);

    if (search->ma)
        maSendPublicState(search, node);

    return node;
}

int _planSearchCheckGoal(plan_search_t *search, plan_state_id_t state_id)
{
    if (planProblemCheckGoal(search->params.prob, state_id)){
        search->goal_state = state_id;
        planSearchStatSetFoundSolution(&search->stat);
        return 1;
    }

    return 0;
}

void planSearchDel(plan_search_t *search)
{
    search->del_fn(search);
}

int planSearchRun(plan_search_t *search, plan_path_t *path)
{
    int res;
    long steps = 0;
    bor_timer_t timer;

    borTimerStart(&timer);

    res = search->init_fn(search);
    while (res == PLAN_SEARCH_CONT){
        res = search->step_fn(search, NULL);

        ++steps;
        if (res == PLAN_SEARCH_CONT
                && search->params.progress_fn
                && steps >= search->params.progress_freq){
            _planUpdateStat(&search->stat, steps, &timer);
            res = search->params.progress_fn(&search->stat,
                                             search->params.progress_data);
            steps = 0;
        }
    }

    if (search->params.progress_fn && res != PLAN_SEARCH_ABORT && steps != 0){
        _planUpdateStat(&search->stat, steps, &timer);
        search->params.progress_fn(&search->stat,
                                   search->params.progress_data);
    }

    if (res == PLAN_SEARCH_FOUND){
        extractPath(search->state_space, search->goal_state, path);
    }

    return res;
}

static void maSendTerminateAck(plan_ma_comm_queue_t *comm)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew();
    planMAMsgSetTerminateAck(msg);
    planMACommQueueSendToArbiter(comm, msg);
    planMAMsgDel(msg);
}

static void maTerminate(plan_search_t *search)
{
    plan_ma_msg_t *msg;
    int count, ack;

    if (search->ma_terminated)
        return;

    if (search->ma_comm->arbiter){
        // If this is arbiter just send TERMINATE signal and wait for ACK
        // from all peers.
        // Because the TERMINATE_ACK is sent only as a response to TERMINATE
        // signal and because TERMINATE signal can send only the single
        // arbiter from exactly this place it is enough just to count
        // number of ACKs.

        msg = planMAMsgNew();
        planMAMsgSetTerminate(msg);
        planMACommQueueSendToAll(search->ma_comm, msg);
        planMAMsgDel(msg);

        count = planMACommQueueNumPeers(search->ma_comm);
        while (count > 0
                && (msg = planMACommQueueRecvBlock(search->ma_comm)) != NULL){

            if (planMAMsgIsTerminateAck(msg))
                --count;
            planMAMsgDel(msg);
        }
    }else{
        // If this node is not arbiter send TERMINATE_REQUEST, wait for
        // TERMINATE signal, ACK it and then termination is finished.

        // 1. Send TERMINATE_REQUEST
        msg = planMAMsgNew();
        planMAMsgSetTerminateRequest(msg);
        planMACommQueueSendToArbiter(search->ma_comm, msg);
        planMAMsgDel(msg);

        // 2. Wait for TERMINATE
        ack = 0;
        while (!ack && (msg = planMACommQueueRecvBlock(search->ma_comm)) != NULL){
            if (planMAMsgIsTerminate(msg)){
                // 3. Send TERMINATE_ACK
                maSendTerminateAck(search->ma_comm);
                ack = 1;
            }
            planMAMsgDel(msg);
        }
    }

    search->ma_terminated = 1;
}

static int maSendPublicState(plan_search_t *search,
                             const plan_state_space_node_t *node)
{
    plan_ma_msg_t *msg;
    const void *statebuf;
    int res;

    if (node->op == NULL || planOperatorIsPrivate(node->op))
        return -2;

    statebuf = planStatePoolGetPackedState(search->state_pool, node->state_id);
    if (statebuf == NULL)
        return -1;

    msg = planMAMsgNew();
    planMAMsgSetPublicState(msg, search->ma_comm->node_id,
                            statebuf,
                            planStatePackerBufSize(search->state_pool->packer),
                            node->state_id,
                            node->cost, node->heuristic);
    res = planMACommQueueSendToAll(search->ma_comm, msg);
    planMAMsgDel(msg);

    return res;
}

static void maInjectPublicState(plan_search_t *search,
                                const plan_ma_msg_t *msg)
{
    int cost, heuristic;
    ma_pub_state_data_t *pub_state_data;
    plan_state_id_t state_id;
    const void *packed_state;

    // Unroll data from the message
    packed_state = planMAMsgPublicStateStateBuf(msg);
    cost         = planMAMsgPublicStateCost(msg);
    heuristic    = planMAMsgPublicStateHeur(msg);

    // Insert packed state into state-pool if not already inserted
    state_id = planStatePoolInsertPacked(search->state_pool,
                                         packed_state);

    // Get public state reference data
    pub_state_data = planStatePoolData(search->state_pool,
                                       search->ma_pub_state_reg,
                                       state_id);

    // This state was already inserted in past, so set the reference
    // data only if the cost is smaller
    // Set the reference data only if the new cost is smaller than the
    // current one. This means that either the state is brand new or the
    // previously inserted state had bigger cost.
    if (pub_state_data->cost > cost){
        pub_state_data->agent_id = planMAMsgPublicStateAgent(msg);
        pub_state_data->cost     = cost;
        pub_state_data->state_id = planMAMsgPublicStateStateId(msg);
    }

    // Inject state into search algorithm
    search->inject_state_fn(search, state_id, cost, heuristic);
}

static int maProcessMsg(plan_search_t *search, plan_ma_msg_t *msg)
{
    int res = PLAN_SEARCH_CONT;

    if (planMAMsgIsPublicState(msg)){
        maInjectPublicState(search, msg);
        res = PLAN_SEARCH_CONT;

    }else if (planMAMsgIsTerminateType(msg)){
        if (search->ma_comm->arbiter){
            // The arbiter should ignore all signals except
            // TERMINATE_REQUEST because TERMINATE is allowed to send
            // only arbiter itself and TERMINATE_ACK should be received
            // in terminate() method.
            if (planMAMsgIsTerminateRequest(msg)){
                maTerminate(search);
                res = PLAN_SEARCH_ABORT;
            }

        }else{
            // The non-arbiter node should accept only TERMINATE
            // signal and send ACK to him.
            if (planMAMsgIsTerminate(msg)){
                maSendTerminateAck(search->ma_comm);
                search->ma_terminated = 1;
                res = PLAN_SEARCH_ABORT;
            }
        }

    }else if (planMAMsgIsTracePath(msg)){
        // TODO
        //res = processTracePath(agent, msg);
    }

    planMAMsgDel(msg);

    return res;
}

int planSearchMARun(plan_search_t *search,
                    plan_search_ma_params_t *ma_params)
{
    plan_ma_msg_t *msg;
    int res;
    long steps = 0L;
    bor_timer_t timer;

    borTimerStart(&timer);

    search->ma = 1;
    search->ma_comm = ma_params->comm;
    search->ma_terminated = 0;

    res = search->init_fn(search);
    while (res == PLAN_SEARCH_CONT){
        // Process all pending messages
        while (res == PLAN_SEARCH_CONT
                    && (msg = planMACommQueueRecv(search->ma_comm)) != NULL){
            res = maProcessMsg(search, msg);
        }

        // Again check the status because the message could change it
        if (res != PLAN_SEARCH_CONT)
            break;

        // Perform one step of algorithm.
        res = search->step_fn(search, NULL);
        ++steps;

        // call progress callback
        if (res == PLAN_SEARCH_CONT
                && search->params.progress_fn
                && steps >= search->params.progress_freq){
            _planUpdateStat(&search->stat, steps, &timer);
            res = search->params.progress_fn(&search->stat,
                                             search->params.progress_data);
            steps = 0;
        }

        if (res == PLAN_SEARCH_ABORT){
            maTerminate(search);
            break;

        }else if (res == PLAN_SEARCH_FOUND){
            // If the solution was found, terminate agent cluster and exit.
            // TODO
            //if (tracePath(agent) == PLAN_SEARCH_FOUND)
            //    agent->found = 1;
            maTerminate(search);
            break;

        }else if (res == PLAN_SEARCH_NOT_FOUND){
            // If this agent reached dead-end, wait either for terminate
            // signal or for some public state it can continue from.
            if ((msg = planMACommQueueRecvBlock(search->ma_comm)) != NULL){
                res = maProcessMsg(search, msg);
            }
        }
    }

    // call last progress callback
    if (search->params.progress_fn && steps != 0L){
        _planUpdateStat(&search->stat, steps, &timer);
        search->params.progress_fn(&search->stat,
                                   search->params.progress_data);
    }

    return res;
}





void planSearchBackTrackPath(plan_search_t *search, plan_path_t *path)
{
    if (search->goal_state != PLAN_NO_STATE)
        planSearchBackTrackPathFrom(search, search->goal_state, path);
}

void planSearchBackTrackPathFrom(plan_search_t *search,
                                 plan_state_id_t from_state,
                                 plan_path_t *path)
{
    extractPath(search->state_space, from_state, path);
}


void _planUpdateStat(plan_search_stat_t *stat,
                     long steps, bor_timer_t *timer)
{
    stat->steps += steps;

    borTimerStop(timer);
    stat->elapsed_time = borTimerElapsedInSF(timer);

    planSearchStatUpdatePeakMemory(stat);
}

static void extractPath(plan_state_space_t *state_space,
                        plan_state_id_t goal_state,
                        plan_path_t *path)
{
    plan_state_space_node_t *node;

    planPathInit(path);

    node = planStateSpaceNode(state_space, goal_state);
    while (node && node->op){
        planPathPrepend(path, node->op,
                        node->parent_state_id, node->state_id);
        node = planStateSpaceNode(state_space, node->parent_state_id);
    }
}

static void planSearchApplicableOpsInit(plan_search_t *search, int op_size)
{
    search->applicable_ops.op = BOR_ALLOC_ARR(plan_operator_t *, op_size);
    search->applicable_ops.op_size = op_size;
    search->applicable_ops.op_found = 0;
    search->applicable_ops.state = PLAN_NO_STATE;
}

static void planSearchApplicableOpsFree(plan_search_t *search)
{
    BOR_FREE(search->applicable_ops.op);
}

_bor_inline void planSearchApplicableOpsFind(plan_search_t *search,
                                             plan_state_id_t state_id)
{
    plan_search_applicable_ops_t *app = &search->applicable_ops;

    if (state_id == app->state)
        return;

    // unroll the state into search->state struct
    planStatePoolGetState(search->state_pool, state_id, search->state);

    // get operators to get successors
    app->op_found = planSuccGenFind(search->params.prob->succ_gen,
                                    search->state, app->op, app->op_size);

    // remember the corresponding state
    app->state = state_id;
}
