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

    ma_search = planMASearchNew(&params);
    th->res = planMASearchRun(ma_search, &th->path);
    planMASearchDel(ma_search);
}

static void runMALMCut(plan_problem_agents_t *p,
                       int optimal_cost)
{
    plan_search_astar_params_t params;
    plan_search_t *search;
    plan_heur_t *heur;
    bor_tasks_t *tasks;
    th_t th[p->agent_size];
    int i;
    int found = 0;


    tasks = borTasksNew(p->agent_size);
    for (i = 0; i < p->agent_size; ++i){
        heur = planHeurLMCutNew(p->agent[i].var, p->agent[i].var_size,
                                p->agent[i].goal,
                                p->agent[i].proj_op, p->agent[i].proj_op_size);

        planSearchAStarParamsInit(&params);
        params.search.heur = heur;
        params.search.heur_del = 1;
        params.search.prob = &p->agent[i];
        search = planSearchAStarNew(&params);

        th[i].search = search;
        th[i].comm = planMACommInprocNew(i, p->agent_size);
        planPathInit(&th[i].path);
        borTasksAdd(tasks, maLMCutTask, i, th + i);
    }

    borTasksRun(tasks);
    borTasksDel(tasks);

    for (i = 0; i < p->agent_size; ++i){
        if (th[i].res == PLAN_SEARCH_FOUND){
            assertEquals(planPathCost(&th[i].path), optimal_cost);
            found = 1;
        }

        planSearchDel(th[i].search);
        planPathFree(&th[i].path);
        planMACommDel(th[i].comm);
    }

    assertTrue(found);
}

static void maSearch(const char *proto, int optimal_cost)
{
    plan_problem_agents_t *p;

    p = planProblemAgentsFromProto(proto, PLAN_PROBLEM_USE_CG);
    runMALMCut(p, optimal_cost);
    planProblemAgentsDel(p);
}

TEST(testMASearch)
{
    //maSearch("../data/ma-benchmarks/driverlog/pfile3.proto");
    maSearch("../data/ma-benchmarks/depot/pfile1.proto", 10);
}
