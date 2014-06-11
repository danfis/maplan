#ifndef TEST_LIST_LAZY_H
#define TEST_LIST_LAZY_H

TEST(testListLazyHeap);

TEST_SUITE(TSListLazy){
    TEST_ADD(testListLazyHeap),
    TEST_SUITE_CLOSURE
};

#endif
