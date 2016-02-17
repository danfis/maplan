#ifndef TEST_PDDL_INV_H
#define TEST_PDDL_INV_H

TEST(testPDDLInv);
TEST(protobufTearDown);
TEST_SUITE(TSPDDLInv) {
    TEST_ADD(testPDDLInv),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
