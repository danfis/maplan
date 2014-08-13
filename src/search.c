#include <sys/resource.h>
#include <boruvka/alloc.h>
#include <boruvka/timer.h>

#include "plan/search.h"

/** Maximal time (in ms) for which is multi-agent thread blocked when
 *  dead-end is reached. This constant is here basicaly to prevent
 *  dead-lock when all threads are in dead-end. So, it should be set to
 *  fairly high number. */
#define SEARCH_MA_MAX_BLOCK_TIME (600 * 1000) // 10 minutes

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

/** Sends TERMINATE_ACK to the arbiter */
static void maSendTerminateAck(plan_ma_comm_queue_t *comm);
/** Performs terminate operation. */
static void maTerminate(plan_search_t *search);
/** Sends public state to peers */
static int maSendPublicState(plan_search_t *search,
                             const plan_state_space_node_t *node);
/** Injects public state corresponding to the message to the search
 *  algorithm */
static void maInjectPublicState(plan_search_t *search,
                                const plan_ma_msg_t *msg);
/** Process one message and returns PLAN_SEARCH_* status */
static int maProcessMsg(plan_search_t *search, plan_ma_msg_t *msg);

/** Back-track path discovered on this node */
static int maBackTrackLocalPath(plan_search_t *search,
                                plan_state_id_t from_state,
                                plan_path_t *path);
/** Updates path in the given message with operators in the path */
static void maUpdateTracedPath(plan_path_t *path, plan_ma_msg_t *msg);
/** Updates message with the traced path and return the agent where the
 *  message should be sent.
 *  If the message shouldn't be sent to any agent -1 is returned in case of
 *  error or -2 if tracing is done. */
static int maUpdateTracePathMsg(plan_search_t *search, plan_ma_msg_t *msg);
/** Reads path stored in the message into the output path structure */
static void maReadMsgPath(plan_ma_msg_t *msg, plan_path_t *path);
/** Process trace-path type message and returns PLAN_SEARCH_* status */
static int maProcessTracePath(plan_search_t *search, plan_ma_msg_t *msg);
/** Performs trace-path operation, it is assumed that search has
 *  found the solution. */
static int maTracePath(plan_search_t *search);

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

    search->heur     = params->heur;
    search->heur_del = params->heur_del;

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
    if (search->heur && search->heur_del)
        planHeurDel(search->heur);
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

static int maHeur(plan_search_t *search, plan_heur_res_t *res)
{
    plan_ma_msg_t *msg;
    int ma_res, msg_res;

    // First call of multi-agent heuristic
    ma_res = planHeurMA(search->heur, search->ma_comm, search->state, res);
    if (ma_res == 0)
        return PLAN_SEARCH_CONT;

    // Wait for update messages
    while (ma_res != 0
            && (msg = planMACommQueueRecvBlock(search->ma_comm)) != NULL){

        if (planMAMsgIsHeurResponse(msg)){
            ma_res = planHeurMAUpdate(search->heur, search->ma_comm, msg, res);
            planMAMsgDel(msg);

        }else{
            msg_res = maProcessMsg(search, msg);
            if (msg_res != PLAN_SEARCH_CONT){
                res->heur = PLAN_HEUR_DEAD_END;
                return msg_res;
            }
        }
    }

    return PLAN_SEARCH_CONT;
}

int _planSearchHeuristic(plan_search_t *search,
                         plan_state_id_t state_id,
                         plan_cost_t *heur_val,
                         plan_search_applicable_ops_t *preferred_ops)
{
    plan_heur_res_t res;
    int fres = PLAN_SEARCH_CONT;

    planStatePoolGetState(search->state_pool, state_id, search->state);
    planSearchStatIncEvaluatedStates(&search->stat);

    planHeurResInit(&res);
    if (preferred_ops){
        res.pref_op = preferred_ops->op;
        res.pref_op_size = preferred_ops->op_found;
    }

    if (!search->heur->ma){
        planHeur(search->heur, search->state, &res);
    }else{
        fres = maHeur(search, &res);
    }

    if (preferred_ops){
        preferred_ops->op_preferred = res.pref_size;
    }

    *heur_val = res.heur;
    return fres;
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
        if (!search->heur->ma){
            _planSearchHeuristic(search, state_id, &heur_val, NULL);
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
    if (node == NULL)
        return node;

    planStateSpaceClose(search->state_space, node);

    if (search->ma)
        maSendPublicState(search, node);

    return node;
}

int _planSearchCheckGoal(plan_search_t *search, plan_state_id_t state_id)
{
    if (planProblemCheckGoal(search->params.prob, state_id)){
        search->goal_state = state_id;
        if (!search->ma)
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
        res = search->step_fn(search);

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

int planSearchMARun(plan_search_t *search,
                    plan_search_ma_params_t *ma_params,
                    plan_path_t *path)
{
    plan_ma_msg_t *msg;
    int res;
    long steps = 0L;
    bor_timer_t timer;

    borTimerStart(&timer);

    search->ma = 1;
    search->ma_comm = ma_params->comm;
    search->ma_terminated = 0;
    search->ma_path = path;

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
        res = search->step_fn(search);
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
            if (maTracePath(search) == PLAN_SEARCH_FOUND){
                planSearchStatSetFoundSolution(&search->stat);
            }else{
                res = PLAN_SEARCH_NOT_FOUND;
            }
            maTerminate(search);
            break;

        }else if (res == PLAN_SEARCH_NOT_FOUND){
            // If this agent reached dead-end, wait either for terminate
            // signal or for some public state it can continue from.
            msg = planMACommQueueRecvBlockTimeout(search->ma_comm,
                                                  SEARCH_MA_MAX_BLOCK_TIME);
            if (msg != NULL){
                res = maProcessMsg(search, msg);
            }else{
                fprintf(stderr, "[%d] Timeout.\n",
                        search->ma_comm->node_id);
                maTerminate(search);
                break;
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
    int res, i, len, *peers;

    if (node->op == NULL || planOperatorIsPrivate(node->op))
        return -2;

    statebuf = planStatePoolGetPackedState(search->state_pool, node->state_id);
    if (statebuf == NULL)
        return -1;

    peers = node->op->send_peer;
    len   = node->op->send_peer_size;
    if (len == 0)
        return -1;

    msg = planMAMsgNew();
    planMAMsgSetPublicState(msg, search->ma_comm->node_id,
                            statebuf,
                            planStatePackerBufSize(search->state_pool->packer),
                            node->state_id,
                            node->cost, node->heuristic);
    for (i = 0; i < len; ++i){
        if (peers[i] != search->ma_comm->node_id)
            res = planMACommQueueSendToNode(search->ma_comm, peers[i], msg);
    }
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

    }else if (planMAMsgIsHeurRequest(msg)){
        if (search->heur)
            planHeurMARequest(search->heur, search->ma_comm, msg);

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
        res = maProcessTracePath(search, msg);
    }

    planMAMsgDel(msg);

    return res;
}

static int maBackTrackLocalPath(plan_search_t *search,
                                plan_state_id_t from_state,
                                plan_path_t *path)
{
    extractPath(search->state_space, from_state, path);
    if (planPathEmpty(path)){
        fprintf(stderr, "Error: Could not trace any portion of the"
                        " path.\n");
        return -1;
    }

    return 0;
}

static void maUpdateTracedPath(plan_path_t *path, plan_ma_msg_t *msg)
{
    plan_path_op_t *op;
    bor_list_t *lst;

    for (lst = borListPrev(path); lst != path; lst = borListPrev(lst)){
        op = BOR_LIST_ENTRY(lst, plan_path_op_t, path);
        planMAMsgTracePathAddOperator(msg, op->name, op->cost);
    }
}

/** Updates message with the traced path and return the agent where the
 *  message should be sent.
 *  If the message shouldn't be sent to any agent -1 is returned in case of
 *  error or -2 if tracing is done. */
static int maUpdateTracePathMsg(plan_search_t *search, plan_ma_msg_t *msg)
{
    plan_path_t path;
    plan_state_id_t from_state;
    plan_path_op_t *first_op;
    ma_pub_state_data_t *pub_state;
    int target_agent_id;

    // Early exit if the path was already traced
    if (planMAMsgTracePathIsDone(msg))
        return -2;

    // Get state from which start back-tracking
    from_state = planMAMsgTracePathStateId(msg);

    // Back-track path and handle possible error
    planPathInit(&path);
    if (maBackTrackLocalPath(search, from_state, &path) != 0){
        return -1;
    }

    // Update trace path message by adding all operators in reverse
    // order
    maUpdateTracedPath(&path, msg);

    // Check if we don't have the full path already
    first_op = planPathFirstOp(&path);
    if (first_op->from_state == 0){
        // If traced the path to the initial state -- set the traced
        // path as done.
        planMAMsgTracePathSetDone(msg);
        target_agent_id = -2;

    }else{
        // Retrieve public-state related data for the first state in path
        pub_state = planStatePoolData(search->state_pool,
                                      search->ma_pub_state_reg,
                                      first_op->from_state);
        if (pub_state->agent_id == -1){
            fprintf(stderr, "Error: Trace-back of the path end up in"
                            " non-public state which also isn't the"
                            " initial state!\n");
            return -1;
        }

        planMAMsgTracePathSetStateId(msg, pub_state->state_id);
        target_agent_id = pub_state->agent_id;
    }

    planPathFree(&path);

    return target_agent_id;
}

static void maReadMsgPath(plan_ma_msg_t *msg, plan_path_t *path)
{
    int i, len, cost;
    const char *name;

    // get number of operators in path
    len = planMAMsgTracePathNumOperators(msg);

    // copy path to the path strcture
    planPathInit(path);
    for (i = 0; i < len; ++i){
        name = planMAMsgTracePathOperator(msg, i, &cost);
        planPathPrepend2(path, name, cost);
    }
}

static int maProcessTracePath(plan_search_t *search, plan_ma_msg_t *msg)
{
    int res, origin_agent;

    res = maUpdateTracePathMsg(search, msg);
    if (res >= 0){
        planMACommQueueSendToNode(search->ma_comm, res, msg);
        return PLAN_SEARCH_CONT;

    }else if (res == -1){
        return PLAN_SEARCH_ABORT;

    }else if (res == -2){
        origin_agent = planMAMsgTracePathOriginAgent(msg);

        if (origin_agent != search->ma_comm->node_id){
            // If this is not the original agent sent the result to the
            // original agent
            planMACommQueueSendToNode(search->ma_comm, origin_agent, msg);
            return PLAN_SEARCH_CONT;

        }else{
            // If we have received the full traced path which we
            // originated, read the path to the internal structure and
            // report found solution.
            if (search->ma_path)
                maReadMsgPath(msg, search->ma_path);
            return PLAN_SEARCH_FOUND;
        }
    }

    return PLAN_SEARCH_ABORT;
}

static int maTracePath(plan_search_t *search)
{
    plan_ma_msg_t *msg;
    int res;

    // Construct trace-path message and fake it as it were we received this
    // message and we should process it
    msg = planMAMsgNew();
    planMAMsgSetTracePath(msg, search->ma_comm->node_id);
    planMAMsgTracePathSetStateId(msg, search->goal_state);

    // Process that message as if it were just received
    res = maProcessTracePath(search, msg);
    planMAMsgDel(msg);

    // We have found solution, so exit early
    if (res != PLAN_SEARCH_CONT)
        return res;

    // Wait for trace-path response or terminate signal
    while ((msg = planMACommQueueRecvBlock(search->ma_comm)) != NULL){
        if (planMAMsgIsTracePath(msg) || planMAMsgIsTerminateType(msg)){
            res = maProcessMsg(search, msg);
            if (res != PLAN_SEARCH_CONT)
                break;
        }else{
            planMAMsgDel(msg);
        }
    }

    return res;
}
