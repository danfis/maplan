#ifndef TEST_HEUR_H
#define TEST_HEUR_H

#define TEST_TS_HEUR(name) \
    TEST(testHeur##name); \
    TEST_SUITE(TSHeur##name) { \
        TEST_ADD(testHeur##name), \
        TEST_ADD(protobufTearDown), \
        TEST_SUITE_CLOSURE \
    }

TEST(protobufTearDown);

TEST_TS_HEUR(Relax);
TEST_TS_HEUR(GoalCount);
TEST_TS_HEUR(RelaxAdd);
TEST_TS_HEUR(RelaxMax);
TEST_TS_HEUR(RelaxFF);
TEST_TS_HEUR(LMCut);
TEST_TS_HEUR(DTG);

TEST(testHeurLMCutIncLocal);
TEST(testHeurLMCutIncCache);
TEST(testHeurLMCutIncCachePrune);
TEST_SUITE(TSHeurLMCutInc) {
    TEST_ADD(testHeurLMCutIncLocal),
    TEST_ADD(testHeurLMCutIncCache),
    TEST_ADD(testHeurLMCutIncCachePrune),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

TEST(testHeurFlow);
TEST(testHeurFlowLandmarks);
TEST(testHeurFlowILP);
TEST_SUITE(TSHeurFlow) {
    TEST_ADD(testHeurFlow),
    TEST_ADD(testHeurFlowLandmarks),
    TEST_ADD(testHeurFlowILP),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

TEST(testHeurPotential);
TEST_SUITE(TSHeurPotential) {
    TEST_ADD(testHeurPotential),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#define TEST_SUITE_ADD_HEUR \
    TEST_SUITE_ADD(TSHeurRelax), \
    TEST_SUITE_ADD(TSHeurGoalCount), \
    TEST_SUITE_ADD(TSHeurRelaxAdd), \
    TEST_SUITE_ADD(TSHeurRelaxMax), \
    TEST_SUITE_ADD(TSHeurRelaxFF), \
    TEST_SUITE_ADD(TSHeurLMCut), \
    TEST_SUITE_ADD(TSHeurLMCutInc), \
    TEST_SUITE_ADD(TSHeurDTG), \
    TEST_SUITE_ADD(TSHeurFlow), \
    TEST_SUITE_ADD(TSHeurPotential)

#endif
