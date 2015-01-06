#ifndef TEST_MA_COMM_NANOMSG
#define TEST_MA_COMM_NANOMSG

TEST(testMACommNanomsgPingPong);
TEST(protobufTearDown);

TEST_SUITE(TSMACommNanomsg){
    TEST_ADD(testMACommNanomsgPingPong),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
