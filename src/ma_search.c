#include <pthread.h>
#include <boruvka/alloc.h>

#include "plan/ma_search.h"

#include "ma_search_msg_handler.h"

/** Maximal time (in ms) for which is multi-agent thread blocked when
 *  dead-end is reached. This constant is here basicaly to prevent
 *  dead-lock when all threads are in dead-end. So, it should be set to
 *  fairly high number. */
#define SEARCH_MA_MAX_BLOCK_TIME (600 * 1000) // 10 minutes


/** Thread with main search algorithm */
struct _plan_ma_search_th_t {
    pthread_t thread;
    plan_search_t *search;
    plan_ma_comm_t *comm;

    // TODO: Replace this with some lock-free option!
    bor_fifo_t msg_queue;
    pthread_mutex_t msg_queue_lock;

    int res; /*!< Result of search */
    plan_path_t *path;
};
typedef struct _plan_ma_search_th_t plan_ma_search_th_t;

/** Creates and starts a search thread */
static void searchThreadCreate(plan_ma_search_th_t *th,
                               plan_search_t *search,
                               plan_ma_comm_t *comm,
                               plan_path_t *path);
/** Blocks until search thread isn't finished */
static void searchThreadJoin(plan_ma_search_th_t *th);
/** Free all resources */
static void searchThreadFree(plan_ma_search_th_t *th);
/** Main search function */
static void *searchTh(void *);

void planMASearchParamsInit(plan_ma_search_params_t *params)
{
    bzero(params, sizeof(*params));
}

plan_ma_search_t *planMASearchNew(plan_ma_search_params_t *params)
{
    plan_ma_search_t *ma_search;

    ma_search = BOR_ALLOC(plan_ma_search_t);
    ma_search->search = params->search;
    ma_search->comm = params->comm;
    ma_search->terminated = 0;
    return ma_search;
}

void planMASearchDel(plan_ma_search_t *ma_search)
{
    BOR_FREE(ma_search);
}

int planMASearchRun(plan_ma_search_t *ma_search, plan_path_t *path)
{
    plan_ma_search_th_t th;
    plan_ma_search_msg_handler_t handler;
    plan_ma_msg_t *msg = NULL;
    int res;

    ma_search->terminated = 0;

    // Initialize message handler
    planMASearchMsgHandlerInit(&handler, ma_search->comm);

    // Start search algorithm
    searchThreadCreate(&th, ma_search->search, ma_search->comm, path);

    // The main thread is here for processing messages
    while (1){
        // Wait for the next message
        msg = planMACommRecvBlock(ma_search->comm, SEARCH_MA_MAX_BLOCK_TIME);
        if (msg == NULL){
            // TODO
            break;

        }else{
            if (planMASearchMsgHandler(&handler, msg) != 0)
                break;
        }

        planMAMsgDel(msg);
    }

    if (msg)
        planMAMsgDel(msg);

    // Wait for search algorithm to end
    searchThreadJoin(&th);
    res = th.res;

    // Free resources
    searchThreadFree(&th);

    planMASearchMsgHandlerFree(&handler);
    return res;
}


static void searchThreadCreate(plan_ma_search_th_t *th,
                               plan_search_t *search,
                               plan_ma_comm_t *comm,
                               plan_path_t *path)
{
    th->search = search;
    th->comm = comm;
    th->path = path;

    borFifoInit(&th->msg_queue, sizeof(plan_ma_msg_t *));
    pthread_create(&th->thread, NULL, searchTh, th);
}

static void searchThreadJoin(plan_ma_search_th_t *th)
{
    pthread_join(th->thread, NULL);
}

static void searchThreadFree(plan_ma_search_th_t *th)
{
    borFifoFree(&th->msg_queue);
}

static void *searchTh(void *data)
{
    plan_ma_search_th_t *th = data;
    th->res = planSearchRun(th->search, th->path);
    if (th->res == PLAN_SEARCH_FOUND)
        planMASearchTerminate(th->comm);
    return NULL;
}
