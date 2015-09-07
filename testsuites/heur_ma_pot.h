#ifndef TEST_HEUR_MA_POT_H
#define TEST_HEUR_MA_POT_H

TEST(testHeurMAPot);
TEST(protobufTearDown);

TEST_SUITE(TSHeurMAPot) {
    TEST_ADD(testHeurMAPot),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif

