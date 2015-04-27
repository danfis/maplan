#ifndef TEST_HEUR_FLOW_H
#define TEST_HEUR_FLOW_H

TEST(testHeurFlow);
TEST(testHeurFlowLandmarks);
TEST(testHeurFlowILP);
TEST(protobufTearDown);

TEST_SUITE(TSHeurFlow) {
    TEST_ADD(testHeurFlow),
    TEST_ADD(testHeurFlowLandmarks),
    TEST_ADD(testHeurFlowILP),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
