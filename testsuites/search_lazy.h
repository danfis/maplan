#ifndef TEST_SEARCH_LAZY_H
#define TEST_SEARCH_LAZY_H

TEST(testSearchLazy);
TEST(protobufTearDown);

TEST_SUITE(TSSearchLazy) {
    TEST_ADD(testSearchLazy),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif /* TEST_SEARCH_LAZY_H */
