#include <cu/cu.h>
#include "plan/statespace_fifo.h"
#include "plan/statespace_heap.h"

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
    sspace = planStateSpaceHeapHeuristicNew(pool);
    state = planStateNew(pool);

    // insert first state
    planStateZeroize(state);
    assertEquals(planStatePoolInsert(pool, state), 0);

    // check that open list is empty
    assertEquals(planStateSpacePop(sspace), NULL);

    // open the first node and check its values
    node = planStateSpaceNode(sspace, 0);
    node->parent_state_id = PLAN_NO_STATE;
    node->op = NULL;
    node->cost = 1;
    node->heuristic = 10;
    assertEquals(planStateSpaceOpen(sspace, node), 0);
    assertTrue(planStateSpaceNodeIsOpen(node));

    // open second state node
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 2);
    planStateSet(state, 2, 3);
    planStateSet(state, 3, 4);
    assertEquals(planStatePoolInsert(pool, state), 1);
    node = planStateSpaceNode(sspace, 1);
    node->parent_state_id = 0;
    node->op = NULL;
    node->cost = 10;
    node->heuristic = 12;
    assertEquals(planStateSpaceOpen(sspace, node), 0);
    assertTrue(planStateSpaceNodeIsOpen(node));

    // open third state node
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 2);
    assertEquals(planStatePoolInsert(pool, state), 2);
    node = planStateSpaceOpen2(sspace, 2, 1, NULL, 5, 6);
    assertNotEquals(node, NULL);
    assertTrue(planStateSpaceNodeIsOpen(node));

    node = planStateSpacePop(sspace);
    assertNotEquals(node, NULL);
    assertEquals(node->state_id, 2);
    assertEquals(node->parent_state_id, 1);
    assertEquals(node->op, NULL);
    assertEquals(node->cost, 5);
    assertEquals(node->heuristic, 6);
    assertTrue(planStateSpaceNodeIsClosed(node));

    node = planStateSpacePop(sspace);
    assertNotEquals(node, NULL);
    assertEquals(node->state_id, 0);
    assertEquals(node->parent_state_id, PLAN_NO_STATE);
    assertEquals(node->op, NULL);
    assertEquals(node->cost, 1);
    assertEquals(node->heuristic, 10);
    assertTrue(planStateSpaceNodeIsClosed(node));

    node = planStateSpacePop(sspace);
    assertNotEquals(node, NULL);
    assertEquals(node->state_id, 1);
    assertEquals(node->parent_state_id, 0);
    assertEquals(node->op, NULL);
    assertEquals(node->cost, 10);
    assertEquals(node->heuristic, 12);
    assertTrue(planStateSpaceNodeIsClosed(node));

    node = planStateSpacePop(sspace);
    assertEquals(node, NULL);



    planStateSet(state, 0, 0);
    planStateSet(state, 1, 2);
    planStateSet(state, 2, 3);
    planStateSet(state, 3, 4);
    assertEquals(planStatePoolInsert(pool, state), 3);
    node = planStateSpaceNode(sspace, 3);
    assertTrue(planStateSpaceNodeIsNew(node));
    node->parent_state_id = 0;
    node->op = NULL;
    assertEquals(planStateSpaceOpen(sspace, node), 0);
    assertTrue(planStateSpaceNodeIsOpen(node));

    // open third state node
    planStateSet(state, 0, 0);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 2);
    assertEquals(planStatePoolInsert(pool, state), 4);
    node = planStateSpaceNode(sspace, 4);
    assertTrue(planStateSpaceNodeIsNew(node));
    node->parent_state_id = 1;
    node->op = NULL;
    assertEquals(planStateSpaceOpen(sspace, node), 0);
    assertTrue(planStateSpaceNodeIsOpen(node));

    planStateSpaceClear(sspace);
    assertEquals(planStateSpacePop(sspace), NULL);
    node = planStateSpaceNode(sspace, 3);
    assertTrue(planStateSpaceNodeIsNew(node));
    node = planStateSpaceNode(sspace, 4);
    assertTrue(planStateSpaceNodeIsNew(node));


    node = planStateSpaceNode(sspace, 3);
    assertEquals(planStateSpaceOpen(sspace, node), 0);
    assertTrue(planStateSpaceNodeIsOpen(node));

    node = planStateSpaceNode(sspace, 4);
    assertEquals(planStateSpaceOpen(sspace, node), 0);
    assertTrue(planStateSpaceNodeIsOpen(node));

    planStateSpaceCloseAll(sspace);
    assertEquals(planStateSpacePop(sspace), NULL);
    node = planStateSpaceNode(sspace, 3);
    assertTrue(planStateSpaceNodeIsClosed(node));
    node = planStateSpaceNode(sspace, 4);
    assertTrue(planStateSpaceNodeIsClosed(node));

    planStateDel(pool, state);
    planStateSpaceHeapDel(sspace);
    planStatePoolDel(pool);
}

