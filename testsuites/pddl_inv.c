#include <cu/cu.h>
#include "plan/pddl_inv.h"

static void inferInvs(const char *domain_fn, const char *problem_fn)
{
    plan_pddl_t *d;
    plan_pddl_ground_t ground;
    plan_pddl_inv_finder_t invf;

    printf("---- Invariants %s | %s ----\n", domain_fn, problem_fn);
    d = planPDDLNew(domain_fn, problem_fn);
    if (d == NULL)
        return;

    planPDDLGroundInit(&ground, d);
    planPDDLGround(&ground);

    planPDDLInvFinderInit(&invf, &ground);
    planPDDLInvFinder(&invf);
    planPDDLInvFinderFree(&invf);

    //planPDDLGroundPrint(&ground, stdout);
    planPDDLGroundFree(&ground);
    planPDDLDel(d);
    printf("---- Invariants %s | %s END ----\n", domain_fn, problem_fn);
}

TEST(testPDDLInv)
{
    inferInvs("pddl/depot-domain.pddl", "pddl/depot-pfile1.pddl");
    inferInvs("pddl/driverlog-domain.pddl", "pddl/driverlog-pfile1.pddl");
    inferInvs("pddl/openstacks-p03-domain.pddl", "pddl/openstacks-p03.pddl");
    inferInvs("pddl/rovers-domain.pddl", "pddl/rovers-p01.pddl");
    //inferInvs("pddl/CityCar-domain.pddl", "pddl/CityCar-p3-2-2-0-1.pddl");
    inferInvs("pddl/elevators08-domain.pddl", "pddl/elevators08-p01.pddl");
}
