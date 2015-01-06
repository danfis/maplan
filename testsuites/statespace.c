#include <cu/cu.h>
#include "plan/statespace.h"

TEST(testStateSpace)
{
    plan_var_t vars[4];
    plan_state_pool_t *pool;
    plan_state_space_t *sspace;
    plan_state_t *state;
    plan_state_space_node_t *nodeins[3];

    planVarInit(vars + 0, "a", 2);
    planVarInit(vars + 1, "b", 3);
    planVarInit(vars + 2, "c", 4);
    planVarInit(vars + 3, "d", 5);

    pool = planStatePoolNew(vars, 4);
    sspace = planStateSpaceNew(pool);
    state = planStateNew(pool);

    // insert first state
    planStateZeroize(state);
    assertEquals(planStatePoolInsert(pool, state), 0);

    // open the first node and check its values
    nodeins[0] = planStateSpaceNode(sspace, 0);
    nodeins[0]->parent_state_id = PLAN_NO_STATE;
    nodeins[0]->op = NULL;
    nodeins[0]->cost = 1;
    nodeins[0]->heuristic = 10;
    assertEquals(planStateSpaceOpen(sspace, nodeins[0]), 0);
    assertTrue(planStateSpaceNodeIsOpen(nodeins[0]));

    // open second state node
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 2);
    planStateSet(state, 2, 3);
    planStateSet(state, 3, 4);
    assertEquals(planStatePoolInsert(pool, state), 1);
    nodeins[1] = planStateSpaceNode(sspace, 1);
    nodeins[1]->parent_state_id = 0;
    nodeins[1]->op = NULL;
    nodeins[1]->cost = 2;
    nodeins[1]->heuristic = 8;
    assertEquals(planStateSpaceOpen(sspace, nodeins[1]), 0);
    assertTrue(planStateSpaceNodeIsOpen(nodeins[1]));

    // open third state node
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 2);
    assertEquals(planStatePoolInsert(pool, state), 2);
    nodeins[2] = planStateSpaceOpen2(sspace, 2, 1, NULL, 5, 7);
    assertNotEquals(nodeins[2], NULL);
    assertTrue(planStateSpaceNodeIsOpen(nodeins[2]));

    assertEquals(planStateSpaceClose(sspace, nodeins[0]), 0);
    assertEquals(nodeins[0]->parent_state_id, PLAN_NO_STATE);
    assertEquals(nodeins[0]->op, NULL);
    assertEquals(nodeins[0]->cost, 1);
    assertEquals(nodeins[0]->heuristic, 10);
    assertTrue(planStateSpaceNodeIsClosed(nodeins[0]));

    assertNotEquals(planStateSpaceClose(sspace, nodeins[0]), 0);
    assertEquals(planStateSpaceReopen(sspace, nodeins[0]), 0);
    assertEquals(planStateSpaceClose(sspace, nodeins[0]), 0);
    assertNotEquals(planStateSpaceClose(sspace, nodeins[0]), 0);

    planStateDel(state);
    planStateSpaceDel(sspace);
    planStatePoolDel(pool);
}
