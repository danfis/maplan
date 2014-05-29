#ifndef TEST_SUCCGEN_H
#define TEST_SUCCGEN_H

TEST(testSuccGen);

TEST_SUITE(TSSuccGen){
    TEST_ADD(testSuccGen),
    TEST_SUITE_CLOSURE
};

#endif
