#include <cu/cu.h>
#include <boruvka/tasks.h>
#include <plan/ma_search.h>

struct _th_t {
    plan_problem_agents_t *p;
    plan_ma_comm_t *comm;
};
typedef struct _th_t th_t;

static void maLMCutTask(int id, void *data, const bor_tasks_thinfo_t *_)
{
    plan_heur_t *heur;
    plan_ma_search_params_t params;
    plan_ma_search_t ma_search;
    plan_path_t path;
    plan_problem_agents_t *p = ((th_t *)data)->p;
    plan_ma_comm_t *comm = ((th_t *)data)->comm;

    heur = planHeurLMCutNew(p->glob.var, p->glob.var_size,
                            p->glob.goal,
                            p->glob.op, p->glob.op_size,
                            p->glob.succ_gen);

    planMASearchParamsInit(&params);
    params.comm = comm;
    params.initial_state = p->glob.initial_state;
    params.goal = p->glob.goal;
    params.state_pool = p->glob.state_pool;
    params.heur = heur;
    params.succ_gen = p->glob.succ_gen;

    planMASearchInit(&ma_search, &params);

    planPathInit(&path);
    //planMASearchRun(&ma_search, &path);

    planMASearchFree(&ma_search);

    planHeurDel(heur);
}

static void runMALMCut(plan_problem_agents_t *p,
                       plan_ma_comm_queue_pool_t *comm_pool)
{
    bor_tasks_t *tasks;
    th_t th[p->agent_size];
    int i;

    tasks = borTasksNew(p->agent_size);
    for (i = 0; i < p->agent_size; ++i){
        th[i].p = p;
        th[i].comm = planMACommQueue(comm_pool, i);
        borTasksAdd(tasks, maLMCutTask, i, th + i);
    }

    borTasksRun(tasks);
    borTasksDel(tasks);
}

TEST(testMASearch)
{
    plan_problem_agents_t *p;
    plan_ma_comm_queue_pool_t *comm_pool;

    p = planProblemAgentsFromProto("../data/ma-benchmarks/driverlog/pfile3.proto",
                                   PLAN_PROBLEM_USE_CG);
    comm_pool = planMACommQueuePoolNew(p->agent_size);

    runMALMCut(p, comm_pool);

    planMACommQueuePoolDel(comm_pool);
    planProblemAgentsDel(p);
}
