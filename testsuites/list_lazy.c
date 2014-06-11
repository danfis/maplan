#include <cu/cu.h>
#include <plan/list_lazy_heap.h>

TEST(testListLazyHeap)
{
    plan_list_lazy_heap_t *l;
    plan_state_id_t sid;
    plan_operator_t *op;

    l = planListLazyHeapNew();
    planListLazyHeapPush(l, 1, 1, NULL);
    planListLazyHeapPush(l, 3, 2, (void *)0x1);
    planListLazyHeapPush(l, 10, 3, (void *)0x3);
    planListLazyHeapPush(l, 4, 4, (void *)0x2);
    planListLazyHeapPush(l, 7, 5, NULL);
    planListLazyHeapPush(l, 0, 6, NULL);
    planListLazyHeapDel(l);

    l = planListLazyHeapNew();
    planListLazyHeapPush(l, 1, 1, NULL);
    planListLazyHeapPush(l, 3, 2, (void *)0x1);
    planListLazyHeapPush(l, 10, 3, (void *)0x3);
    planListLazyHeapPush(l, 4, 4, (void *)0x2);
    planListLazyHeapPush(l, 7, 5, NULL);
    planListLazyHeapPush(l, 0, 6, NULL);

    assertEquals(planListLazyHeapPop(l, &sid, &op), 0);
    assertEquals(sid, 6);
    assertEquals(op, NULL);

    assertEquals(planListLazyHeapPop(l, &sid, &op), 0);
    assertEquals(sid, 1);
    assertEquals(op, NULL);

    assertEquals(planListLazyHeapPop(l, &sid, &op), 0);
    assertEquals(sid, 2);
    assertEquals(op, (plan_operator_t *)0x1);

    assertEquals(planListLazyHeapPop(l, &sid, &op), 0);
    assertEquals(sid, 4);
    assertEquals(op, (plan_operator_t *)0x2);

    assertEquals(planListLazyHeapPop(l, &sid, &op), 0);
    assertEquals(sid, 5);
    assertEquals(op, NULL);

    assertEquals(planListLazyHeapPop(l, &sid, &op), 0);
    assertEquals(sid, 3);
    assertEquals(op, (plan_operator_t *)0x3);

    assertEquals(planListLazyHeapPop(l, &sid, &op), -1);
    assertEquals(planListLazyHeapPop(l, &sid, &op), -1);

    planListLazyHeapDel(l);
}
