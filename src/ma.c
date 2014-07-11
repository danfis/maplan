#include <boruvka/alloc.h>
#include <boruvka/tasks.h>
#include <plan/ma.h>

static void taskRun(int id, void *_agent, const bor_tasks_thinfo_t *thinfo)
{
    plan_ma_agent_t *agent = _agent;
    planMAAgentRun(agent);
}

int planMARun(int agent_size, plan_search_t **search, plan_path_t *path)
{
    bor_tasks_t *tasks;
    plan_ma_comm_queue_pool_t *queue_pool;
    plan_ma_agent_t *agents[agent_size];
    int i;

    // create pool of communication queues
    queue_pool = planMACommQueuePoolNew(agent_size);

    // create agent nodes
    for (i = 0; i < agent_size; ++i){
        agents[i] = planMAAgentNew(search[i], planMACommQueue(queue_pool, i));
    }

    // run agents in parallel
    tasks = borTasksNew(agent_size);
    for (i = 0; i < agent_size; ++i){
        borTasksAdd(tasks, taskRun, i, agents[i]);
    }

    borTasksRun(tasks);
    borTasksDel(tasks);

    // delete agent nodes
    for (i = 0; i < agent_size; ++i){
        planMAAgentDel(agents[i]);
    }

    planMACommQueuePoolDel(queue_pool);

    return 0;
}