TEST(testStateSpaceFifo)
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
    sspace = planStateSpaceFifoNew(pool);
    state = planStateNew(pool);

    // insert first state
    planStateZeroize(state);
    assertEquals(planStatePoolInsert(pool, state), 0);

    // check that open list is empty
    assertEquals(planStateSpacePop(sspace), NULL);

    // open the first node and check its values
    node = planStateSpaceNode(sspace, 0);
    node->parent_state_id = PLAN_NO_STATE;
    node->op = NULL;
    assertEquals(planStateSpaceOpen(sspace, node), 0);
    assertTrue(planStateSpaceNodeIsOpen(node));

    // open second state node
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 2);
    planStateSet(state, 2, 3);
    planStateSet(state, 3, 4);
    assertEquals(planStatePoolInsert(pool, state), 1);
    node = planStateSpaceNode(sspace, 1);
    node->parent_state_id = 0;
    node->op = NULL;
    node->cost = 10;
    node->heuristic = 12;
    assertEquals(planStateSpaceOpen(sspace, node), 0);
    assertTrue(planStateSpaceNodeIsOpen(node));

    // open third state node
    planStateSet(state, 0, 1);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 2);
    assertEquals(planStatePoolInsert(pool, state), 2);
    node = planStateSpaceOpen2(sspace, 2, 1, NULL, 5, 6);
    assertNotEquals(node, NULL);
    assertTrue(planStateSpaceNodeIsOpen(node));

    node = planStateSpacePop(sspace);
    assertNotEquals(node, NULL);
    assertEquals(node->state_id, 0);
    assertEquals(node->parent_state_id, PLAN_NO_STATE);
    assertEquals(node->op, NULL);
    assertTrue(planStateSpaceNodeIsClosed(node));

    node = planStateSpacePop(sspace);
    assertNotEquals(node, NULL);
    assertEquals(node->state_id, 1);
    assertEquals(node->parent_state_id, 0);
    assertEquals(node->op, NULL);
    assertEquals(node->cost, 10);
    assertEquals(node->heuristic, 12);
    assertTrue(planStateSpaceNodeIsClosed(node));

    node = planStateSpacePop(sspace);
    assertNotEquals(node, NULL);
    assertEquals(node->state_id, 2);
    assertEquals(node->parent_state_id, 1);
    assertEquals(node->op, NULL);
    assertEquals(node->cost, 5);
    assertEquals(node->heuristic, 6);
    assertTrue(planStateSpaceNodeIsClosed(node));

    node = planStateSpacePop(sspace);
    assertEquals(node, NULL);



    planStateSet(state, 0, 0);
    planStateSet(state, 1, 2);
    planStateSet(state, 2, 3);
    planStateSet(state, 3, 4);
    assertEquals(planStatePoolInsert(pool, state), 3);
    node = planStateSpaceNode(sspace, 3);
    assertTrue(planStateSpaceNodeIsNew(node));
    node->parent_state_id = 0;
    node->op = NULL;
    assertEquals(planStateSpaceOpen(sspace, node), 0);
    assertTrue(planStateSpaceNodeIsOpen(node));

    // open third state node
    planStateSet(state, 0, 0);
    planStateSet(state, 1, 1);
    planStateSet(state, 2, 2);
    planStateSet(state, 3, 2);
    assertEquals(planStatePoolInsert(pool, state), 4);
    node = planStateSpaceNode(sspace, 4);
    assertTrue(planStateSpaceNodeIsNew(node));
    node->parent_state_id = 1;
    node->op = NULL;
    assertEquals(planStateSpaceOpen(sspace, node), 0);
    assertTrue(planStateSpaceNodeIsOpen(node));

    planStateSpaceClear(sspace);
    assertEquals(planStateSpacePop(sspace), NULL);
    node = planStateSpaceNode(sspace, 3);
    assertTrue(planStateSpaceNodeIsNew(node));
    node = planStateSpaceNode(sspace, 4);
    assertTrue(planStateSpaceNodeIsNew(node));


    node = planStateSpaceNode(sspace, 3);
    assertEquals(planStateSpaceOpen(sspace, node), 0);
    assertTrue(planStateSpaceNodeIsOpen(node));

    node = planStateSpaceNode(sspace, 4);
    assertEquals(planStateSpaceOpen(sspace, node), 0);
    assertTrue(planStateSpaceNodeIsOpen(node));

    planStateSpaceCloseAll(sspace);
    assertEquals(planStateSpacePop(sspace), NULL);
    node = planStateSpaceNode(sspace, 3);
    assertTrue(planStateSpaceNodeIsClosed(node));
    node = planStateSpaceNode(sspace, 4);
    assertTrue(planStateSpaceNodeIsClosed(node));

    planStateDel(pool, state);
    planStateSpaceFifoDel(sspace);
    planStatePoolDel(pool);
}
