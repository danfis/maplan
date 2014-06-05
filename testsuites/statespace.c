#include <cu/cu.h>
#include "plan/statespace.h"
#include "plan/statespace_fifo.h"

TEST(testStateSpaceBasic)
{
    /*
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
    node = planStateSpaceOpenNode(sspace, 0, PLAN_NO_STATE, NULL, 1);
    assertEquals(planStateSpaceNodeHeuristic(node), 1);
    assertTrue(planStateSpaceNodeIsOpen(node));
    assertEquals(planStateSpaceNodeStateId(node), 0);
    assertEquals(planStateSpaceNodeParentStateId(node), PLAN_NO_STATE);
    assertEquals(planStateSpaceNodeOpName(node), NULL);

    // open second state node
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 2);
    planStateSet(state, 2, 3);
    planStateSet(state, 3, 4);
    assertEquals(planStatePoolInsert(pool, state), 1);
    node = planStateSpaceOpenNode(sspace, 1, 0, NULL, 10);
    assertEquals(planStateSpaceNodeHeuristic(node), 10);
    assertTrue(planStateSpaceNodeIsOpen(node));
    assertEquals(planStateSpaceNodeStateId(node), 1);
    assertEquals(planStateSpaceNodeParentStateId(node), 0);

    // open third state node
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 2);
    assertEquals(planStatePoolInsert(pool, state), 2);
    node = planStateSpaceOpenNode(sspace, 2, 1, NULL, 5);
    assertEquals(planStateSpaceNodeHeuristic(node), 5);
    assertTrue(planStateSpaceNodeIsOpen(node));
    assertEquals(planStateSpaceNodeStateId(node), 2);
    assertEquals(planStateSpaceNodeParentStateId(node), 1);

    // try to extract one node
    node = planStateSpaceExtractMin(sspace);
    assertNotEquals(node, NULL);
    assertEquals(planStateSpaceNodeHeuristic(node), 1);
    assertTrue(planStateSpaceNodeIsClosed(node));
    assertEquals(planStateSpaceNodeStateId(node), 0);
    assertEquals(planStateSpaceNodeParentStateId(node), PLAN_NO_STATE);

    // check closing closed node
    assertEquals(planStateSpaceCloseNode(sspace, 0), NULL);

    // try to reopen the node
    assertEquals(planStateSpaceReopenNode(sspace, 0, PLAN_NO_STATE, NULL, 11), node);
    assertEquals(planStateSpaceNodeHeuristic(node), 11);
    assertTrue(planStateSpaceNodeIsOpen(node));
    assertEquals(planStateSpaceNodeStateId(node), 0);
    assertEquals(planStateSpaceNodeParentStateId(node), PLAN_NO_STATE);

    // close other node
    assertNotEquals(planStateSpaceCloseNode(sspace, 1), NULL);

    // extract the rest and check values
    node = planStateSpaceExtractMin(sspace);
    assertNotEquals(node, NULL);
    assertEquals(planStateSpaceNodeHeuristic(node), 5);
    assertTrue(planStateSpaceNodeIsClosed(node));
    assertEquals(planStateSpaceNodeStateId(node), 2);
    assertEquals(planStateSpaceNodeParentStateId(node), 1);

    node = planStateSpaceExtractMin(sspace);
    assertNotEquals(node, NULL);
    assertEquals(planStateSpaceNodeHeuristic(node), 11);
    assertTrue(planStateSpaceNodeIsClosed(node));
    assertEquals(planStateSpaceNodeStateId(node), 0);
    assertEquals(planStateSpaceNodeParentStateId(node), PLAN_NO_STATE);

    node = planStateSpaceExtractMin(sspace);
    assertEquals(node, NULL);

    planStateDel(pool, state);
    planStateSpaceDel(sspace);
    planStatePoolDel(pool);
    */
}

