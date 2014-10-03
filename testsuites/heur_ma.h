#ifndef TEST_HEUR_MA_H
#define TEST_HEUR_MA_H

TEST(testHeurMAMax);
TEST(protobufTearDown);

TEST_SUITE(TSHeurMA) {
    TEST_ADD(testHeurMAMax),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
