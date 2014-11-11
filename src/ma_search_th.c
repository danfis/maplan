#include "ma_search_th.h"
#include "ma_search_common.h"

/** Reference data for the received public states */
struct _pub_state_data_t {
    int agent_id;             /*!< ID of the source agent */
    plan_cost_t cost;         /*!< Cost of the path to this state as found
                                   by the remote agent. */
    plan_state_id_t state_id; /*!< ID of the state in remote agent's state
                                   pool. This is used for back-tracking. */
};
typedef struct _pub_state_data_t pub_state_data_t;

static void *searchThread(void *data);
static int searchThreadPostStep(plan_search_t *search, int res, void *ud);
static void searchThreadExpandedNode(plan_search_t *search,
                                     plan_state_space_node_t *node, void *ud);
static void processMsg(plan_ma_search_th_t *th, plan_ma_msg_t *msg);
static void publicStateSend(plan_ma_search_th_t *th,
                            plan_state_space_node_t *node);
static void publicStateRecv(plan_ma_search_th_t *th,
                            plan_ma_msg_t *msg);

void planMASearchThInit(plan_ma_search_th_t *th,
                        plan_search_t *search,
                        plan_ma_comm_t *comm,
                        plan_path_t *path)
{
    pub_state_data_t msg_init;

    th->search = search;
    th->comm = comm;
    th->path = path;

    msg_init.agent_id = -1;
    msg_init.cost = PLAN_COST_MAX;
    msg_init.state_id = PLAN_NO_STATE;
    th->pub_state_reg = planStatePoolDataReserve(search->state_pool,
                                                 sizeof(pub_state_data_t),
                                                 NULL, &msg_init);
    borFifoSemInit(&th->msg_queue, sizeof(plan_ma_msg_t *));

    th->res = -1;
}

void planMASearchThFree(plan_ma_search_th_t *th)
{
    plan_ma_msg_t *msg;

    // Empty queue
    while (borFifoSemPop(&th->msg_queue, &msg) == 0){
        planMAMsgDel(msg);
    }
    borFifoSemFree(&th->msg_queue);
}

void planMASearchThRun(plan_ma_search_th_t *th)
{
    pthread_create(&th->thread, NULL, searchThread, th);
}

void planMASearchThJoin(plan_ma_search_th_t *th)
{
    plan_ma_msg_t *msg = NULL;

    // Push an empty message to unblock message queue
    borFifoSemPush(&th->msg_queue, &msg);

    pthread_join(th->thread, NULL);
}

static void *searchThread(void *data)
{
    plan_ma_search_th_t *th = data;

    planSearchSetPostStep(th->search, searchThreadPostStep, th);
    planSearchSetExpandedNode(th->search, searchThreadExpandedNode, th);
    planSearchRun(th->search, th->path);
    fprintf(stderr, "Th[%d] END\n", th->comm->node_id);
    fflush(stderr);

    return NULL;
}

static int searchThreadPostStep(plan_search_t *search, int res, void *ud)
{
    plan_ma_search_th_t *th = ud;
    plan_ma_msg_t *msg = NULL;
    int type = -1;

    fprintf(stderr, "Th[%d] PostStep: %d\n", th->comm->node_id, res);
    fflush(stderr);
    if (res == PLAN_SEARCH_FOUND){
        fprintf(stderr, "Th[%d] FOUND\n", th->comm->node_id);
        fflush(stderr);
        // TODO: Verify solution
        planMASearchTerminate(th->comm);
        th->res = PLAN_SEARCH_FOUND;
        res = PLAN_SEARCH_CONT;

    }else if (res == PLAN_SEARCH_ABORT){
        fprintf(stderr, "Th[%d] ABORT\n", th->comm->node_id);
        fflush(stderr);
        // Initialize termination of a whole cluster
        planMASearchTerminate(th->comm);

        // Ignore all messages except terminate (which is empty)
        do {
            fprintf(stderr, "Th[%d] Block\n", th->comm->node_id);
            fflush(stderr);
            borFifoSemPopBlock(&th->msg_queue, &msg);
            fprintf(stderr, "Th[%d] Unblock\n", th->comm->node_id);
            fflush(stderr);
            if (msg)
                planMAMsgDel(msg);
        } while (msg);

    }else if (res == PLAN_SEARCH_NOT_FOUND){
        fprintf(stderr, "Th[%d] NOT FOUND\n", th->comm->node_id);
        fflush(stderr);

        // Block until public-state or terminate isn't received
        do {
            fprintf(stderr, "Th[%d] Block\n", th->comm->node_id);
            fflush(stderr);
            borFifoSemPopBlock(&th->msg_queue, &msg);
            fprintf(stderr, "Th[%d] Unblock\n", th->comm->node_id);
            fflush(stderr);

            if (msg){
                type = planMAMsgType(msg);
                processMsg(th, msg);
                planMAMsgDel(msg);
            }
        } while (msg && type != PLAN_MA_MSG_PUBLIC_STATE);

        // Empty message means to terminate
        if (!msg)
            return PLAN_SEARCH_ABORT;
        res = PLAN_SEARCH_CONT;
    }

    // Process all messages
    while (borFifoSemPop(&th->msg_queue, &msg) == 0){
        // Empty message means to terminate
        if (!msg)
            return PLAN_SEARCH_ABORT;

        // Process message
        processMsg(th, msg);
        planMAMsgDel(msg);
    }

    fprintf(stderr, "Th[%d] res: %d\n", th->comm->node_id, res);
    fflush(stderr);
    return res;
}

