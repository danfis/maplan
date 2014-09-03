#ifndef TEST_DATA_ARR_H
#define TEST_DATA_ARR_H

TEST(dataarrTest);
TEST(protobufTearDown);

TEST_SUITE(TSDataArr){
    TEST_ADD(dataarrTest),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif /* TEST_DATA_ARR_H */
