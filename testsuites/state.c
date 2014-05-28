#include <cu/cu.h>
#include "plan/state.h"


TEST(testStateBasic)
{
    plan_var_t vars[4];
    plan_state_pool_t *pool;
    plan_state_t *state;
    plan_state_id_t ins[4];

    planVarInit(vars + 0);
    planVarInit(vars + 1);
    planVarInit(vars + 2);
    planVarInit(vars + 3);

    vars[0].range = 6;
    vars[1].range = 2;
    vars[2].range = 3;
    vars[3].range = 7;


    pool = planStatePoolNew(vars, 4);
    state = planStateNew(pool);

    planStateSet(state, 0, 1);
    planStateSet(state, 1, 1);
    ins[0] = planStatePoolInsert(pool, state);
    assertEquals(planStatePoolInsert(pool, state), ins[0]);

    planStateZeroize(state);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 5);
    ins[1] = planStatePoolInsert(pool, state);
    assertEquals(planStatePoolInsert(pool, state), ins[1]);

    planStateZeroize(state);
    planStateSet(state, 0, 4);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 0);
    planStateSet(state, 3, 1);
    ins[2] = planStatePoolInsert(pool, state);
    assertEquals(planStatePoolInsert(pool, state), ins[2]);

    planStateZeroize(state);
    planStateSet(state, 0, 5);
    planStateSet(state, 1, 0);
    planStateSet(state, 2, 1);
    planStateSet(state, 3, 2);
    ins[3] = planStatePoolInsert(pool, state);
    assertEquals(planStatePoolInsert(pool, state), ins[3]);



    planStateZeroize(state);
    planStateSet(state, 0, 5);
    planStateSet(state, 1, 0);
    planStateSet(state, 2, 1);
    planStateSet(state, 3, 2);
    assertEquals(planStatePoolFind(pool, state), ins[3]);
    planStateSet(state, 3, 0);
    assertEquals(planStatePoolFind(pool, state), PLAN_NO_STATE);

    planStateZeroize(state);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 5);
    assertEquals(planStatePoolFind(pool, state), ins[1]);

    planStateZeroize(state);
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 1);
    assertEquals(planStatePoolFind(pool, state), ins[0]);

    planStateZeroize(state);
    planStateSet(state, 0, 4);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 0);
    planStateSet(state, 3, 1);
    assertEquals(planStatePoolFind(pool, state), ins[2]);


    planStateDel(pool, state);
    planStatePoolDel(pool);

    planVarFree(vars + 0);
    planVarFree(vars + 1);
    planVarFree(vars + 2);
    planVarFree(vars + 3);
}
