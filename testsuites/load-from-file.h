#ifndef TEST_LOAD_FROM_FILE_H
#define TEST_LOAD_FROM_FILE_H

TEST(testLoadFromProto);
TEST(testLoadFromProtoCondEff);
TEST(testLoadAgentFromProto);
TEST(testLoadFromProtoClone);
TEST(testLoadAgentFromProtoClone);
TEST(testLoadFromFactoredProto);

TEST(testLoadFromPDDL);

TEST(protobufTearDown);

TEST_SUITE(TSLoadFromFile) {
    TEST_ADD(testLoadFromProto),
    TEST_ADD(testLoadFromProtoCondEff),
    TEST_ADD(testLoadAgentFromProto),
    TEST_ADD(testLoadFromProtoClone),
    TEST_ADD(testLoadAgentFromProtoClone),
    TEST_ADD(testLoadFromFactoredProto),

    TEST_ADD(testLoadFromPDDL),

    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};


#endif /* TEST_LOAD_FROM_FILE_H */
