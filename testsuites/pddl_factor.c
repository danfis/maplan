#include <stdarg.h>
#include <pthread.h>
#include <cu/cu.h>
#include <boruvka/alloc.h>
#include "plan/pddl.h"
#include "plan/pddl_ground.h"
#include "plan/pddl_sas.h"

static void dumpDomain(const char *domain_fn, const char *problem_fn)
{
    plan_pddl_t *d;

    d = planPDDLNew(domain_fn, problem_fn);
    if (d == NULL)
        return;

    printf("---- Domain: %s | %s ----\n", domain_fn, problem_fn);
    planPDDLDump(d, stdout);
    printf("---- Domain: %s | %s END ----\n", domain_fn, problem_fn);
    planPDDLDel(d);
}

TEST(testPDDLFactor)
{
    dumpDomain("pddl/depot-pfile1-factor/domain-depot0.pddl",
               "pddl/depot-pfile1-factor/problem-depot0.pddl");
    dumpDomain("pddl/depot-pfile1-factor/domain-distributor0.pddl",
               "pddl/depot-pfile1-factor/problem-distributor0.pddl");
    dumpDomain("pddl/depot-pfile1-factor/domain-distributor1.pddl",
               "pddl/depot-pfile1-factor/problem-distributor1.pddl");
    dumpDomain("pddl/depot-pfile1-factor/domain-driver0.pddl",
               "pddl/depot-pfile1-factor/problem-driver0.pddl");
    dumpDomain("pddl/depot-pfile1-factor/domain-driver1.pddl",
               "pddl/depot-pfile1-factor/problem-driver1.pddl");
}

struct _ground_th_t {
    int id;
    int size;
    int use_tcp;
    plan_pddl_t *pddl;
    plan_pddl_ground_t ground;
    plan_ma_comm_inproc_pool_t *comm_pool;
    pthread_t th;
};
typedef struct _ground_th_t ground_th_t;

static const char *tcp_urls[] = {
    "127.0.0.1:21000",
    "127.0.0.1:21001",
    "127.0.0.1:21002",
    "127.0.0.1:21003",
    "127.0.0.1:21004",
    "127.0.0.1:21005",
    "127.0.0.1:21006",
    "127.0.0.1:21007",
    "127.0.0.1:21008",
    "127.0.0.1:21009",
    "127.0.0.1:21010",
};

static void *groundTh(void *arg)
{
    ground_th_t *th = arg;
    plan_ma_comm_t *comm;

    if (th->use_tcp){
        comm = planMACommTCPNew(th->id, th->size, tcp_urls, 0);
    }else{
        comm = planMACommInprocNew(th->comm_pool, th->id);
    }
    planPDDLGroundFactor(&th->ground, comm);
    planMACommDel(comm);
    return NULL;
}

static void testGround(int size, ...)
{
    va_list ap;
    plan_ma_comm_inproc_pool_t *comm_pool;
    ground_th_t th[size];
    const char *dom[size], *prob[size];
    int i;

    printf("---- Ground ----\n");
    comm_pool = planMACommInprocPoolNew(size);

    va_start(ap, size);
    for (i = 0; i < size; ++i){
        th[i].id = i;
        th[i].size = size;
        th[i].use_tcp = 0;
        th[i].comm_pool = comm_pool;

        dom[i]  = va_arg(ap, const char *);
        prob[i] = va_arg(ap, const char *);

        printf("    |- %s | %s\n", dom[i], prob[i]);
        th[i].pddl = planPDDLNew(dom[i], prob[i]);
        if (th[i].pddl == NULL)
            return;

        planPDDLGroundInit(&th[i].ground, th[i].pddl);
    }
    va_end(ap);

    for (i = 0; i < size; ++i)
        pthread_create(&th[i].th, NULL, groundTh, &th[i]);

    for (i = 0; i < size; ++i)
        pthread_join(th[i].th, NULL);

    for (i = 0; i < size; ++i){
        printf("++++ %s | %s\n", dom[i], prob[i]);
        planPDDLGroundPrint(&th[i].ground, stdout);
        planPDDLGroundFree(&th[i].ground);
        planPDDLDel(th[i].pddl);
    }
    planMACommInprocPoolDel(comm_pool);

    printf("---- Ground END ----\n");
}

TEST(testPDDLFactorGround)
{
    testGround(5, "pddl/depot-pfile1-factor/domain-depot0.pddl",
                  "pddl/depot-pfile1-factor/problem-depot0.pddl",
                  "pddl/depot-pfile1-factor/domain-distributor0.pddl",
                  "pddl/depot-pfile1-factor/problem-distributor0.pddl",
                  "pddl/depot-pfile1-factor/domain-distributor1.pddl",
                  "pddl/depot-pfile1-factor/problem-distributor1.pddl",
                  "pddl/depot-pfile1-factor/domain-driver0.pddl",
                  "pddl/depot-pfile1-factor/problem-driver0.pddl",
                  "pddl/depot-pfile1-factor/domain-driver1.pddl",
                  "pddl/depot-pfile1-factor/problem-driver1.pddl");
}
