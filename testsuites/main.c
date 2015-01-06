#include <cu/cu.h>
#include "plan/ma_msg.h"
#include "load-from-file.h"
#include "state.h"
#include "dataarr.h"
#include "succgen.h"
#include "statespace.h"
//#include "search_ehc.h"
//#include "search_lazy.h"
#include "search_astar.h"
#include "heur.h"
#include "heur_ma.h"
#include "list_lazy.h"
#include "list.h"
#include "ma_comm_nanomsg.h"
#include "causalgraph.h"
#include "ma_search.h"
#include "heur_admissible.h"

TEST(protobufTearDown)
{
    planShutdownProtobuf();
}

TEST_SUITES {
    TEST_SUITE_ADD(TSLoadFromFile),
    TEST_SUITE_ADD(TSState),
    TEST_SUITE_ADD(TSDataArr),
    TEST_SUITE_ADD(TSSuccGen),
    TEST_SUITE_ADD(TSStateSpace),
    //TEST_SUITE_ADD(TSSearchEHC),
    //TEST_SUITE_ADD(TSSearchLazy),
    TEST_SUITE_ADD(TSSearchAStar),
    TEST_SUITE_ADD(TSHeur),
    TEST_SUITE_ADD(TSHeurMA),
    TEST_SUITE_ADD(TSListLazy),
    TEST_SUITE_ADD(TSList),
    TEST_SUITE_ADD(TSMACommNanomsg),
    TEST_SUITE_ADD(TSCausalGraph),
    TEST_SUITE_ADD(TSMASearch),
    TEST_SUITE_ADD(TSHeurAdmissible),
    TEST_SUITES_CLOSURE
};

int main(int argc, char *argv[])
{
    CU_SET_OUT_PREFIX("regressions/");
    CU_RUN(argc, argv);

    planShutdownProtobuf();
    return 0;
}
