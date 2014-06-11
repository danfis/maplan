#include <cu/cu.h>
#include <plan/list_lazy_heap.h>
#include <plan/list_lazy_bucket.h>

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
    //planListLazyHeapPush(l, 7, 8, NULL);

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

    /*
    assertEquals(planListLazyHeapPop(l, &sid, &op), 0);
    assertEquals(sid, 8);
    assertEquals(op, NULL);
    */

    assertEquals(planListLazyHeapPop(l, &sid, &op), 0);
    assertEquals(sid, 3);
    assertEquals(op, (plan_operator_t *)0x3);

    assertEquals(planListLazyHeapPop(l, &sid, &op), -1);
    assertEquals(planListLazyHeapPop(l, &sid, &op), -1);

    planListLazyHeapDel(l);
}

TEST(testListLazyBucket)
{
    plan_list_lazy_bucket_t *l;
    plan_state_id_t sid;
    plan_operator_t *op;

    l = planListLazyBucketNew();
    planListLazyBucketPush(l, 1, 1, NULL);
    planListLazyBucketPush(l, 3, 2, (void *)0x1);
    planListLazyBucketPush(l, 10, 3, (void *)0x3);
    planListLazyBucketPush(l, 4, 4, (void *)0x2);
    planListLazyBucketPush(l, 7, 5, NULL);
    planListLazyBucketPush(l, 0, 6, NULL);
    planListLazyBucketDel(l);

    l = planListLazyBucketNew();
    planListLazyBucketPush(l, 1, 1, NULL);
    planListLazyBucketPush(l, 3, 2, (void *)0x1);
    planListLazyBucketPush(l, 10, 3, (void *)0x3);
    planListLazyBucketPush(l, 4, 4, (void *)0x2);
    planListLazyBucketPush(l, 7, 5, NULL);
    planListLazyBucketPush(l, 0, 6, NULL);
    planListLazyBucketPush(l, 7, 8, NULL);

    assertEquals(planListLazyBucketPop(l, &sid, &op), 0);
    assertEquals(sid, 6);
    assertEquals(op, NULL);

    assertEquals(planListLazyBucketPop(l, &sid, &op), 0);
    assertEquals(sid, 1);
    assertEquals(op, NULL);

    assertEquals(planListLazyBucketPop(l, &sid, &op), 0);
    assertEquals(sid, 2);
    assertEquals(op, (plan_operator_t *)0x1);

    assertEquals(planListLazyBucketPop(l, &sid, &op), 0);
    assertEquals(sid, 4);
    assertEquals(op, (plan_operator_t *)0x2);

    assertEquals(planListLazyBucketPop(l, &sid, &op), 0);
    assertEquals(sid, 5);
    assertEquals(op, NULL);

    assertEquals(planListLazyBucketPop(l, &sid, &op), 0);
    assertEquals(sid, 8);
    assertEquals(op, NULL);

    assertEquals(planListLazyBucketPop(l, &sid, &op), 0);
    assertEquals(sid, 3);
    assertEquals(op, (plan_operator_t *)0x3);

    assertEquals(planListLazyBucketPop(l, &sid, &op), -1);
    assertEquals(planListLazyBucketPop(l, &sid, &op), -1);

    planListLazyBucketDel(l);
}
