#ifndef TEST_FA_MUTEX
#define TEST_FA_MUTEX

TEST(testFAMutex);
TEST(testFAMutexGoal);
TEST_SUITE(TSFAMutex) {
    TEST_ADD(testFAMutex),
    TEST_ADD(testFAMutexGoal),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif /* TEST_FA_MUTEX */
