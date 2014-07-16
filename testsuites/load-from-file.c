#include <cu/cu.h>
#include "plan/problem.h"


TEST(testLoadFromFD)
{
    plan_problem_t *p;
    plan_state_t *initial_state;

    p = planProblemFromFD("load-from-file.in1.sas");
    assertNotEquals(p, NULL);
    if (p == NULL)
        return;


    assertEquals(p->var_size, 9);
    assertEquals(p->var[0].range, 5);
    assertEquals(p->var[1].range, 2);
    assertEquals(p->var[2].range, 2);
    assertEquals(p->var[3].range, 2);
    assertEquals(p->var[4].range, 2);
    assertEquals(p->var[5].range, 2);
    assertEquals(p->var[6].range, 5);
    assertEquals(p->var[7].range, 5);
    assertEquals(p->var[8].range, 5);

    initial_state = planStateNew(p->state_pool);
    planStatePoolGetState(p->state_pool, 0, initial_state);

    assertEquals(planStateGet(initial_state, 0), 2);
    assertEquals(planStateGet(initial_state, 1), 0);
    assertEquals(planStateGet(initial_state, 2), 1);
    assertEquals(planStateGet(initial_state, 3), 1);
    assertEquals(planStateGet(initial_state, 4), 1);
    assertEquals(planStateGet(initial_state, 5), 0);
    assertEquals(planStateGet(initial_state, 6), 3);
    assertEquals(planStateGet(initial_state, 7), 1);
    assertEquals(planStateGet(initial_state, 8), 4);

    planStateDel(initial_state);

    assertEquals(planPartStateGet(p->goal, 0), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 1), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 2), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 3), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 4), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 5), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 6), 1);
    assertEquals(planPartStateGet(p->goal, 7), 1);
    assertEquals(planPartStateGet(p->goal, 8), 3);

    assertFalse(planPartStateIsSet(p->goal, 0));
    assertFalse(planPartStateIsSet(p->goal, 1));
    assertFalse(planPartStateIsSet(p->goal, 2));
    assertFalse(planPartStateIsSet(p->goal, 3));
    assertFalse(planPartStateIsSet(p->goal, 4));
    assertFalse(planPartStateIsSet(p->goal, 5));
    assertTrue(planPartStateIsSet(p->goal, 6));
    assertTrue(planPartStateIsSet(p->goal, 7));
    assertTrue(planPartStateIsSet(p->goal, 8));

    assertEquals(p->op_size, 32);
    assertEquals(p->op[0].cost, 1);
    assertEquals(strcmp(p->op[0].name, "pick-up a"), 0);
    assertEquals(planPartStateGet(p->op[0].eff, 0), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->op[0].eff, 2), 1);
    assertEquals(planPartStateGet(p->op[0].eff, 5), 1);
    assertEquals(planPartStateGet(p->op[0].eff, 6), 0);
    assertEquals(planPartStateGet(p->op[0].pre, 0), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->op[0].pre, 2), 0);
    assertEquals(planPartStateGet(p->op[0].pre, 5), 0);
    assertEquals(planPartStateGet(p->op[0].pre, 6), 4);
    assertEquals(planPartStateGet(p->op[5].eff, 0), 4);
    assertTrue(planPartStateIsSet(p->op[5].eff, 0));
    assertEquals(planPartStateGet(p->op[5].eff, 1), 0);
    assertTrue(planPartStateIsSet(p->op[5].eff, 1));
    assertEquals(planPartStateGet(p->op[5].eff, 5), 0);
    assertTrue(planPartStateIsSet(p->op[5].eff, 5));
    assertEquals(planPartStateGet(p->op[5].eff, 6), PLAN_VAL_UNDEFINED);
    assertFalse(planPartStateIsSet(p->op[5].eff, 6));
    assertEquals(planPartStateGet(p->op[5].pre, 0), 0);
    assertTrue(planPartStateIsSet(p->op[5].pre, 0));
    assertEquals(planPartStateGet(p->op[5].pre, 1), PLAN_VAL_UNDEFINED);
    assertFalse(planPartStateIsSet(p->op[5].pre, 1));
    assertEquals(planPartStateGet(p->op[5].pre, 5), PLAN_VAL_UNDEFINED);
    assertFalse(planPartStateIsSet(p->op[5].pre, 5));
    assertEquals(planPartStateGet(p->op[5].pre, 6), PLAN_VAL_UNDEFINED);
    assertFalse(planPartStateIsSet(p->op[5].pre, 6));

    planProblemDel(p);
}


