#include <cu/cu.h>
#include "plan/pddl.h"

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

TEST(testPDDL)
{
    dumpDomain("pddl/depot-domain.pddl", "pddl/depot-pfile1.pddl");
    dumpDomain("pddl/driverlog-domain.pddl", "pddl/driverlog-pfile1.pddl");
    dumpDomain("pddl/openstacks-p03-domain.pddl", "pddl/openstacks-p03.pddl");
    dumpDomain("pddl/rovers-domain.pddl", "pddl/rovers-p01.pddl");
    dumpDomain("pddl/CityCar-domain.pddl", "pddl/CityCar-p3-2-2-0-1.pddl");
}
