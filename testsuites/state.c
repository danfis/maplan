#include <cu/cu.h>
#include "plan/state.h"


TEST(testStateBasic)
{
    plan_var_t vars[4];
    plan_state_pool_t *pool;
    plan_state_t *state;
    const plan_state_t *s2;

    planVarInit(vars + 0);
    planVarInit(vars + 1);
    planVarInit(vars + 2);
    planVarInit(vars + 3);

    vars[0].range = 6;
    vars[1].range = 2;
    vars[2].range = 3;
    vars[3].range = 7;


    pool = planStatePoolNew(vars, 4);
    state = planStatePoolCreateState(pool);

    planStateSet(state, 0, 1);
    planStateSet(state, 1, 1);
    assertEquals(planStatePoolInsert(pool, state), 0);
    assertEquals(planStatePoolInsert(pool, state), 0);

    planStateZeroize(pool, state);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 5);
    assertEquals(planStatePoolInsert(pool, state), 1);
    assertEquals(planStatePoolInsert(pool, state), 1);

    planStateZeroize(pool, state);
    planStateSet(state, 0, 4);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 0);
    planStateSet(state, 3, 1);
    assertEquals(planStatePoolInsert(pool, state), 2);
    assertEquals(planStatePoolInsert(pool, state), 2);

    planStateZeroize(pool, state);
    planStateSet(state, 0, 5);
    planStateSet(state, 1, 0);
    planStateSet(state, 2, 1);
    planStateSet(state, 3, 2);
    assertEquals(planStatePoolInsert(pool, state), 3);
    assertEquals(planStatePoolInsert(pool, state), 3);

    planStateZeroize(pool, state);
    planStateSet(state, 0, 5);
    planStateSet(state, 1, 0);
    planStateSet(state, 2, 1);
    planStateSet(state, 3, 2);
    assertEquals(planStatePoolFind(pool, state), 3);
    planStateSet(state, 3, 0);
    assertEquals(planStatePoolFind(pool, state), -1);

    planStateZeroize(pool, state);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 5);
    assertEquals(planStatePoolFind(pool, state), 1);

    planStateZeroize(pool, state);
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 1);
    assertEquals(planStatePoolFind(pool, state), 0);

    planStateZeroize(pool, state);
    planStateSet(state, 0, 4);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 0);
    planStateSet(state, 3, 1);
    assertEquals(planStatePoolFind(pool, state), 2);


    planStateZeroize(pool, state);
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 1);
    s2 = planStatePoolGet(pool, 0);
    assertTrue(planStateEq(pool, s2, state));

    planStateZeroize(pool, state);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 5);
    assertFalse(planStateEq(pool, s2, state));
    s2 = planStatePoolGet(pool, 1);
    assertTrue(planStateEq(pool, s2, state));

    planStateZeroize(pool, state);
    planStateSet(state, 0, 4);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 0);
    planStateSet(state, 3, 1);
    assertFalse(planStateEq(pool, s2, state));
    s2 = planStatePoolGet(pool, 2);
    assertTrue(planStateEq(pool, s2, state));

    planStateZeroize(pool, state);
    planStateSet(state, 0, 5);
    planStateSet(state, 1, 0);
    planStateSet(state, 2, 1);
    planStateSet(state, 3, 2);
    assertFalse(planStateEq(pool, s2, state));
    s2 = planStatePoolGet(pool, 3);
    assertTrue(planStateEq(pool, s2, state));

    planStatePoolDestroyState(pool, state);
    planStatePoolDel(pool);

    planVarFree(vars + 0);
    planVarFree(vars + 1);
    planVarFree(vars + 2);
    planVarFree(vars + 3);
}
