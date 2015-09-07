#include <cu/cu.h>
#include <pthread.h>
#include "plan/heur.h"
#include "plan/ma_search.h"

static const char *tcp_addr[] = {
    "127.0.0.1:13910",
    "127.0.0.1:13911",
    "127.0.0.1:13912",
    "127.0.0.1:13913",
    "127.0.0.1:13914",
    "127.0.0.1:13915",
    "127.0.0.1:13916",
    "127.0.0.1:13917",
    "127.0.0.1:13918",
};

struct _th_t {
    pthread_t th;
    plan_search_t *search;
    plan_ma_comm_t *comm;
    int cost;
};
typedef struct _th_t th_t;

static void *runTh(void *data)
{
    th_t *th = data;
    plan_ma_search_params_t param;
    plan_ma_search_t *s;
    plan_path_t path;

    planMASearchParamsInit(&param);
    param.search = th->search;
    param.comm = th->comm;
    param.verify_solution = 1;

    s = planMASearchNew(&param);

    planPathInit(&path);
    planMASearchRun(s, &path);
    th->cost = planPathCost(&path);
    planPathFree(&path);

    planMASearchDel(s);

    return NULL;
}

static void runAStar(unsigned flags, int expected_cost,
                     int num_proto, ...)
{
    plan_problem_t *p[num_proto];
    plan_ma_comm_t *comm[num_proto];
    plan_search_t *search[num_proto];
    plan_search_astar_params_t sparams;
    th_t th[num_proto];
    unsigned prob_flags;
    const char *proto[num_proto];
    va_list ap;
    int i;

    prob_flags  = PLAN_PROBLEM_MA_STATE_PRIVACY;
    prob_flags |= PLAN_PROBLEM_NUM_AGENTS(num_proto);

    va_start(ap, num_proto);
    for (i = 0; i < num_proto; ++i){
        proto[i] = va_arg(ap, const char *);
        p[i] = planProblemFromProto(proto[i], prob_flags);
        if (p[i] == NULL){
            fprintf(stderr, "Error: Could not load `%s'\n", proto[i]);
            exit(-1);
        }

        comm[i] = planMACommTCPNew(i, num_proto, tcp_addr);
        if (comm[i] == NULL)
            exit(-1);

        planSearchAStarParamsInit(&sparams);
        sparams.search.heur = planHeurMAPotentialNew(p[i], flags);
        sparams.search.heur_del = 1;
        sparams.search.prob = p[i];
        search[i] = planSearchAStarNew(&sparams);

        th[i].search = search[i];
        th[i].comm = comm[i];
        th[i].cost = -1;
    }
    va_end(ap);

    for (i = 0; i < num_proto; ++i)
        pthread_create(&th[i].th, NULL, runTh, &th[i]);

    for (i = 0; i < num_proto; ++i){
        pthread_join(th[i].th, NULL);
        if (th[i].cost != expected_cost)
            fprintf(stderr, "%s: cost: %d, expcted-cost: %d\n",
                    proto[i], th[i].cost, expected_cost);
        assertEquals(th[i].cost, expected_cost);
    }

    for (i = 0; i < num_proto; ++i){
        planSearchDel(search[i]);
        planMACommDel(comm[i]);
        planProblemDel(p[i]);
    }
}

TEST(testHeurMAPot)
{
    int i;

    for (i = 0; i < 5; ++i){
        runAStar(0, 10, 5,
                 "proto/depot-pfile1-depot0.proto",
                 "proto/depot-pfile1-distributor0.proto",
                 "proto/depot-pfile1-distributor1.proto",
                 "proto/depot-pfile1-truck0.proto",
                 "proto/depot-pfile1-truck1.proto");

        runAStar(0, 7, 2,
                 "proto/driverlog-pfile1-driver1.proto",
                 "proto/driverlog-pfile1-driver2.proto");
        runAStar(0, 11, 2,
                 "proto/rovers-p03-rover0.proto",
                 "proto/rovers-p03-rover1.proto");
    }
}
