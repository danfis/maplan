#ifndef TEST_LOAD_FROM_FILE_H
#define TEST_LOAD_FROM_FILE_H

TEST(testLoadFromFD);
TEST(testLoadFromFD2);
TEST(testLoadAgentFromFD);
TEST(testLoadCondEffFromFD);

TEST_SUITE(TSLoadFromFile) {
    TEST_ADD(testLoadFromFD),
    TEST_ADD(testLoadFromFD2),
    TEST_ADD(testLoadAgentFromFD),
    TEST_ADD(testLoadCondEffFromFD),
    TEST_SUITE_CLOSURE
};


#endif /* TEST_LOAD_FROM_FILE_H */