TEST(testLoadFromFD2)
{
    plan_problem_t *p;
    plan_state_t *initial_state;

    p = planProblemFromFD("../data/ma-benchmarks/depot/pfile1.sas");
    assertNotEquals(p, NULL);
    if (p == NULL)
        return;


    assertEquals(p->var_size, 14);
    assertEquals(p->var[0].range, 3);
    assertEquals(p->var[1].range, 3);
    assertEquals(p->var[2].range, 2);
    assertEquals(p->var[3].range, 2);
    assertEquals(p->var[4].range, 2);
    assertEquals(p->var[5].range, 2);
    assertEquals(p->var[6].range, 2);
    assertEquals(p->var[7].range, 2);
    assertEquals(p->var[8].range, 4);
    assertEquals(p->var[9].range, 4);
    assertEquals(p->var[10].range, 2);
    assertEquals(p->var[11].range, 2);
    assertEquals(p->var[12].range, 9);
    assertEquals(p->var[13].range, 9);

    initial_state = planStateNew(p->state_pool);
    planStatePoolGetState(p->state_pool, 0, initial_state);

    assertEquals(planStateGet(initial_state, 0), 0);
    assertEquals(planStateGet(initial_state, 1), 2);
    assertEquals(planStateGet(initial_state, 2), 1);
    assertEquals(planStateGet(initial_state, 3), 1);
    assertEquals(planStateGet(initial_state, 4), 0);
    assertEquals(planStateGet(initial_state, 5), 0);
    assertEquals(planStateGet(initial_state, 6), 0);
    assertEquals(planStateGet(initial_state, 7), 0);
    assertEquals(planStateGet(initial_state, 8), 1);
    assertEquals(planStateGet(initial_state, 9), 0);
    assertEquals(planStateGet(initial_state, 10), 0);
    assertEquals(planStateGet(initial_state, 11), 0);
    assertEquals(planStateGet(initial_state, 12), 7);
    assertEquals(planStateGet(initial_state, 13), 6);

    planStateDel(initial_state);

    assertEquals(planPartStateGet(p->goal, 0), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 1), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 2), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 3), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 4), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 5), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 6), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 7), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 8), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 9), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 10), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 11), PLAN_VAL_UNDEFINED);
    assertEquals(planPartStateGet(p->goal, 12), 8);
    assertEquals(planPartStateGet(p->goal, 13), 7);

    assertFalse(planPartStateIsSet(p->goal, 0));
    assertFalse(planPartStateIsSet(p->goal, 1));
    assertFalse(planPartStateIsSet(p->goal, 2));
    assertFalse(planPartStateIsSet(p->goal, 3));
    assertFalse(planPartStateIsSet(p->goal, 4));
    assertFalse(planPartStateIsSet(p->goal, 5));
    assertFalse(planPartStateIsSet(p->goal, 6));
    assertFalse(planPartStateIsSet(p->goal, 7));
    assertFalse(planPartStateIsSet(p->goal, 8));
    assertFalse(planPartStateIsSet(p->goal, 9));
    assertFalse(planPartStateIsSet(p->goal, 10));
    assertFalse(planPartStateIsSet(p->goal, 11));
    assertTrue(planPartStateIsSet(p->goal, 12));
    assertTrue(planPartStateIsSet(p->goal, 13));

    assertEquals(p->op_size, 72);
    assertEquals(p->op[0].cost, 1);
    assertEquals(strcmp(p->op[0].name, "drive truck0 depot0 distributor0"), 0);
    assertEquals(planPartStateGet(p->op[0].eff, 1), 1);
    assertEquals(planPartStateGet(p->op[0].pre, 1), 0);
    assertTrue(planPartStateIsSet(p->op[0].eff, 1));
    assertTrue(planPartStateIsSet(p->op[0].pre, 1));

    assertEquals(planPartStateGet(p->op[12].pre, 9), 0);
    assertEquals(planPartStateGet(p->op[12].pre, 11), 0);
    assertEquals(planPartStateGet(p->op[12].pre, 12), 2);
    assertEquals(planPartStateGet(p->op[12].eff, 8), 0);
    assertEquals(planPartStateGet(p->op[12].eff, 5), 0);
    assertEquals(planPartStateGet(p->op[12].eff, 10), 0);
    assertEquals(planPartStateGet(p->op[12].eff, 11), 1);
    assertEquals(planPartStateGet(p->op[12].eff, 12), 5);

    assertEquals(planPartStateGet(p->op[16].pre, 9), 1);
    assertEquals(planPartStateGet(p->op[16].pre, 11), 0);
    assertEquals(planPartStateGet(p->op[16].pre, 12), 3);
    assertEquals(planPartStateGet(p->op[16].eff, 8), 1);
    assertEquals(planPartStateGet(p->op[16].eff, 6), 0);
    assertEquals(planPartStateGet(p->op[16].eff, 10), 0);
    assertEquals(planPartStateGet(p->op[16].eff, 11), 1);
    assertEquals(planPartStateGet(p->op[16].eff, 12), 5);

    planProblemDel(p);
}

