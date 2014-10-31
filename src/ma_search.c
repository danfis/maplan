#include <pthread.h>

#include "plan/ma_search.h"

/** Maximal time (in ms) for which is multi-agent thread blocked when
 *  dead-end is reached. This constant is here basicaly to prevent
 *  dead-lock when all threads are in dead-end. So, it should be set to
 *  fairly high number. */
#define SEARCH_MA_MAX_BLOCK_TIME (600 * 1000) // 10 minutes

struct _plan_ma_search_th_t {
    pthread_t thread; /*!< Thread with running search algorithm */

    // TODO: Replace this with some lock-free option!
    bor_fifo_t msg_queue;
    pthread_mutex_t msg_queue_lock;

    int final_state; /*!< State in which the algorithm ended */
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
}

void planMASearchFree(plan_ma_search_t *search)
{
}

int planMASearchRun(plan_ma_search_t *search, plan_path_t *path)
{
    plan_ma_search_th_t search_thread;
    plan_ma_msg_t *msg;

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
    // TODO: Let the parent thread know that we ended (using some internal
    // message or by closing communication channel)

    return NULL;
}

static int searchThreadInitStep(plan_ma_search_th_t *th)
{
    return PLAN_SEARCH_ABORT;
}

static int searchThreadStep(plan_ma_search_th_t *th)
{
    return PLAN_SEARCH_ABORT;
}

static int searchThreadProcessMsgs(plan_ma_search_th_t *th)
{
    return PLAN_SEARCH_ABORT;
}

