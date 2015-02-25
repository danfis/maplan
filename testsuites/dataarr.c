#include <cu/cu.h>
#include <stdio.h>
#include "plan/dataarr.h"

void elinit(void *d, const void *u)
{
    assertEquals((long)u, 1UL);
    memset(d, 1, 10);
    ((char *)d)[10] = 2;
    ((char *)d)[11] = 2;
}

TEST(dataarrTest)
{
    plan_data_arr_t *arr, *arr2;
    int i;
    char d[12];
    char *d2;

    memset(d, 1, 12);

    arr = planDataArrNew(12, NULL, d);
    d2 = (char *)planDataArrGet(arr, 0);
    assertEquals(memcmp(d2, d, 12), 0);

    d2 = (char *)planDataArrGet(arr, 10);
    assertEquals(memcmp(d2, d, 12), 0);

    d2[0] = 2;

    d2 = (char *)planDataArrGet(arr, 5);
    assertEquals(memcmp(d2, d, 12), 0);

    d2 = (char *)planDataArrGet(arr, 10);
    assertNotEquals(memcmp(d2, d, 12), 0);

    arr2 = planDataArrClone(arr);
    assertEquals(arr2->num_els, arr->num_els);
    assertEquals(arr2->init_fn, arr->init_fn);
    assertNotEquals(arr2->init_data, arr->init_data);
    assertNotEquals(arr2->arr, arr->arr);
    for (i = 0; i < arr->num_els; ++i){
        assertEquals(memcmp(planDataArrGet(arr2, i), planDataArrGet(arr, i), 12), 0);
    }

    planDataArrDel(arr);
    planDataArrDel(arr2);

    arr = planDataArrNew(12, elinit, (const void *)1UL);
    d2 = (char *)planDataArrGet(arr, 0);
    assertEquals(memcmp(d2, d, 10), 0);

    d2 = (char *)planDataArrGet(arr, 10);
    assertEquals(memcmp(d2, d, 10), 0);

    d2[0] = 2;

    d2 = (char *)planDataArrGet(arr, 5);
    assertEquals(memcmp(d2, d, 10), 0);

    d2 = (char *)planDataArrGet(arr, 10);
    assertNotEquals(memcmp(d2, d, 10), 0);
    d2[0] = 13;

    arr2 = planDataArrClone(arr);
    assertEquals(arr2->num_els, arr->num_els);
    assertEquals(arr2->init_fn, arr->init_fn);
    assertEquals(arr2->init_data, arr->init_data);
    assertNotEquals(arr2->arr, arr->arr);
    for (i = 0; i < arr->num_els; ++i){
        assertEquals(memcmp(planDataArrGet(arr2, i), planDataArrGet(arr, i), 12), 0);
    }

    planDataArrDel(arr);
    planDataArrDel(arr2);
}