TEST(testLoadAgentFromFD)
{
    plan_problem_agents_t *agents;
    plan_problem_agent_t *agent;
    plan_problem_t *p;
    plan_state_t *initial_state;
    int i, api;

    agents = planProblemAgentsFromFD("../data/ma-benchmarks/depot/pfile1.sas");
    assertEquals(agents, NULL);
    agents = planProblemAgentsFromFD("../data/ma-benchmarks/rovers/p03.asas");
    assertNotEquals(agents, NULL);
    if (agents == NULL)
        return;

    for (api = 0; api < agents->agent_size; ++api){
        p = &agents->agent[api].prob;

        assertEquals(p->var_size, 13);
        assertEquals(p->var[0].range, 4);
        assertEquals(p->var[1].range, 2);
        assertEquals(p->var[2].range, 2);
        assertEquals(p->var[3].range, 2);
        assertEquals(p->var[4].range, 3);
        assertEquals(p->var[5].range, 2);
        assertEquals(p->var[6].range, 2);
        assertEquals(p->var[7].range, 3);
        assertEquals(p->var[8].range, 2);
        assertEquals(p->var[9].range, 2);
        assertEquals(p->var[10].range, 3);
        assertEquals(p->var[11].range, 2);
        assertEquals(p->var[12].range, 2);

        initial_state = planStateNew(p->state_pool);
        planStatePoolGetState(p->state_pool, 0, initial_state);

        assertEquals(planStateGet(initial_state, 0), 3);
        assertEquals(planStateGet(initial_state, 1), 1);
        assertEquals(planStateGet(initial_state, 2), 1);
        assertEquals(planStateGet(initial_state, 3), 1);
        assertEquals(planStateGet(initial_state, 4), 1);
        assertEquals(planStateGet(initial_state, 5), 0);
        assertEquals(planStateGet(initial_state, 6), 0);
        assertEquals(planStateGet(initial_state, 7), 0);
        assertEquals(planStateGet(initial_state, 8), 0);
        assertEquals(planStateGet(initial_state, 9), 0);
        assertEquals(planStateGet(initial_state, 10), 0);
        assertEquals(planStateGet(initial_state, 11), 1);
        assertEquals(planStateGet(initial_state, 12), 1);

        planStateDel(initial_state);

        assertEquals(planPartStateGet(p->goal, 0), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 1), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 2), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 3), 0);
        assertEquals(planPartStateGet(p->goal, 4), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 5), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 6), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 7), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 8), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 9), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 10), PLAN_VAL_UNDEFINED);
        assertEquals(planPartStateGet(p->goal, 11), 0);
        assertEquals(planPartStateGet(p->goal, 12), 0);

        assertFalse(planPartStateIsSet(p->goal, 0));
        assertFalse(planPartStateIsSet(p->goal, 1));
        assertFalse(planPartStateIsSet(p->goal, 2));
        assertTrue(planPartStateIsSet(p->goal, 3));
        assertFalse(planPartStateIsSet(p->goal, 4));
        assertFalse(planPartStateIsSet(p->goal, 5));
        assertFalse(planPartStateIsSet(p->goal, 6));
        assertFalse(planPartStateIsSet(p->goal, 7));
        assertFalse(planPartStateIsSet(p->goal, 8));
        assertFalse(planPartStateIsSet(p->goal, 9));
        assertFalse(planPartStateIsSet(p->goal, 10));
        assertTrue(planPartStateIsSet(p->goal, 11));
        assertTrue(planPartStateIsSet(p->goal, 12));
    }

    assertEquals(agents->agent_size, 2);

    agent = agents->agent + 0;
    assertEquals(strcmp(agent->name, "rover0"), 0);
    assertEquals(agent->id, 0);

    for (i = 0; i < 5; ++i){
        assertTrue(planOperatorIsPrivate(agent->prob.op + i));
    }
    for (i = 5; i < agent->prob.op_size; ++i){
        assertFalse(planOperatorIsPrivate(agent->prob.op + i));
    }

    agent = agents->agent + 1;
    assertEquals(strcmp(agent->name, "rover1"), 0);
    assertEquals(agent->id, 1);
    assertTrue(planOperatorIsPrivate(agent->prob.op + 0));

    for (i = 0; i < 23; ++i){
        assertTrue(planOperatorIsPrivate(agent->prob.op + i));
    }
    for (i = 23; i < agent->prob.op_size; ++i){
        assertFalse(planOperatorIsPrivate(agent->prob.op + i));
    }

    planProblemAgentsDel(agents);
}

