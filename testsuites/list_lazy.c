#include <cu/cu.h>
#include <plan/list_lazy.h>

TEST(testListLazyHeap)
{
    plan_list_lazy_t *l;
    plan_state_id_t sid;
    plan_op_t *op;

    l = planListLazyHeapNew();
    planListLazyPush(l, 1, 1, NULL);
    planListLazyPush(l, 3, 2, (void *)0x1);
    planListLazyPush(l, 10, 3, (void *)0x3);
    planListLazyPush(l, 4, 4, (void *)0x2);
    planListLazyPush(l, 7, 5, NULL);
    planListLazyPush(l, 0, 6, NULL);
    planListLazyDel(l);

    l = planListLazyHeapNew();
    planListLazyPush(l, 1, 1, NULL);
    planListLazyPush(l, 3, 2, (void *)0x1);
    planListLazyPush(l, 10, 3, (void *)0x3);
    planListLazyPush(l, 4, 4, (void *)0x2);
    planListLazyPush(l, 7, 5, NULL);
    planListLazyPush(l, 0, 6, NULL);
    //planListLazyPush(l, 7, 8, NULL);

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 6);
    assertEquals(op, NULL);

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 1);
    assertEquals(op, NULL);

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 2);
    assertEquals(op, (plan_op_t *)0x1);

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 4);
    assertEquals(op, (plan_op_t *)0x2);

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 5);
    assertEquals(op, NULL);

    /*
    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 8);
    assertEquals(op, NULL);
    */

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 3);
    assertEquals(op, (plan_op_t *)0x3);

    assertEquals(planListLazyPop(l, &sid, &op), -1);
    assertEquals(planListLazyPop(l, &sid, &op), -1);

    planListLazyDel(l);
}

TEST(testListLazyBucket)
{
    plan_list_lazy_t *l;
    plan_state_id_t sid;
    plan_op_t *op;

    l = planListLazyBucketNew();
    planListLazyPush(l, 1, 1, NULL);
    planListLazyPush(l, 3, 2, (void *)0x1);
    planListLazyPush(l, 10, 3, (void *)0x3);
    planListLazyPush(l, 4, 4, (void *)0x2);
    planListLazyPush(l, 7, 5, NULL);
    planListLazyPush(l, 0, 6, NULL);
    planListLazyDel(l);

    l = planListLazyBucketNew();
    planListLazyPush(l, 1, 1, NULL);
    planListLazyPush(l, 3, 2, (void *)0x1);
    planListLazyPush(l, 10, 3, (void *)0x3);
    planListLazyPush(l, 4, 4, (void *)0x2);
    planListLazyPush(l, 7, 5, NULL);
    planListLazyPush(l, 0, 6, NULL);
    planListLazyPush(l, 7, 8, NULL);

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 6);
    assertEquals(op, NULL);

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 1);
    assertEquals(op, NULL);

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 2);
    assertEquals(op, (plan_op_t *)0x1);

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 4);
    assertEquals(op, (plan_op_t *)0x2);

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 5);
    assertEquals(op, NULL);

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 8);
    assertEquals(op, NULL);

    assertEquals(planListLazyPop(l, &sid, &op), 0);
    assertEquals(sid, 3);
    assertEquals(op, (plan_op_t *)0x3);

    assertEquals(planListLazyPop(l, &sid, &op), -1);
    assertEquals(planListLazyPop(l, &sid, &op), -1);

    planListLazyDel(l);
}
