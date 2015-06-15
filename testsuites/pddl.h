#ifndef TEST_PDDL_H
#define TEST_PDDL_H

TEST(testPDDL);
TEST(protobufTearDown);
TEST_SUITE(TSPDDL) {
    TEST_ADD(testPDDL),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif

