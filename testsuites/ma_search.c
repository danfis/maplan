#include <cu/cu.h>
#include <boruvka/tasks.h>
#include <plan/ma_search.h>

struct _th_t {
    plan_search_t *search;
    plan_ma_comm_t *comm;
    plan_path_t path;
    int res;
};
typedef struct _th_t th_t;

static void maLMCutTask(int id, void *data, const bor_tasks_thinfo_t *_)
{
    th_t *th = data;
    plan_ma_search_params_t params;
    plan_ma_search_t *ma_search;

    planMASearchParamsInit(&params);
    params.comm = th->comm;
    params.search = th->search;
    params.verify_solution = 1;

    ma_search = planMASearchNew(&params);
    th->res = planMASearchRun(ma_search, &th->path);
    planMASearchDel(ma_search);
}

static void runMALMCut(int agent_size, plan_problem_t **prob,
                       int optimal_cost)
{
    plan_search_astar_params_t params;
    plan_search_t *search;
    plan_ma_comm_inproc_pool_t *comm_pool;
    plan_heur_t *heur;
    bor_tasks_t *tasks;
    th_t th[agent_size];
    int i;
    int found = 0;


    comm_pool = planMACommInprocPoolNew(agent_size);
    tasks = borTasksNew(agent_size);
    for (i = 0; i < agent_size; ++i){
        heur = planHeurLMCutNew(prob[i]->var, prob[i]->var_size,
                                prob[i]->goal,
                                prob[i]->proj_op, prob[i]->proj_op_size, 0);

        planSearchAStarParamsInit(&params);
        params.search.heur = heur;
        params.search.heur_del = 1;
        params.search.prob = prob[i];
        search = planSearchAStarNew(&params);

        th[i].search = search;
        th[i].comm = planMACommInprocNew(comm_pool, i);
        planPathInit(&th[i].path);
        borTasksAdd(tasks, maLMCutTask, i, th + i);
    }

    borTasksRun(tasks);
    borTasksDel(tasks);

    for (i = 0; i < agent_size; ++i){
        if (th[i].res == PLAN_SEARCH_FOUND){
            assertEquals(planPathCost(&th[i].path), optimal_cost);
            //fprintf(stdout, "cost: %d\n", planPathCost(&th[i].path));
            //planPathPrint(&th[i].path, stdout);
            found = 1;
        }

        planSearchDel(th[i].search);
        planPathFree(&th[i].path);
        planMACommDel(th[i].comm);
    }

    assertTrue(found);
    planMACommInprocPoolDel(comm_pool);
}

static void maSearch(const char *proto, int optimal_cost)
{
    plan_problem_agents_t *p;
    plan_problem_t **prob;
    int i;

    p = planProblemAgentsFromProto(proto, PLAN_PROBLEM_USE_CG);
    prob = alloca(sizeof(plan_problem_t *) * p->agent_size);
    for (i = 0; i < p->agent_size; ++i)
        prob[i] = p->agent + i;
    runMALMCut(p->agent_size, prob, optimal_cost);
    planProblemAgentsDel(p);
}

TEST(testMASearch)
{
    //maSearch("proto/driverlog-pfile3.proto");
    maSearch("proto/depot-pfile1.proto", 10);
    maSearch("proto/driverlog-pfile1.proto", 7);
}


static void maSearchFactored(int optimal_cost, int agent_size, ...)
{
    va_list ap;
    plan_problem_t *prob[agent_size];
    const char *fn;
    int flags, i;

    va_start(ap, agent_size);
    flags  = PLAN_PROBLEM_MA_STATE_PRIVACY;
    flags |= PLAN_PROBLEM_NUM_AGENTS(agent_size);
    for (i = 0; i < agent_size; ++i){
        fn = va_arg(ap, const char *);
        prob[i] = planProblemFromProto(fn, flags);
    }
    va_end(ap);

    runMALMCut(agent_size, prob, optimal_cost);
    for (i = 0; i < agent_size; ++i){
        planProblemDel(prob[i]);
    }
}

TEST(testMASearchFactored)
{
    maSearchFactored(7, 2, "proto/driverlog-pfile1-driver1.proto",
                     "proto/driverlog-pfile1-driver2.proto");
    maSearchFactored(11, 2, "proto/rovers-p03-rover0.proto",
                     "proto/rovers-p03-rover1.proto");
    maSearchFactored(10, 5, "proto/depot-pfile1-depot0.proto",
                     "proto/depot-pfile1-distributor0.proto",
                     "proto/depot-pfile1-distributor1.proto",
                     "proto/depot-pfile1-truck0.proto",
                     "proto/depot-pfile1-truck1.proto");
}
