#include <cu/cu.h>
#include "fd/state.h"


TEST(testStateBasic)
{
    fd_state_pool_t *pool;

    pool = fdStatePoolNew(10);
    fdStatePoolDel(pool);
}
