#ifndef TEST_SEARCH_ASTAR_H
#define TEST_SEARCH_ASTAR_H

TEST(testSearchAStar);
TEST(protobufTearDown);

TEST_SUITE(TSSearchAStar) {
    TEST_ADD(testSearchAStar),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
