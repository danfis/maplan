#ifndef TEST_STATESPACE_H
#define TEST_STATESPACE_H

TEST(testStateSpaceBasic);

TEST_SUITE(TSStateSpace) {
    TEST_ADD(testStateSpaceBasic),
    TEST_SUITE_CLOSURE
};

#endif /* TEST_STATESPACE_H */
