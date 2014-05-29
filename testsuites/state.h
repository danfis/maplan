#ifndef TEST_STATE_H
#define TEST_STATE_H

TEST(testStateBasic);
TEST(testStatePreEff);

TEST_SUITE(TSState) {
    TEST_ADD(testStateBasic),
    TEST_ADD(testStatePreEff),
    TEST_SUITE_CLOSURE
};

#endif /* TEST_STATE_H */
