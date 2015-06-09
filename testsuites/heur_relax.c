#include <time.h>
#include <stdlib.h>
#include <cu/cu.h>
#include <boruvka/alloc.h>
#include "plan/problem.h"
#include "plan/heur.h"
#include "plan/search.h"
#include "state_pool.h"
#include "../src/heur_relax.h"

static void heurRelaxIdentity(const char *proto, const char *states,
                              int type)
{
    plan_problem_t *p;
    state_pool_t state_pool;
    plan_state_t *state;
    plan_heur_relax_t relax;
    int si;
    plan_cost_t h, h2;

    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->state_pool->num_vars);
    statePoolInit(&state_pool, states);

    planHeurRelaxInit(&relax, type, p->var, p->var_size, p->goal,
                      p->op, p->op_size, 0);

    for (si = 0; statePoolNext(&state_pool, state) == 0; ++si){
        planHeurRelax(&relax, state);
        h = relax.fact[relax.cref.goal_id].value;

        h2 = planHeurRelax2(&relax, state, p->goal);
        assertEquals(h, h2);
    }

    planHeurRelaxFree(&relax);
    statePoolFree(&state_pool);
    planStateDel(state);
    planProblemDel(p);
}

static plan_part_state_t *relaxStateToPartState(const plan_state_t *s,
                                                plan_state_pool_t *state_pool)
{
    plan_part_state_t *ps;
    int i, len;
    plan_val_t val;

    ps = planPartStateNew(state_pool->num_vars);
    len = planStateSize(s);
    for (i = 0; i < len; ++i){
        if (rand() < RAND_MAX / 2){
            val = planStateGet(s, i);
            planPartStateSet(ps, i, val);
        }
    }

    if (ps->vals_size == 0){
        val = planStateGet(s, 0);
        planPartStateSet(ps, 0, val);
    }

    return ps;
}

static void heurRelaxIdentity2(const char *proto, const char *states,
                               int type)
{
    plan_problem_t *p;
    state_pool_t state_pool;
    plan_state_t *state, *init_state;
    plan_part_state_t *goal;
    plan_heur_relax_t relax, relax2;
    int si;
    plan_cost_t h, h2;

    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->var_size);
    init_state = planStateNew(p->var_size);
    statePoolInit(&state_pool, states);

    statePoolNext(&state_pool, init_state);

    planHeurRelaxInit(&relax2, type, p->var, p->var_size, p->goal,
                      p->op, p->op_size, 0);

    for (si = 0; statePoolNext(&state_pool, state) == 0 && si < 2000; ++si){
        goal = relaxStateToPartState(state, p->state_pool);

        planHeurRelaxInit(&relax, type, p->var, p->var_size, goal,
                          p->op, p->op_size, 0);
        planHeurRelax(&relax, init_state);
        h = relax.fact[relax.cref.goal_id].value;
        planHeurRelaxFree(&relax);

        h2 = planHeurRelax2(&relax2, init_state, goal);
        assertEquals(h, h2);

        planPartStateDel(goal);
    }

    planHeurRelaxFree(&relax2);
    statePoolFree(&state_pool);
    planStateDel(state);
    planStateDel(init_state);
    planProblemDel(p);
}

TEST(testHeurRelax)
{
    heurRelaxIdentity("proto/depot-pfile1.proto",
                      "states/depot-pfile1.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity("proto/depot-pfile1.proto",
                      "states/depot-pfile1.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity("proto/depot-pfile5.proto",
                      "states/depot-pfile5.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity("proto/depot-pfile5.proto",
                      "states/depot-pfile5.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity("proto/rovers-p03.proto",
                      "states/rovers-p03.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity("proto/rovers-p03.proto",
                      "states/rovers-p03.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity("proto/rovers-p15.proto",
                      "states/rovers-p15.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity("proto/rovers-p15.proto",
                      "states/rovers-p15.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity("proto/CityCar-p3-2-2-0-1.proto",
                      "states/citycar-p3-2-2-0-1.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity("proto/CityCar-p3-2-2-0-1.proto",
                      "states/citycar-p3-2-2-0-1.txt", PLAN_HEUR_RELAX_TYPE_MAX);

    srand(time(NULL));
    heurRelaxIdentity2("proto/depot-pfile1.proto",
                       "states/depot-pfile1.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity2("proto/depot-pfile1.proto",
                       "states/depot-pfile1.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity2("proto/depot-pfile5.proto",
                       "states/depot-pfile5.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity2("proto/depot-pfile5.proto",
                       "states/depot-pfile5.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity2("proto/rovers-p03.proto",
                       "states/rovers-p03.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity2("proto/rovers-p03.proto",
                       "states/rovers-p03.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity2("proto/rovers-p15.proto",
                       "states/rovers-p15.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity2("proto/rovers-p15.proto",
                       "states/rovers-p15.txt", PLAN_HEUR_RELAX_TYPE_MAX);
    heurRelaxIdentity2("proto/CityCar-p3-2-2-0-1.proto",
                       "states/citycar-p3-2-2-0-1.txt", PLAN_HEUR_RELAX_TYPE_ADD);
    heurRelaxIdentity2("proto/CityCar-p3-2-2-0-1.proto",
                       "states/citycar-p3-2-2-0-1.txt", PLAN_HEUR_RELAX_TYPE_MAX);
}
