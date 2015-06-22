#include <cu/cu.h>
#include "src/pddl_lisp.h"

static void testLisp(const char *fn)
{
    plan_pddl_lisp_t *lisp;

    printf("---- %s ----\n", fn);
    lisp = planPDDLLispParse(fn);
    planPDDLLispDump(lisp, stdout);
    planPDDLLispDel(lisp);
    printf("---- %s END ----\n", fn);
}

TEST(testLispFile)
{
    testLisp("pddl/CityCar-domain.pddl");
    testLisp("pddl/CityCar-p3-2-2-0-1.pddl");
    testLisp("pddl/depot-domain.pddl");
    testLisp("pddl/depot-pfile1.pddl");
    testLisp("pddl/depot-pfile2.pddl");
    testLisp("pddl/depot-pfile5.pddl");
    testLisp("pddl/driverlog-domain.pddl");
    testLisp("pddl/driverlog-pfile1.pddl");
    testLisp("pddl/driverlog-pfile3.pddl");
    testLisp("pddl/openstacks-p03-domain.pddl");
    testLisp("pddl/openstacks-p03.pddl");
    testLisp("pddl/rovers-domain.pddl");
    testLisp("pddl/rovers-p01.pddl");
    testLisp("pddl/rovers-p02.pddl");
    testLisp("pddl/rovers-p03.pddl");
    testLisp("pddl/rovers-p15.pddl");
}
