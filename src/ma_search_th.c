#include "ma_search_th.h"

static void *searchThread(void *data);
static int searchThreadPostStep(plan_search_t *search, void *ud);

void planMASearchThInit(plan_ma_search_th_t *th,
                        plan_search_t *search,
                        plan_ma_comm_t *comm,
                        plan_path_t *path)
{
    th->search = search;
    th->comm = comm;
    th->path = path;

    borFifoSemInit(&th->msg_queue, sizeof(int));

    th->res = -1;
}

void planMASearchThFree(plan_ma_search_th_t *th)
{
    // TODO: Empty queue
    borFifoSemFree(&th->msg_queue);
}

void planMASearchThRun(plan_ma_search_th_t *th)
{
    pthread_create(&th->thread, NULL, searchThread, th);
}

void planMASearchThJoin(plan_ma_search_th_t *th)
{
    pthread_join(th->thread, NULL);
}

static void *searchThread(void *data)
{
    plan_ma_search_th_t *th = data;

    planSearchSetPostStep(th->search, searchThreadPostStep, th);

    th->res = planSearchRun(th->search, th->path);
    if (th->res == PLAN_SEARCH_FOUND)
        planMASearchTerminate(th->comm);
    return NULL;
}

static int searchThreadPostStep(plan_search_t *search, void *ud)
{
    plan_ma_search_th_t *th = ud;

    if (search->result == PLAN_SEARCH_FOUND){
        // TODO: Verify solution
        return PLAN_SEARCH_FOUND;

    }else if (search->result == PLAN_SEARCH_NOT_FOUND){
        // TODO: Block until public-state or terminate isn't received
        fprintf(stderr, "[%d] NOT FOUND\n", th->comm->node_id);
        fflush(stderr);

    }else{
        // TODO: Process messages
    }

    return search->result;
}
