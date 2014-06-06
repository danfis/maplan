#ifndef TEST_STATESPACE_H
#define TEST_STATESPACE_H

TEST(testStateSpace);

TEST_SUITE(TSStateSpace) {
    TEST_ADD(testStateSpace),
    TEST_SUITE_CLOSURE
};

#endif /* TEST_STATESPACE_H */
