#include <cu/cu.h>
#include "plan/state.h"


TEST(testStateBasic)
{
    plan_state_pool_t *pool;

    pool = planStatePoolNew(10);
    planStatePoolDel(pool);
}