TEST(testLoadCondEffFromFD)
{
    plan_problem_t *prob;
    plan_state_t *state, *state_check;
    plan_state_id_t sid, sid_check;
    plan_operator_t opcpy;
    int i;

    prob = planProblemFromFD("cond-eff.sas");
    assertNotEquals(prob, NULL);
    if (prob == NULL)
        return;

    assertEquals(prob->op[0].cond_eff_size, 0);
    assertEquals(prob->op[589].cond_eff_size, 5);
    assertEquals(prob->op[589].cost, 10);
    assertEquals(prob->op[590].cond_eff_size, 5);
    assertEquals(prob->op[591].cond_eff_size, 5);

    state = planStateNew(prob->state_pool);
    state_check = planStateNew(prob->state_pool);

    for (i = 0; i < prob->var_size; ++i){
        planStateSet(state, i, 0);
    }
    planStateSet(state, 579, 1);
    planStateSet(state, 602, 1);
    planStateSet(state, 617, 1);
    planStateSet(state, 589, 1);
    planStateSet(state, 627, 1);

    sid = planStatePoolInsert(prob->state_pool, state);
    sid_check = planOperatorApply(prob->op + 589, sid);
    planStatePoolGetState(prob->state_pool, sid_check, state_check);
    for (i = 0; i < prob->var_size; ++i){
        if (i == 534
                || i == 24
                || i == 579
                || i == 602
                || i == 617
                || i == 589
                || i == 627){
            assertEquals(planStateGet(state_check, i), 1);
        }else{
            assertEquals(planStateGet(state_check, i), 0);
        }
    }


    planStateSet(state, 579, 0);

    sid = planStatePoolInsert(prob->state_pool, state);
    sid_check = planOperatorApply(prob->op + 589, sid);
    planStatePoolGetState(prob->state_pool, sid_check, state_check);
    for (i = 0; i < prob->var_size; ++i){
        if (i == 534
                || i == 24
                || i == 579
                || i == 602
                || i == 617
                || i == 589
                || i == 512
                || i == 627){
            assertEquals(planStateGet(state_check, i), 1);
        }else{
            assertEquals(planStateGet(state_check, i), 0);
        }
    }


    planStateSet(state, 602, 0);

    sid = planStatePoolInsert(prob->state_pool, state);
    sid_check = planOperatorApply(prob->op + 589, sid);
    planStatePoolGetState(prob->state_pool, sid_check, state_check);
    for (i = 0; i < prob->var_size; ++i){
        if (i == 534
                || i == 24
                || i == 579
                || i == 602
                || i == 617
                || i == 589
                || i == 512
                || i == 515
                || i == 627){
            assertEquals(planStateGet(state_check, i), 1);
        }else{
            assertEquals(planStateGet(state_check, i), 0);
        }
    }


    planStateSet(state, 617, 0);

    sid = planStatePoolInsert(prob->state_pool, state);
    sid_check = planOperatorApply(prob->op + 589, sid);
    planStatePoolGetState(prob->state_pool, sid_check, state_check);
    for (i = 0; i < prob->var_size; ++i){
        if (i == 534
                || i == 24
                || i == 579
                || i == 602
                || i == 617
                || i == 589
                || i == 512
                || i == 515
                || i == 519
                || i == 627){
            assertEquals(planStateGet(state_check, i), 1);
        }else{
            assertEquals(planStateGet(state_check, i), 0);
        }
    }


    for (i = 0; i < prob->var_size; ++i){
        planStateSet(state, i, 0);
    }
    planStateSet(state, 580, 1);
    planStateSet(state, 604, 1);
    planStateSet(state, 618, 1);
    planStateSet(state, 590, 1);
    planStateSet(state, 628, 1);

    sid = planStatePoolInsert(prob->state_pool, state);
    sid_check = planOperatorApply(prob->op + 590, sid);
    planStatePoolGetState(prob->state_pool, sid_check, state_check);
    for (i = 0; i < prob->var_size; ++i){
        if (i == 580
                || i == 604
                || i == 618
                || i == 590
                || i == 628
                || i == 533
                || i == 107){
            assertEquals(planStateGet(state_check, i), 1);
        }else{
            assertEquals(planStateGet(state_check, i), 0);
        }
    }

    planStateSet(state, 628, 0);
    sid = planStatePoolInsert(prob->state_pool, state);
    sid_check = planOperatorApply(prob->op + 590, sid);
    planStatePoolGetState(prob->state_pool, sid_check, state_check);
    for (i = 0; i < prob->var_size; ++i){
        if (i == 580
                || i == 604
                || i == 618
                || i == 590
                || i == 628
                || i == 533
                || i == 526
                || i == 107){
            assertEquals(planStateGet(state_check, i), 1);
        }else{
            assertEquals(planStateGet(state_check, i), 0);
        }
    }

    planStateSet(state, 604, 0);
    sid = planStatePoolInsert(prob->state_pool, state);
    sid_check = planOperatorApply(prob->op + 590, sid);
    planStatePoolGetState(prob->state_pool, sid_check, state_check);
    for (i = 0; i < prob->var_size; ++i){
        if (i == 580
                || i == 604
                || i == 618
                || i == 590
                || i == 628
                || i == 533
                || i == 526
                || i == 515
                || i == 527
                || i == 107){
            assertEquals(planStateGet(state_check, i), 1);
        }else{
            assertEquals(planStateGet(state_check, i), 0);
        }
    }


    planOperatorInit(&opcpy, prob->state_pool);
    planOperatorCopy(&opcpy, prob->op + 590);


    for (i = 0; i < prob->var_size; ++i){
        planStateSet(state, i, 0);
    }
    planStateSet(state, 580, 1);
    planStateSet(state, 604, 1);
    planStateSet(state, 618, 1);
    planStateSet(state, 590, 1);
    planStateSet(state, 628, 1);

    sid = planStatePoolInsert(prob->state_pool, state);
    sid_check = planOperatorApply(&opcpy, sid);
    planStatePoolGetState(prob->state_pool, sid_check, state_check);
    for (i = 0; i < prob->var_size; ++i){
        if (i == 580
                || i == 604
                || i == 618
                || i == 590
                || i == 628
                || i == 533
                || i == 107){
            assertEquals(planStateGet(state_check, i), 1);
        }else{
            assertEquals(planStateGet(state_check, i), 0);
        }
    }

    planStateSet(state, 628, 0);
    sid = planStatePoolInsert(prob->state_pool, state);
    sid_check = planOperatorApply(&opcpy, sid);
    planStatePoolGetState(prob->state_pool, sid_check, state_check);
    for (i = 0; i < prob->var_size; ++i){
        if (i == 580
                || i == 604
                || i == 618
                || i == 590
                || i == 628
                || i == 533
                || i == 526
                || i == 107){
            assertEquals(planStateGet(state_check, i), 1);
        }else{
            assertEquals(planStateGet(state_check, i), 0);
        }
    }

    planStateSet(state, 604, 0);
    sid = planStatePoolInsert(prob->state_pool, state);
    sid_check = planOperatorApply(&opcpy, sid);
    planStatePoolGetState(prob->state_pool, sid_check, state_check);
    for (i = 0; i < prob->var_size; ++i){
        if (i == 580
                || i == 604
                || i == 618
                || i == 590
                || i == 628
                || i == 533
                || i == 526
                || i == 515
                || i == 527
                || i == 107){
            assertEquals(planStateGet(state_check, i), 1);
        }else{
            assertEquals(planStateGet(state_check, i), 0);
        }
    }

    planOperatorFree(&opcpy);

    planStateDel(state_check);
    planStateDel(state);

    planProblemDel(prob);
}
