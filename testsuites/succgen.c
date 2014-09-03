#include <cu/cu.h>
#include <boruvka/alloc.h>
#include "plan/problem.h"
#include "plan/succgen.h"

#define NUM_STATES 20
static unsigned states[NUM_STATES][9] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 0, 0, 1 },
    { 0, 1, 1, 0, 1, 0, 2, 1, 0 },
    { 1, 0, 1, 0, 0, 0, 0, 2, 0 },
    { 0, 1, 0, 1, 0, 1, 0, 3, 2 },
    { 2, 0, 0, 0, 0, 0, 4, 2, 1 },
    { 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 0, 1, 0, 1, 0, 4, 3 },
    { 3, 0, 0, 0, 0, 0, 3, 2, 0 },
    { 4, 1, 1, 0, 0, 0, 0, 2, 0 },
    { 0, 1, 0, 0, 1, 0, 0, 1, 4 },
    { 1, 0, 0, 1, 0, 0, 0, 1, 4 },
    { 1, 1, 1, 0, 0, 1, 2, 1, 4 },
    { 0, 0, 0, 0, 1, 0, 3, 2, 4 },
    { 1, 1, 1, 1, 0, 0, 0, 0, 4 },
    { 0, 0, 0, 0, 1, 0, 4, 1, 4 },
    { 0, 1, 0, 0, 1, 1, 4, 2, 4 },
    { 3, 1, 0, 1, 0, 0, 4, 1, 4 },
    { 2, 1, 1, 1, 1, 1, 0, 0, 4 },
    { 1, 1, 0, 0, 1, 0, 0, 3, 4 },
};

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


TEST(testSuccGen)
{
    plan_problem_t *prob;
    plan_succ_gen_t *sg;
    plan_op_t **ops1, **ops2;
    plan_state_id_t sid;
    int ops_size, found1, found2, i, j;
    plan_state_t *state;

    prob = planProblemFromFD("load-from-file.in1.sas");

    sg = planSuccGenNew(prob->op, prob->op_size, NULL);

    ops_size = prob->op_size;
    ops1 = BOR_ALLOC_ARR(plan_op_t *, ops_size);
    ops2 = BOR_ALLOC_ARR(plan_op_t *, ops_size);
    state = planStateNew(prob->state_pool);

    for (i = 0; i < NUM_STATES; ++i){
        for (j = 0; j < 9; ++j){
            planStateSet(state, j, states[i][j]);
        }

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
}
