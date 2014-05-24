#include <cu/cu.h>
#include "load-from-file.h"
#include "state.h"

TEST_SUITES {
    TEST_SUITE_ADD(TSLoadFromFile),
    TEST_SUITE_ADD(TSState),
    TEST_SUITES_CLOSURE
};
int main(int argc, char *argv[])
{
    CU_SET_OUT_PREFIX("regressions/");
    CU_RUN(argc, argv);

    return 0;
}
