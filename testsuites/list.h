#ifndef TEST_LIST
#define TEST_LIST

TEST(testListTieBreaking);

TEST_SUITE(TSList) {
    TEST_ADD(testListTieBreaking),
    TEST_SUITE_CLOSURE
};

#endif
