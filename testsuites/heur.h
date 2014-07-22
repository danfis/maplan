#ifndef TEST_HEUR_H
#define TEST_HEUR_H

TEST(testHeurRelaxAdd);
TEST(testHeurRelaxMax);
TEST(testHeurRelaxFF);
TEST(testHeurRelaxLMCut);

TEST_SUITE(TSHeur) {
    TEST_ADD(testHeurRelaxAdd),
    TEST_ADD(testHeurRelaxMax),
    TEST_ADD(testHeurRelaxFF),
    TEST_ADD(testHeurRelaxLMCut),
    TEST_SUITE_CLOSURE
};

#endif
