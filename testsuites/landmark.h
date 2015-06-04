#ifndef TEST_LANDMARK

TEST(testLandmarkCache);
TEST(protobufTearDown);

TEST_SUITE(TSLandmark) {
    TEST_ADD(testLandmarkCache),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};
#endif
