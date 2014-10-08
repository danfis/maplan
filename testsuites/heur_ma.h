#ifndef TEST_HEUR_MA_H
#define TEST_HEUR_MA_H

TEST(testHeurMAMaxDepotPfile1);
TEST(testHeurMAMaxDepotPfile2);
TEST(testHeurMAMaxRoversP03);
TEST(testHeurMAMaxRoversP15);
TEST(protobufTearDown);

TEST_SUITE(TSHeurMA) {
    TEST_ADD(testHeurMAMaxDepotPfile1),
    TEST_ADD(testHeurMAMaxDepotPfile2),
    TEST_ADD(testHeurMAMaxRoversP03),
    TEST_ADD(testHeurMAMaxRoversP15),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
