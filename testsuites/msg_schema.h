#ifndef TEST_MSG_SCHEMA_H
#define TEST_MSG_SCHEMA_H

TEST(testMsgSchema);
TEST(protobufTearDown);

TEST_SUITE(TSMsgSchema) {
    TEST_ADD(testMsgSchema),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
