#include <cu/cu.h>
#include "plan/problem.h"

static void pVar(const plan_var_t *var, int var_size)
{
    int i;

    printf("Vars[%d]:\n", var_size);
    for (i = 0; i < var_size; ++i){
        printf("[%d] name: `%s', range: %d\n", i, var[i].name, var[i].range);
    }
}

static void pInitState(const plan_state_pool_t *state_pool, plan_state_id_t sid)
{
    plan_state_t *state;
    int i, size;

    state = planStateNew(state_pool);
    planStatePoolGetState(state_pool, sid, state);
    size = planStateSize(state);
    printf("Initial state:");
    for (i = 0; i < size; ++i){
        printf(" %d:%d", i, (int)planStateGet(state, i));
    }
    printf("\n");
    planStateDel(state);
}

static void pPartState(const plan_part_state_t *p)
{
    plan_var_id_t var;
    plan_val_t val;
    int i;

    PLAN_PART_STATE_FOR_EACH(p, i, var, val){
        printf(" %d:%d", (int)var, (int)val);
    }
}

static void pGoal(const plan_part_state_t *p)
{
    printf("Goal:");
    pPartState(p);
    printf("\n");
}

static void pOp(const plan_op_t *op, int op_size)
{
    int i, j;

    printf("Ops[%d]:\n", op_size);
    for (i = 0; i < op_size; ++i){
        printf("[%d] name: `%s', cost: %d\n", i, op[i].name, (int)op[i].cost);
        printf("[%d] pre:", i);
        pPartState(op[i].pre);
        printf(", eff:");
        pPartState(op[i].eff);
        printf("\n");

        for (j = 0; j < op[i].cond_eff_size; ++j){
            printf("[%d] cond_eff[%d]:", i, j);
            printf(" pre:");
            pPartState(op[i].cond_eff[j].pre);
            printf(", eff:");
            pPartState(op[i].cond_eff[j].eff);
            printf("\n");
        }
    }
}

TEST(testLoadFromProto)
{
    plan_problem_t *p;

    p = planProblemFromProto("../data/ma-benchmarks/rovers/p03.proto",
                             PLAN_PROBLEM_USE_CG);
    printf("---- testLoadFromProto ----\n");
    pVar(p->var, p->var_size);
    pInitState(p->state_pool, p->initial_state);
    pGoal(p->goal);
    pOp(p->op, p->op_size);
    printf("Succ Gen: %d\n", (int)(p->succ_gen != NULL));
    printf("---- testLoadFromProto END ----\n");
    planProblemDel(p);
}

TEST(testLoadFromProtoCondEff)
{
    plan_problem_t *p;

    p = planProblemFromProto("../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto",
                             PLAN_PROBLEM_USE_CG);
    printf("---- testLoadFromProtoCondEff ----\n");
    pVar(p->var, p->var_size);
    pInitState(p->state_pool, p->initial_state);
    pGoal(p->goal);
    pOp(p->op, p->op_size);
    printf("Succ Gen: %d\n", (int)(p->succ_gen != NULL));
    printf("---- testLoadFromProtoCondEff END ----\n");
    planProblemDel(p);
}

