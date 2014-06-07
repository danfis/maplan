#ifndef TEST_HEUR_H
#define TEST_HEUR_H

TEST(testHeurRelaxAdd);
TEST(testHeurRelaxMax);

TEST_SUITE(TSHeur) {
    TEST_ADD(testHeurRelaxAdd),
    TEST_ADD(testHeurRelaxMax),
    TEST_SUITE_CLOSURE
};

#endif
