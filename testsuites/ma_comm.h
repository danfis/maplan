#ifndef TEST_MA_COMM_
#define TEST_MA_COMM_

TEST(testMACommPingPong);
TEST(protobufTearDown);

TEST_SUITE(TSMAComm){
    TEST_ADD(testMACommPingPong),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
