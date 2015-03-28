#ifndef TEST_LISP_FILE
#define TEST_LISP_FILE

TEST(protobufTearDown);
TEST(testLispFile);

TEST_SUITE(TSLispFile){
    TEST_ADD(testLispFile),
    TEST_ADD(protobufTearDown),
    TEST_SUITE_CLOSURE
};

#endif
