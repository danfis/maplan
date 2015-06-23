#include <cu/cu.h>
#include "plan/pddl.h"

static void dumpDomain(const char *fn)
{
    plan_pddl_domain_t *d;

    d = planPDDLDomainNew(fn);
    if (d == NULL)
        return;

    printf("---- Domain: %s ----\n", fn);
    planPDDLDomainDump(d, stdout);
    printf("---- Domain: %s END ----\n", fn);
    planPDDLDomainDel(d);
}

TEST(testPDDL)
{
    dumpDomain("pddl/depot-domain.pddl");
    dumpDomain("pddl/driverlog-domain.pddl");
    dumpDomain("pddl/openstacks-p03-domain.pddl");
    dumpDomain("pddl/rovers-domain.pddl");
    dumpDomain("pddl/CityCar-domain.pddl");
}