static void searchThreadExpandedNode(plan_search_t *search,
                                     plan_state_space_node_t *node, void *ud)
{
    plan_ma_search_th_t *th = ud;
    publicStateSend(th, node);
}

static void processMsg(plan_ma_search_th_t *th, plan_ma_msg_t *msg)
{
    int type; 

    type = planMAMsgType(msg);
    fprintf(stderr, "Th[%d] msg: %d\n", th->comm->node_id, type);
    if (type == PLAN_MA_MSG_PUBLIC_STATE){
        publicStateRecv(th, msg);
    }
}

static void publicStateSend(plan_ma_search_th_t *th,
                            plan_state_space_node_t *node)
{
    const void *statebuf;
    size_t statebuf_size;
    int i, len;
    const int *peers;
    plan_ma_msg_t *msg;

    fprintf(stderr, "Th[%d] State %d op %lx\n",
            th->comm->node_id, node->state_id, (long)node->op);
    if (node->op == NULL || planOpExtraMAOpIsPrivate(node->op))
        return;

    statebuf = planStatePoolGetPackedState(th->search->state_pool,
                                           node->state_id);
    statebuf_size = planStatePackerBufSize(th->search->state_pool->packer);
    if (statebuf == NULL)
        return;

    peers = planOpExtraMAOpRecvAgents(node->op, &len);
    if (len == 0)
        return;

    msg = planMAMsgNew(PLAN_MA_MSG_PUBLIC_STATE, 0, th->comm->node_id);
    planMAMsgPublicStateSetState(msg, statebuf, statebuf_size,
                                 node->state_id, node->cost,
                                 node->heuristic);

    for (i = 0; i < len; ++i){
        if (peers[i] != th->comm->node_id)
            planMACommSendToNode(th->comm, peers[i], msg);
    }
    planMAMsgDel(msg);
}

static void publicStateRecv(plan_ma_search_th_t *th,
                            plan_ma_msg_t *msg)
{
    int cost, heur;
    pub_state_data_t *pub_state;
    plan_state_id_t state_id;
    plan_state_space_node_t *node;
    const void *packed_state;
    int insert = 0;

    // Unroll data from the message
    packed_state = planMAMsgPublicStateStateBuf(msg);
    cost         = planMAMsgPublicStateCost(msg);
    heur         = planMAMsgPublicStateHeur(msg);

    /*TODO
    // Skip nodes that are worse than the best goal state so far
    if (search->ma_ack_solution && cost > search->best_goal_cost)
        return -1;
    */

    // Insert packed state into state-pool if not already inserted
    state_id = planStatePoolInsertPacked(th->search->state_pool,
                                         packed_state);

    // Get public state reference data
    pub_state = planStatePoolData(th->search->state_pool,
                                  th->pub_state_reg, state_id);

    // Get corresponding node
    node = planStateSpaceNode(th->search->state_space, state_id);

    // TODO: Heuristic re-computation

    // Determine whether to insert this state into open-list.
    // Either it is new state (thus agent_id == -1) or it has lower
    // heuristic than is currently set.
    if (pub_state->agent_id == -1
            || node->heuristic > heur){
        insert = 1;
    }

    // Set the reference data only if the new cost is smaller than the
    // current one. This means that either the state is brand new or the
    // previously inserted state had bigger cost.
    if (pub_state->cost > cost){
        pub_state->agent_id = planMAMsgAgent(msg);
        pub_state->cost     = cost;
        pub_state->state_id = planMAMsgPublicStateStateId(msg);
    }

    if (insert){
        fprintf(stderr, "Th[%d] INS\n", th->comm->node_id);
        node->parent_state_id = PLAN_NO_STATE;
        node->op              = NULL;
        node->cost            = cost;
        node->heuristic       = heur;
        planSearchInsertNode(th->search, node);
    }
}
