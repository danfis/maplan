#ifndef TEST_MA_SEARCH_H
#define TEST_MA_SEARCH_H

TEST(testMASearch);
TEST(protobufTearDown);

TEST_SUITE(TSMASearch) {
    TEST_ADD(testMASearch),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
