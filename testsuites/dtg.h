#ifndef TEST_DTG_H
#define TEST_DTG_H

TEST(dtgTest);
TEST(protobufTearDown);

TEST_SUITE(TSDTG) {
    TEST_ADD(dtgTest),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
