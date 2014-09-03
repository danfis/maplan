#ifndef TEST_LIST_LAZY_H
#define TEST_LIST_LAZY_H

TEST(testListLazyHeap);
TEST(testListLazyBucket);
TEST(protobufTearDown);

TEST_SUITE(TSListLazy){
    TEST_ADD(testListLazyHeap),
    TEST_ADD(testListLazyBucket),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