TEST(testStateSpaceFifo)
{
    plan_var_t vars[4];
    plan_state_pool_t *pool;
    plan_state_space_fifo_t *sspace;
    plan_state_t *state;
    plan_state_space_fifo_node_t *node;

    planVarInit(vars + 0);
    planVarInit(vars + 1);
    planVarInit(vars + 2);
    planVarInit(vars + 3);

    vars[0].range = 2;
    vars[1].range = 3;
    vars[2].range = 4;
    vars[3].range = 5;

    pool = planStatePoolNew(vars, 4);
    sspace = planStateSpaceFifoNew(pool);
    state = planStateNew(pool);

    // insert first state
    planStateZeroize(state);
    assertEquals(planStatePoolInsert(pool, state), 0);

    // check that open list is empty
    assertEquals(planStateSpaceFifoPop(sspace), NULL);

    // open the first node and check its values
    node = planStateSpaceFifoNode(sspace, 0);
    node->node.parent_state_id = PLAN_NO_STATE;
    node->node.op = NULL;
    assertEquals(planStateSpaceFifoOpen(sspace, node), 0);
    assertTrue(planStateSpaceFifoNodeIsOpen(node));

    // open second state node
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 2);
    planStateSet(state, 2, 3);
    planStateSet(state, 3, 4);
    assertEquals(planStatePoolInsert(pool, state), 1);
    node = planStateSpaceFifoNode(sspace, 1);
    node->node.parent_state_id = 0;
    node->node.op = NULL;
    assertEquals(planStateSpaceFifoOpen(sspace, node), 0);
    assertTrue(planStateSpaceFifoNodeIsOpen(node));

    // open third state node
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 2);
    assertEquals(planStatePoolInsert(pool, state), 2);
    node = planStateSpaceFifoNode(sspace, 2);
    node->node.parent_state_id = 1;
    node->node.op = NULL;
    assertEquals(planStateSpaceFifoOpen(sspace, node), 0);
    assertTrue(planStateSpaceFifoNodeIsOpen(node));

    node = planStateSpaceFifoPop(sspace);
    assertNotEquals(node, NULL);
    assertEquals(node->node.state_id, 0);
    assertEquals(node->node.parent_state_id, PLAN_NO_STATE);
    assertEquals(node->node.op, NULL);
    assertTrue(planStateSpaceFifoNodeIsClosed(node));

    node = planStateSpaceFifoPop(sspace);
    assertNotEquals(node, NULL);
    assertEquals(node->node.state_id, 1);
    assertEquals(node->node.parent_state_id, 0);
    assertEquals(node->node.op, NULL);
    assertTrue(planStateSpaceFifoNodeIsClosed(node));

    node = planStateSpaceFifoPop(sspace);
    assertNotEquals(node, NULL);
    assertEquals(node->node.state_id, 2);
    assertEquals(node->node.parent_state_id, 1);
    assertEquals(node->node.op, NULL);
    assertTrue(planStateSpaceFifoNodeIsClosed(node));

    node = planStateSpaceFifoPop(sspace);
    assertEquals(node, NULL);



    planStateSet(state, 0, 0);
    planStateSet(state, 1, 2);
    planStateSet(state, 2, 3);
    planStateSet(state, 3, 4);
    assertEquals(planStatePoolInsert(pool, state), 3);
    node = planStateSpaceFifoNode(sspace, 3);
    assertTrue(planStateSpaceFifoNodeIsNew(node));
    node->node.parent_state_id = 0;
    node->node.op = NULL;
    assertEquals(planStateSpaceFifoOpen(sspace, node), 0);
    assertTrue(planStateSpaceFifoNodeIsOpen(node));

    // open third state node
    planStateSet(state, 0, 0);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 2);
    assertEquals(planStatePoolInsert(pool, state), 4);
    node = planStateSpaceFifoNode(sspace, 4);
    assertTrue(planStateSpaceFifoNodeIsNew(node));
    node->node.parent_state_id = 1;
    node->node.op = NULL;
    assertEquals(planStateSpaceFifoOpen(sspace, node), 0);
    assertTrue(planStateSpaceFifoNodeIsOpen(node));

    planStateSpaceFifoClear(sspace);
    assertEquals(planStateSpaceFifoPop(sspace), NULL);
    node = planStateSpaceFifoNode(sspace, 3);
    assertTrue(planStateSpaceFifoNodeIsNew(node));
    node = planStateSpaceFifoNode(sspace, 4);
    assertTrue(planStateSpaceFifoNodeIsNew(node));


    node = planStateSpaceFifoNode(sspace, 3);
    assertEquals(planStateSpaceFifoOpen(sspace, node), 0);
    assertTrue(planStateSpaceFifoNodeIsOpen(node));

    node = planStateSpaceFifoNode(sspace, 4);
    assertEquals(planStateSpaceFifoOpen(sspace, node), 0);
    assertTrue(planStateSpaceFifoNodeIsOpen(node));

    planStateSpaceFifoCloseAll(sspace);
    assertEquals(planStateSpaceFifoPop(sspace), NULL);
    node = planStateSpaceFifoNode(sspace, 3);
    assertTrue(planStateSpaceFifoNodeIsClosed(node));
    node = planStateSpaceFifoNode(sspace, 4);
    assertTrue(planStateSpaceFifoNodeIsClosed(node));

    planStateDel(pool, state);
    planStateSpaceFifoDel(sspace);
    planStatePoolDel(pool);
}
