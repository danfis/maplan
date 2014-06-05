#include <cu/cu.h>
#include "plan/statespace_fifo.h"
#include "plan/statespace_heap.h"
#include "plan/statespace_bucket.h"

void testStateSpace(plan_state_space_t *(*new_fn)(plan_state_pool_t *),
                    void (*del_fn)(plan_state_space_t *),
                    int n0, int n1, int n2)
{
    plan_var_t vars[4];
    plan_state_pool_t *pool;
    plan_state_space_t *sspace;
    plan_state_t *state;
    plan_state_space_node_t *node;
    plan_state_space_node_t *nodeins[3];

    planVarInit(vars + 0);
    planVarInit(vars + 1);
    planVarInit(vars + 2);
    planVarInit(vars + 3);

    vars[0].range = 2;
    vars[1].range = 3;
    vars[2].range = 4;
    vars[3].range = 5;

    pool = planStatePoolNew(vars, 4);
    sspace = new_fn(pool);
    state = planStateNew(pool);

    // insert first state
    planStateZeroize(state);
    assertEquals(planStatePoolInsert(pool, state), 0);

    // check that open list is empty
    assertEquals(planStateSpacePop(sspace), NULL);

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

    node = planStateSpacePop(sspace);
    assertNotEquals(node, NULL);
    assertEquals(node, nodeins[n0]);
    assertEquals(node->state_id, nodeins[n0]->state_id);
    assertEquals(node->parent_state_id, nodeins[n0]->parent_state_id);
    assertEquals(node->op, nodeins[n0]->op);
    assertEquals(node->cost, nodeins[n0]->cost);
    assertEquals(node->heuristic, nodeins[n0]->heuristic);
    assertTrue(planStateSpaceNodeIsClosed(node));

    node = planStateSpacePop(sspace);
    assertEquals(node, nodeins[n1]);
    assertEquals(node->state_id, nodeins[n1]->state_id);
    assertEquals(node->parent_state_id, nodeins[n1]->parent_state_id);
    assertEquals(node->op, nodeins[n1]->op);
    assertEquals(node->cost, nodeins[n1]->cost);
    assertEquals(node->heuristic, nodeins[n1]->heuristic);
    assertTrue(planStateSpaceNodeIsClosed(node));

    node = planStateSpacePop(sspace);
    assertEquals(node, nodeins[n2]);
    assertEquals(node->state_id, nodeins[n2]->state_id);
    assertEquals(node->parent_state_id, nodeins[n2]->parent_state_id);
    assertEquals(node->op, nodeins[n2]->op);
    assertEquals(node->cost, nodeins[n2]->cost);
    assertEquals(node->heuristic, nodeins[n2]->heuristic);
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
    node->cost = 0;
    node->heuristic = 1;
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
    node->cost = 2;
    node->heuristic = 0;
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
    del_fn(sspace);
    planStatePoolDel(pool);
}

TEST(testStateSpaceHeapCost)
{
    testStateSpace(planStateSpaceHeapCostNew,
                   planStateSpaceHeapDel, 0, 1, 2);
}

TEST(testStateSpaceHeapHeuristic)
{
    testStateSpace(planStateSpaceHeapHeuristicNew,
                   planStateSpaceHeapDel, 2, 1, 0);
}

TEST(testStateSpaceHeapCostHeuristic)
{
    testStateSpace(planStateSpaceHeapCostHeuristicNew,
                   planStateSpaceHeapDel, 1, 0, 2);
}

TEST(testStateSpaceFifo)
{
    testStateSpace(planStateSpaceFifoNew,
                   planStateSpaceFifoDel, 0, 1, 2);
}

TEST(testStateSpaceBucketCost)
{
    testStateSpace(planStateSpaceBucketCostNew,
                   planStateSpaceBucketDel, 0, 1, 2);
}

TEST(testStateSpaceBucketHeuristic)
{
    testStateSpace(planStateSpaceBucketHeuristicNew,
                   planStateSpaceBucketDel, 2, 1, 0);
}

TEST(testStateSpaceBucketCostHeuristic)
{
    testStateSpace(planStateSpaceBucketCostHeuristicNew,
                   planStateSpaceBucketDel, 1, 0, 2);
}
