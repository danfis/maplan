#ifndef TEST_CAUSALGRAPH_H
#define TEST_CAUSALGRAPH_H

TEST(testCausalGraphImportantVar);
TEST(testCausalGraphVarOrder);
TEST(testCausalGraphPruneUnimportantVar);

TEST_SUITE(TSCausalGraph){
    TEST_ADD(testCausalGraphImportantVar),
    TEST_ADD(testCausalGraphVarOrder),
    TEST_ADD(testCausalGraphPruneUnimportantVar),
    TEST_SUITE_CLOSURE
};

#endif
