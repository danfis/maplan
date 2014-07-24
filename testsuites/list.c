#include <cu/cu.h>
#include <plan/list.h>
#include <stdio.h>

struct _data3_t {
    plan_cost_t cost[3];
    plan_state_id_t state_id;
};
typedef struct _data3_t data3_t;

static data3_t data3[] = {
    { { 1, 1, 1 }, 0 },
    { { 1, 1, 1 }, 1 },
    { { 1, 1, 9 }, 2 },
    { { 1, 2, 1 }, 3 },
    { { 3, 1, 1 }, 3 },
    { { 1, 3, 1 }, 4 },
    { { 1, 1, 4 }, 5 },
    { { 1, 1, 0 }, 6 },
    { { 1, 4, 1 }, 7 },
    { { 1, 1, 1 }, 8 },
    { { 2, 2, 1 }, 9 },
    { { 1, 1, 1 }, 10 },
    { { 2, 1, 0 }, 11 },
    { { 2, 1, 1 }, 12 },
    { { 1, 1, 2 }, 13 },
    { { 1, 3, 1 }, 14 },
    { { 3, 1, 1 }, 15 },
    { { 1, 1, 7 }, 16 },
    { { 1, 4, 1 }, 17 },
    { { 3, 1, 3 }, 18 },
    { { 1, 3, 1 }, 19 },
};
static int data3_size = sizeof(data3) / sizeof(data3_t);

TEST(testListTieBreaking)
{
    plan_list_t *list;
    plan_cost_t cost[3];
    plan_state_id_t state_id;
    int i;

    list = planListTieBreaking(3);

    for (i = 0; i < data3_size; ++i)
        planListPush(list, data3[i].cost, data3[i].state_id);
    for (i = 0; planListPop(list, &state_id, cost) == 0; ++i);
    assertEquals(i, data3_size);

    for (i = 0; i < data3_size; ++i)
        planListPush(list, data3[i].cost, data3[i].state_id);
    for (i = 0; planListPop(list, &state_id, cost) == 0; ++i){
        printf("%d %d %d -> %d\n",
               (int)cost[0], (int)cost[1], (int)cost[2],
               (int)state_id);
    }

    planListDel(list);
}
