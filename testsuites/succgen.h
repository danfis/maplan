#ifndef TEST_SUCCGEN_H
#define TEST_SUCCGEN_H

TEST(testSuccGen);
TEST(protobufTearDown);

TEST_SUITE(TSSuccGen){
    TEST_ADD(testSuccGen),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
