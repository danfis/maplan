#ifndef TEST_SEARCH_EHC_H
#define TEST_SEARCH_EHC_H

TEST(testSearchEHC);
TEST(protobufTearDown);

TEST_SUITE(TSSearchEHC) {
    TEST_ADD(testSearchEHC),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif /* TEST_SEARCH_EHC_H */
