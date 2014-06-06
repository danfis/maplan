#ifndef TEST_HEUR_H
#define TEST_HEUR_H

TEST(testHeurRelax);

TEST_SUITE(TSHeur) {
    TEST_ADD(testHeurRelax),
    TEST_SUITE_CLOSURE
};

#endif
