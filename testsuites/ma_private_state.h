#ifndef TEST_MA_PRIVATE_STATE_H
#define TEST_MA_PRIVATE_STATE_H

TEST(testMAPrivateState);
TEST(protobufTearDown);

TEST_SUITE(TSMAPrivateState) {
    TEST_ADD(testMAPrivateState),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif /* TEST_MA_PRIVATE_STATE_H */
