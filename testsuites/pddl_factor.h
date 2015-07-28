#ifndef TEST_PDDL_FACTOR_H
#define TEST_PDDL_FACTOR_H

TEST(testPDDLFactor);
TEST(testPDDLFactorGround);
TEST(protobufTearDown);

TEST_SUITE(TSPDDLFactor) {
    TEST_ADD(testPDDLFactor),
    TEST_ADD(testPDDLFactorGround),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
