#ifndef TEST_STATESPACE_H
#define TEST_STATESPACE_H

TEST(testStateSpaceHeapCost);
TEST(testStateSpaceHeapHeuristic);
TEST(testStateSpaceHeapCostHeuristic);
TEST(testStateSpaceFifo);
TEST(testStateSpaceBucketCost);
TEST(testStateSpaceBucketHeuristic);
TEST(testStateSpaceBucketCostHeuristic);

TEST_SUITE(TSStateSpace) {
    TEST_ADD(testStateSpaceHeapCost),
    TEST_ADD(testStateSpaceHeapHeuristic),
    TEST_ADD(testStateSpaceHeapCostHeuristic),
    TEST_ADD(testStateSpaceFifo),
    TEST_ADD(testStateSpaceBucketCost),
    TEST_ADD(testStateSpaceBucketHeuristic),
    TEST_ADD(testStateSpaceBucketCostHeuristic),
    TEST_SUITE_CLOSURE
};

#endif /* TEST_STATESPACE_H */
