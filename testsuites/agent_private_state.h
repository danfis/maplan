#ifndef TEST_AGENT_PRIVATE_STATE_H
#define TEST_AGENT_PRIVATE_STATE_H

TEST(testAgentPrivateState);
TEST(protobufTearDown);

TEST_SUITE(TSAgentPrivateState) {
    TEST_ADD(testAgentPrivateState),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif /* TEST_AGENT_PRIVATE_STATE_H */
