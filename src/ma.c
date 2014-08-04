#include <boruvka/alloc.h>
#include <boruvka/tasks.h>
#include <plan/ma.h>

struct _th_t {
    plan_search_t *search;
    plan_search_ma_params_t params;

    plan_path_t path;
    int res;
};
typedef struct _th_t th_t;

static void taskRun(int id, void *_th, const bor_tasks_thinfo_t *thinfo)
{
    th_t *th = _th;
    planPathInit(&th->path);
    th->res = planSearchMARun(th->search, &th->params, &th->path);
}

int planMARun(int agent_size, plan_search_t **search, plan_path_t *path)
{
    bor_tasks_t *tasks;
    plan_ma_comm_queue_pool_t *queue_pool;
    th_t th[agent_size];
    int i, res = PLAN_SEARCH_NOT_FOUND;

    // create pool of communication queues
    queue_pool = planMACommQueuePoolNew(agent_size);

    // create agent nodes
    for (i = 0; i < agent_size; ++i){
        th[i].search = search[i];
        th[i].params.comm = planMACommQueue(queue_pool, i);
    }

    // run agents in parallel
    tasks = borTasksNew(agent_size);
    for (i = 0; i < agent_size; ++i){
        borTasksAdd(tasks, taskRun, i, &th[i]);
    }

    borTasksRun(tasks);
    borTasksDel(tasks);

    // retrieve results
    for (i = 0; i < agent_size; ++i){
        if (!planPathEmpty(&th[i].path)){
            res = PLAN_SEARCH_FOUND;
            if (path)
                planPathCopy(path, &th[i].path);
            break;
        }
    }
    for (i = 0; i < agent_size; ++i){
        planPathFree(&th[i].path);
    }

    planMACommQueuePoolDel(queue_pool);

    return res;
}
