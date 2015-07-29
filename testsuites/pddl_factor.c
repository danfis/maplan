#include <stdarg.h>
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

static void testGround(int size, ...)
{
    va_list ap;
    plan_pddl_t **pddl;
    plan_pddl_ground_t *ground;
    plan_ma_comm_inproc_pool_t *comm_pool;
    plan_ma_comm_t **comm;
    const char *dom, *prob;
    int i;

    printf("---- Ground ----\n");
    pddl = BOR_ALLOC_ARR(plan_pddl_t *, size);
    ground = BOR_ALLOC_ARR(plan_pddl_ground_t, size);
    comm_pool = planMACommInprocPoolNew(size);
    comm = BOR_ALLOC_ARR(plan_ma_comm_t *, size);
    va_start(ap, size);
    for (i = 0; i < size; ++i){
        dom  = va_arg(ap, const char *);
        prob = va_arg(ap, const char *);

        printf("    |- %s | %s\n", dom, prob);
        pddl[i] = planPDDLNew(dom, prob);
        if (pddl[i] == NULL)
            return;

        planPDDLGroundInit(ground + i, pddl[i]);
        comm[i] = planMACommInprocNew(comm_pool, size);
    }
    va_end(ap);

    for (i = 0; i < size; ++i)
        planPDDLGroundFactor(ground + i, comm[i]);

    for (i = 0; i < size; ++i){
        printf("++++++++++++++++\n");
        planPDDLGroundPrint(ground + i, stdout);
        planPDDLGroundFree(ground + i);
        planPDDLDel(pddl[i]);
        planMACommDel(comm[i]);
    }
    planMACommInprocPoolDel(comm_pool);

    BOR_FREE(pddl);
    BOR_FREE(ground);
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
