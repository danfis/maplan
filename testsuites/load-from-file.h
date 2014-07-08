#ifndef TEST_LOAD_FROM_FILE_H
#define TEST_LOAD_FROM_FILE_H

TEST(testLoadFromFile);
TEST(testLoadFromFD);
TEST(testLoadFromFD2);
TEST(testLoadAgentFromFD);

TEST_SUITE(TSLoadFromFile) {
    TEST_ADD(testLoadFromFile),
    TEST_ADD(testLoadFromFD),
    TEST_ADD(testLoadFromFD2),
    TEST_ADD(testLoadAgentFromFD),
    TEST_SUITE_CLOSURE
};


#endif /* TEST_LOAD_FROM_FILE_H */
