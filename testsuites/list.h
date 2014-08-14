#ifndef TEST_LIST
#define TEST_LIST

TEST(testListTieBreaking);
TEST(protobufTearDown);

TEST_SUITE(TSList) {
    TEST_ADD(testListTieBreaking),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
