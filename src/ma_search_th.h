#ifndef __PLAN_MA_SEARCH_TH_H__
#define __PLAN_MA_SEARCH_TH_H__

#include <pthread.h>
#include <boruvka/fifo-sem.h>

#include <plan/search.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_ma_search_th_t {
    pthread_t thread;
    plan_search_t *search;
    plan_ma_comm_t *comm;
    plan_path_t *path;

    bor_fifo_sem_t msg_queue;

    int res; /*!< Result of search */
};
typedef struct _plan_ma_search_th_t plan_ma_search_th_t;


/**
 * Initializes search thread.
 */
void planMASearchThInit(plan_ma_search_th_t *th,
                        plan_search_t *search,
                        plan_ma_comm_t *comm,
                        plan_path_t *path);

/**
 * Frees allocated resources.
 */
void planMASearchThFree(plan_ma_search_th_t *th);

/**
 * Starts thread with search.
 */
void planMASearchThRun(plan_ma_search_th_t *th);

/**
 * Blocks until search thread ends.
 */
void planMASearchThJoin(plan_ma_search_th_t *th);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_MA_SEARCH_TH_H__ */
