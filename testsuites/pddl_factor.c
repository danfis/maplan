#include <cu/cu.h>
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

TEST(testPDDLFactorGround)
{
}
