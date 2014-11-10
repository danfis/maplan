#include "ma_search_th.h"
#include "ma_search_common.h"

static void *searchThread(void *data);
static int searchThreadPostStep(plan_search_t *search, int res, void *ud);
static void processMsg(plan_ma_search_th_t *th, plan_ma_msg_t *msg);

void planMASearchThInit(plan_ma_search_th_t *th,
                        plan_search_t *search,
                        plan_ma_comm_t *comm,
                        plan_path_t *path)
{
    th->search = search;
    th->comm = comm;
    th->path = path;

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
    planSearchRun(th->search, th->path);

    return NULL;
}

static int searchThreadPostStep(plan_search_t *search, int res, void *ud)
{
    plan_ma_search_th_t *th = ud;
    plan_ma_msg_t *msg = NULL;
    int type = -1;

    if (res == PLAN_SEARCH_FOUND){
        // TODO: Verify solution
        planMASearchTerminate(th->comm);
        th->res = PLAN_SEARCH_FOUND;
        res = PLAN_SEARCH_CONT;

    }else if (res == PLAN_SEARCH_ABORT){
        // Initialize termination of a whole cluster
        planMASearchTerminate(th->comm);

        // Ignore all messages except terminate
        do {
            borFifoSemPopBlock(&th->msg_queue, &msg);
            if (msg)
                planMAMsgDel(msg);
        } while (msg);

    }else if (res == PLAN_SEARCH_NOT_FOUND){
        fprintf(stderr, "[%d] NOT FOUND\n", th->comm->node_id);
        fflush(stderr);

        // Block until public-state or terminate isn't received
        do {
            borFifoSemPopBlock(&th->msg_queue, &msg);

            if (msg){
                type = planMAMsgType(msg);
                processMsg(th, msg);
                planMAMsgDel(msg);
            }
        } while (msg && type != PLAN_MA_MSG_PUBLIC_STATE);

        // Empty message means to terminate
        if (!msg)
            return PLAN_SEARCH_ABORT;
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

    return res;
}

static void processMsg(plan_ma_search_th_t *th, plan_ma_msg_t *msg)
{
    // TODO
}
