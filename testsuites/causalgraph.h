#ifndef TEST_CAUSALGRAPH_H
#define TEST_CAUSALGRAPH_H

TEST(testCausalGraphImportantVar);

TEST_SUITE(TSCausalGraph){
    TEST_ADD(testCausalGraphImportantVar),
    TEST_SUITE_CLOSURE
};

#endif
