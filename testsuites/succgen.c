#include <cu/cu.h>
#include <boruvka/alloc.h>
#include "plan/problem.h"
#include "plan/succgen.h"
#include "state_pool.h"

static int sortOpsCmp(const void *a, const void *b)
{
    plan_op_t *opa = *(plan_op_t **)a;
    plan_op_t *opb = *(plan_op_t **)b;
    if (opa == opb)
        return 0;
    if (opa < opb)
        return -1;
    return 1;
}

static int findOpsLinear(const plan_state_pool_t *pool,
                         plan_op_t *op, int op_size,
                         plan_state_id_t sid,
                         plan_op_t **op_out)
{
    int i, found;

    found = 0;
    for (i = 0; i < op_size; ++i){
        if (planStatePoolPartStateIsSubset(pool, op[i].pre, sid)){
            op_out[found++] = op + i;
        }
    }

    qsort(op_out, found, sizeof(plan_op_t *), sortOpsCmp);
    return found;
}

static int findOpsSG(const plan_succ_gen_t *sg,
                     const plan_state_t *state,
                     plan_op_t **ops,
                     int ops_size)
{
    int found;

    found = planSuccGenFind(sg, state, ops, ops_size);
    qsort(ops, BOR_MIN(found, ops_size), sizeof(plan_op_t *), sortOpsCmp);
    return found;
}

static void test(const char *proto, const char *states)
{
    plan_problem_t *prob;
    plan_succ_gen_t *sg;
    plan_op_t **ops1, **ops2;
    plan_state_id_t sid;
    int ops_size, found1, found2, i, j;
    plan_state_t *state;
    state_pool_t state_pool;

    prob = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    statePoolInit(&state_pool, states);

    sg = planSuccGenNew(prob->op, prob->op_size, NULL);

    ops_size = prob->op_size;
    ops1 = BOR_ALLOC_ARR(plan_op_t *, ops_size);
    ops2 = BOR_ALLOC_ARR(plan_op_t *, ops_size);
    state = planStateNew(prob->state_pool->num_vars);

    for (i = 0; statePoolNext(&state_pool, state) == 0; ++i){
        sid = planStatePoolInsert(prob->state_pool, state);
        found1 = findOpsLinear(prob->state_pool, prob->op, prob->op_size, sid, ops1);
        found2 = findOpsSG(sg, state, ops2, ops_size);
        assertEquals(found1, found2);
        if (found1 == found2){
            for (j = 0; j < found1; ++j)
                assertEquals(ops1[j], ops2[j]);
        }

    }

    found2 = findOpsSG(sg, state, ops2, 2);

    planStateDel(state);
    BOR_FREE(ops1);
    BOR_FREE(ops2);
    planSuccGenDel(sg);

    planProblemDel(prob);

    statePoolFree(&state_pool);
}


TEST(testSuccGen)
{
    test("proto/depot-pfile1.proto", "states/depot-pfile1.txt");
    test("proto/depot-pfile5.proto", "states/depot-pfile5.txt");
    test("proto/rovers-p03.proto", "states/rovers-p03.txt");
    test("proto/rovers-p15.proto", "states/rovers-p15.txt");
}
