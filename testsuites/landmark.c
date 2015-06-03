#include <stdio.h>
#include <cu/cu.h>
#include <plan/landmark.h>

static int ldm0[] = { 0, 1, 3, 4 };
static int ldm1[] = { 0, 0, 1, 1, 2 };
static int ldm2[] = { 12 };
static int ldm3[] = { 3, 5, 2 };
static int ldm4[] = { 8, 1, 1, 2, 4 };
static int ldm5[] = { 8, 1, 2, 4 };

static void printLandmarkSet(const plan_landmark_set_t *l,
                             const char *name)
{
    int i, j;

    if (name != NULL)
        printf("%s\n", name);

    for (i = 0; i < l->size; ++i){
        printf("[%d]", i);
        for (j = 0; j < l->landmark[i].size; ++j){
            printf(" %d", l->landmark[i].op_id[j]);
        }
        printf("\n");
    }
}

TEST(testLandmarkCache)
{
    plan_landmark_cache_t *ldmc;
    plan_landmark_set_t ldms;
    const plan_landmark_set_t *l;
    plan_landmark_set_id_t ids[10];

    ldmc = planLandmarkCacheNew();

    planLandmarkSetInit(&ldms);
    planLandmarkSetAdd(&ldms, 4, ldm0);
    planLandmarkSetAdd(&ldms, 5, ldm1);
    planLandmarkSetAdd(&ldms, 1, ldm2);
    ids[0] = planLandmarkCacheAdd(ldmc, &ldms);

    planLandmarkSetInit(&ldms);
    planLandmarkSetAdd(&ldms, 3, ldm3);
    planLandmarkSetAdd(&ldms, 5, ldm4);
    ids[1] = planLandmarkCacheAdd(ldmc, &ldms);

    planLandmarkSetInit(&ldms);
    planLandmarkSetAdd(&ldms, 4, ldm0);
    planLandmarkSetAdd(&ldms, 4, ldm5);
    ids[2] = planLandmarkCacheAdd(ldmc, &ldms);

    planLandmarkSetInit(&ldms);
    planLandmarkSetAdd(&ldms, 5, ldm1);
    planLandmarkSetAdd(&ldms, 4, ldm5);
    planLandmarkSetAdd(&ldms, 5, ldm4);
    ids[3] = planLandmarkCacheAdd(ldmc, &ldms);

    l = planLandmarkCacheGet(ldmc, ids[0]);
    assertEquals(l->size, 3);
    printLandmarkSet(l, "LS 0");

    l = planLandmarkCacheGet(ldmc, ids[1]);
    assertEquals(l->size, 2);
    printLandmarkSet(l, "LS 1");

    l = planLandmarkCacheGet(ldmc, ids[2]);
    assertEquals(l->size, 2);
    printLandmarkSet(l, "LS 2");

    l = planLandmarkCacheGet(ldmc, ids[3]);
    assertEquals(l->size, 3);
    printLandmarkSet(l, "LS 3");

    planLandmarkCacheDel(ldmc);
}
