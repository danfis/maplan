#include <cu/cu.h>
#include <stdio.h>
#include "plan/dataarr.h"

TEST(dataarrTest)
{
    plan_data_arr_t *arr;
    char d[12];
    char *d2;

    memset(d, 1, 12);

    arr = planDataArrNew(12, 8196, d);
    d2 = (char *)planDataArrGet(arr, 0);
    assertEquals(memcmp(d2, d, 12), 0);

    d2 = (char *)planDataArrGet(arr, 10);
    assertEquals(memcmp(d2, d, 12), 0);

    d2[0] = 2;

    d2 = (char *)planDataArrGet(arr, 5);
    assertEquals(memcmp(d2, d, 12), 0);

    d2 = (char *)planDataArrGet(arr, 10);
    assertNotEquals(memcmp(d2, d, 12), 0);

    planDataArrDel(arr);
}