TEST(testLoadAgentFromProto)
{
    plan_problem_agents_t *agents;
    plan_problem_t *agent;
    plan_problem_t *p;
    plan_state_t *initial_state;
    int i, api;
    const char *n;

    agents = planProblemAgentsFromProto("../data/ma-benchmarks/rovers/p03.proto",
                                        PLAN_PROBLEM_USE_CG);
    assertNotEquals(agents, NULL);
    if (agents == NULL)
        return;

    assertEquals(agents->agent[0].private_val_size, 7);
    assertEquals(agents->agent[0].private_val[0].var, 0);
    assertEquals(agents->agent[0].private_val[0].val, 0);
    assertEquals(agents->agent[0].private_val[1].var, 0);
    assertEquals(agents->agent[0].private_val[1].val, 1);
    assertEquals(agents->agent[0].private_val[2].var, 0);
    assertEquals(agents->agent[0].private_val[2].val, 2);
    assertEquals(agents->agent[0].private_val[3].var, 2);
    assertEquals(agents->agent[0].private_val[3].val, 1);
    assertEquals(agents->agent[0].private_val[4].var, 3);
    assertEquals(agents->agent[0].private_val[4].val, 1);
    assertEquals(agents->agent[0].private_val[5].var, 10);
    assertEquals(agents->agent[0].private_val[5].val, 0);
    assertEquals(agents->agent[0].private_val[6].var, 10);
    assertEquals(agents->agent[0].private_val[6].val, 1);

    assertEquals(agents->agent[1].private_val_size, 15);
    assertEquals(agents->agent[1].private_val[0].var, 1);
    assertEquals(agents->agent[1].private_val[0].val, 0);
    assertEquals(agents->agent[1].private_val[1].var, 1);
    assertEquals(agents->agent[1].private_val[1].val, 1);
    assertEquals(agents->agent[1].private_val[2].var, 1);
    assertEquals(agents->agent[1].private_val[2].val, 2);
    assertEquals(agents->agent[1].private_val[3].var, 1);
    assertEquals(agents->agent[1].private_val[3].val, 3);
    assertEquals(agents->agent[1].private_val[4].var, 2);
    assertEquals(agents->agent[1].private_val[4].val, 2);
    assertEquals(agents->agent[1].private_val[5].var, 3);
    assertEquals(agents->agent[1].private_val[5].val, 2);
    assertEquals(agents->agent[1].private_val[6].var, 4);
    assertEquals(agents->agent[1].private_val[6].val, 0);
    assertEquals(agents->agent[1].private_val[7].var, 4);
    assertEquals(agents->agent[1].private_val[7].val, 1);
    assertEquals(agents->agent[1].private_val[8].var, 5);
    assertEquals(agents->agent[1].private_val[8].val, 0);
    assertEquals(agents->agent[1].private_val[9].var, 5);
    assertEquals(agents->agent[1].private_val[9].val, 1);
    assertEquals(agents->agent[1].private_val[10].var, 6);
    assertEquals(agents->agent[1].private_val[10].val, 0);
    assertEquals(agents->agent[1].private_val[11].var, 6);
    assertEquals(agents->agent[1].private_val[11].val, 1);
    assertEquals(agents->agent[1].private_val[12].var, 11);
    assertEquals(agents->agent[1].private_val[12].val, 0);
    assertEquals(agents->agent[1].private_val[13].var, 11);
    assertEquals(agents->agent[1].private_val[13].val, 1);
    assertEquals(agents->agent[1].private_val[14].var, 12);
    assertEquals(agents->agent[1].private_val[14].val, 0);

    for (api = 0; api < agents->agent_size; ++api){
        p = agents->agent + api;

        assertEquals(p->var_size, 13);
        assertEquals(p->var[0].range, 3);
        assertEquals(p->var[1].range, 4);
        assertEquals(p->var[2].range, 3);
        assertEquals(p->var[3].range, 3);
        assertEquals(p->var[4].range, 2);
        assertEquals(p->var[5].range, 2);
        assertEquals(p->var[6].range, 2);
        assertEquals(p->var[7].range, 2);
        assertEquals(p->var[8].range, 2);
        assertEquals(p->var[9].range, 2);
        assertEquals(p->var[10].range, 2);
        assertEquals(p->var[11].range, 2);
        assertEquals(p->var[12].range, 2);

        initial_state = planStateNew(p->state_pool);
        planStatePoolGetState(p->state_pool, 0, initial_state);

        assertEquals(planStateGet(initial_state, 0), 1);
        assertEquals(planStateGet(initial_state, 1), 3);
        assertEquals(planStateGet(initial_state, 2), 0);
        assertEquals(planStateGet(initial_state, 3), 0);
        assertEquals(planStateGet(initial_state, 4), 0);
        assertEquals(planStateGet(initial_state, 5), 0);
        assertEquals(planStateGet(initial_state, 6), 1);
        assertEquals(planStateGet(initial_state, 7), 1);
        assertEquals(planStateGet(initial_state, 8), 1);
        assertEquals(planStateGet(initial_state, 9), 1);
        assertEquals(planStateGet(initial_state, 10), 0);
        assertEquals(planStateGet(initial_state, 11), 0);
        assertEquals(planStateGet(initial_state, 12), 1);

        planStateDel(initial_state);

        assertEquals(planPartStateGet(p->goal, 0), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 1), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 2), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 3), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 4), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 5), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 6), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 7), 0);
        assertEquals(planPartStateGet(p->goal, 8), 0);
        assertEquals(planPartStateGet(p->goal, 9), 0);
        assertEquals(planPartStateGet(p->goal, 10), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 11), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 12), PLAN_VAL_UNDEFINED);

        assertFalse(planPartStateIsSet(p->goal, 0));
        assertFalse(planPartStateIsSet(p->goal, 1));
        assertFalse(planPartStateIsSet(p->goal, 2));
        assertFalse(planPartStateIsSet(p->goal, 3));
        assertFalse(planPartStateIsSet(p->goal, 4));
        assertFalse(planPartStateIsSet(p->goal, 5));
        assertFalse(planPartStateIsSet(p->goal, 6));
        assertTrue(planPartStateIsSet(p->goal, 7));
        assertTrue(planPartStateIsSet(p->goal, 8));
        assertTrue(planPartStateIsSet(p->goal, 9));
        assertFalse(planPartStateIsSet(p->goal, 10));
        assertFalse(planPartStateIsSet(p->goal, 11));
        assertFalse(planPartStateIsSet(p->goal, 12));
    }

    assertEquals(agents->agent_size, 2);

    agent = agents->agent + 0;
    assertEquals(strcmp(agent->agent_name, "rover0"), 0);

    assertTrue(!planOpExtraMAOpIsPrivate(agent->op + 0));
    assertTrue(!planOpExtraMAOpIsPrivate(agent->op + 1));
    assertTrue(planOpExtraMAOpIsPrivate(agent->op + 2));
    assertTrue(planOpExtraMAOpIsPrivate(agent->op + 3));
    assertTrue(planOpExtraMAOpIsPrivate(agent->op + 4));
    assertTrue(planOpExtraMAOpIsPrivate(agent->op + 5));
    assertTrue(planOpExtraMAOpIsPrivate(agent->op + 6));
    assertTrue(!planOpExtraMAOpIsPrivate(agent->op + 7));
    assertTrue(!planOpExtraMAOpIsPrivate(agent->op + 8));

    agent = agents->agent + 1;
    assertEquals(strcmp(agent->agent_name, "rover1"), 0);

    assertTrue(planOpExtraMAOpIsPrivate(agent->op + 0));
    assertTrue(planOpExtraMAOpIsPrivate(agent->op + 0));
    assertTrue(planOpExtraMAOpIsPrivate(agent->op + 1));
    for (i = 2; i <= 10; ++i)
        assertTrue(!planOpExtraMAOpIsPrivate(agent->op + i));
    for (i = 11; i <= 17; ++i)
        assertTrue(planOpExtraMAOpIsPrivate(agent->op + i));
    assertTrue(!planOpExtraMAOpIsPrivate(agent->op + 18));
    assertTrue(!planOpExtraMAOpIsPrivate(agent->op + 19));
    for (i = 20; i <= 33; ++i)
        assertTrue(planOpExtraMAOpIsPrivate(agent->op + i));

    for (i = 0; i < agent->proj_op_size; ++i){
        n = agents->glob.op[planOpExtraMAProjOpGlobalId(agent->proj_op + i)].name;
        assertEquals(strcmp(agent->proj_op[i].name, n), 0);
    }

    planProblemAgentsDel(agents);
}
