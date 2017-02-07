#include <cu/cu.h>
#include "plan/problem.h"
#include "load-from-file.h"
#include "state.h"
#include "succ_gen.h"
#include "state_space.h"
//#include "search_ehc.h"
//#include "search_lazy.h"
#include "search_astar.h"
#include "heur.h"
#include "heur_ma.h"
#include "heur_ma_pot.h"
#include "list_lazy.h"
#include "list.h"
#include "ma_comm.h"
#include "causal_graph.h"
#include "ma_search.h"
#include "heur_admissible.h"
#include "dtg.h"
#include "ma_private_state.h"
#include "landmark.h"
#include "msg_schema.h"

TEST(protobufTearDown)
{
    planShutdownProtobuf();
}

TEST_SUITES {
    TEST_SUITE_ADD(TSLoadFromFile),
    TEST_SUITE_ADD(TSState),
    TEST_SUITE_ADD(TSSuccGen),
    TEST_SUITE_ADD(TSStateSpace),
    //TEST_SUITE_ADD(TSSearchEHC),
    //TEST_SUITE_ADD(TSSearchLazy),
    TEST_SUITE_ADD(TSSearchAStar),
    TEST_SUITE_ADD_HEUR,
    TEST_SUITE_ADD(TSHeurMA),
    TEST_SUITE_ADD(TSHeurMAPot),
    TEST_SUITE_ADD(TSListLazy),
    TEST_SUITE_ADD(TSList),
    TEST_SUITE_ADD(TSMAComm),
    TEST_SUITE_ADD(TSCausalGraph),
    TEST_SUITE_ADD(TSMASearch),
    TEST_SUITE_ADD(TSHeurAdmissible),
    TEST_SUITE_ADD(TSDTG),
    TEST_SUITE_ADD(TSMAPrivateState),
    TEST_SUITE_ADD(TSLandmark),
    TEST_SUITE_ADD(TSMsgSchema),
    TEST_SUITES_CLOSURE
};

int main(int argc, char *argv[])
{
    CU_SET_OUT_PREFIX("regressions/");
    CU_SET_OUT_PER_TEST(1);
    CU_RUN(argc, argv);

    planShutdownProtobuf();
    return 0;
}
