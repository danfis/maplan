#include <cu/cu.h>
#include <plan/ma_private_state.h>

TEST(testMAPrivateState)
{
    plan_ma_private_state_t aps;
    int state[5] = { 0, 0, 0, 0, 0 };

    planMAPrivateStateInit(&aps, 5, 3);
    assertEquals(planMAPrivateStateInsert(&aps, state), 0);
    assertEquals(planMAPrivateStateInsert(&aps, state), 0);
    state[3] = 12;
    assertEquals(planMAPrivateStateInsert(&aps, state), 0);
    state[1] = 12;
    assertEquals(planMAPrivateStateInsert(&aps, state), 1);
    state[3] = -12;
    assertEquals(planMAPrivateStateInsert(&aps, state), 1);
    state[1] = 0;
    assertEquals(planMAPrivateStateInsert(&aps, state), 0);
    state[0] = 1;
    state[1] = 2;
    state[2] = 3;
    state[3] = 4;
    state[4] = 5;
    assertEquals(planMAPrivateStateInsert(&aps, state), 2);

    planMAPrivateStateGet(&aps, 0, state);
    assertEquals(state[0], 0);
    assertEquals(state[1], 0);
    assertEquals(state[2], 0);
    assertEquals(state[3], -1);
    assertEquals(state[4], 0);
    planMAPrivateStateGet(&aps, 1, state);
    assertEquals(state[0], 0);
    assertEquals(state[1], 12);
    assertEquals(state[2], 0);
    assertEquals(state[3], -1);
    assertEquals(state[4], 0);
    planMAPrivateStateGet(&aps, 2, state);
    assertEquals(state[0], 1);
    assertEquals(state[1], 2);
    assertEquals(state[2], 3);
    assertEquals(state[3], -1);
    assertEquals(state[4], 5);

    planMAPrivateStateFree(&aps);
}
