#ifndef TEST_LOAD_FROM_FILE_H
#define TEST_LOAD_FROM_FILE_H

TEST(testLoadFromProto);
TEST(testLoadFromProtoCondEff);
TEST(testLoadAgentFromProto);
TEST(testLoadFromProtoClone);
TEST(protobufTearDown);

TEST_SUITE(TSLoadFromFile) {
    TEST_ADD(testLoadFromProto),
    TEST_ADD(testLoadFromProtoCondEff),
    TEST_ADD(testLoadAgentFromProto),
    TEST_ADD(testLoadFromProtoClone),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};


#endif /* TEST_LOAD_FROM_FILE_H */
