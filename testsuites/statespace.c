#include <cu/cu.h>
#include "plan/statespace.h"

TEST(testStateSpaceBasic)
{
    plan_var_t vars[4];
    plan_state_pool_t *pool;
    plan_state_space_t *sspace;
    plan_state_t *state;
    plan_state_space_node_t *node;

    planVarInit(vars + 0);
    planVarInit(vars + 1);
    planVarInit(vars + 2);
    planVarInit(vars + 3);

    vars[0].range = 2;
    vars[1].range = 3;
    vars[2].range = 4;
    vars[3].range = 5;

    pool = planStatePoolNew(vars, 4);
    sspace = planStateSpaceNew(pool);
    state = planStateNew(pool);

    // insert first state
    planStateZeroize(state);
    assertEquals(planStatePoolInsert(pool, state), 0);

    // check that open list is empty
    assertEquals(planStateSpaceExtractMin(sspace), NULL);

    // open the first node and check its values
    node = planStateSpaceOpenNode(sspace, 0, 1);
    assertEquals(planStateSpaceNodeHeuristic(node), 1);
    assertTrue(planStateSpaceNodeIsOpen(node));
    assertEquals(planStateSpaceNodeStateId(node), 0);

    // open second state node
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 2);
    planStateSet(state, 2, 3);
    planStateSet(state, 3, 4);
    assertEquals(planStatePoolInsert(pool, state), 1);
    node = planStateSpaceOpenNode(sspace, 1, 10);
    assertEquals(planStateSpaceNodeHeuristic(node), 10);
    assertTrue(planStateSpaceNodeIsOpen(node));
    assertEquals(planStateSpaceNodeStateId(node), 1);

    // open third state node
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 2);
    assertEquals(planStatePoolInsert(pool, state), 2);
    node = planStateSpaceOpenNode(sspace, 2, 5);
    assertEquals(planStateSpaceNodeHeuristic(node), 5);
    assertTrue(planStateSpaceNodeIsOpen(node));
    assertEquals(planStateSpaceNodeStateId(node), 2);

    // try to extract one node
    node = planStateSpaceExtractMin(sspace);
    assertNotEquals(node, NULL);
    assertEquals(planStateSpaceNodeHeuristic(node), 1);
    assertTrue(planStateSpaceNodeIsClosed(node));
    assertEquals(planStateSpaceNodeStateId(node), 0);

    // check closing closed node
    assertEquals(planStateSpaceCloseNode(sspace, 0), NULL);

    // try to reopen the node
    assertEquals(planStateSpaceReopenNode(sspace, 0, 11), node);
    assertEquals(planStateSpaceNodeHeuristic(node), 11);
    assertTrue(planStateSpaceNodeIsOpen(node));
    assertEquals(planStateSpaceNodeStateId(node), 0);

    // close other node
    assertNotEquals(planStateSpaceCloseNode(sspace, 1), NULL);

    // extract the rest and check values
    node = planStateSpaceExtractMin(sspace);
    assertNotEquals(node, NULL);
    assertEquals(planStateSpaceNodeHeuristic(node), 5);
    assertTrue(planStateSpaceNodeIsClosed(node));
    assertEquals(planStateSpaceNodeStateId(node), 2);

    node = planStateSpaceExtractMin(sspace);
    assertNotEquals(node, NULL);
    assertEquals(planStateSpaceNodeHeuristic(node), 11);
    assertTrue(planStateSpaceNodeIsClosed(node));
    assertEquals(planStateSpaceNodeStateId(node), 0);

    node = planStateSpaceExtractMin(sspace);
    assertEquals(node, NULL);

    planStateDel(pool, state);
    planStateSpaceDel(sspace);
    planStatePoolDel(pool);
}
