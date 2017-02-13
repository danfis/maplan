#include <cu/cu.h>
#include <plan/fa_mutex.h>

void faMutex(const char *fn)
{
    plan_problem_t *prob;
    plan_mutex_group_set_t ms;
    int i, j;

    printf("---- %s ----\n", fn);
    prob = planProblemFromProto(fn, PLAN_PROBLEM_USE_CG);
    PLAN_STATE_STACK(state, prob->var_size);
    planStatePoolGetState(prob->state_pool, prob->initial_state, &state);

    //for (i = 0; i < prob->var_size; ++i)
    //    printf("var[%d]: %d\n", i, prob->var[i].range);

    planMutexGroupSetInit(&ms);
    planFAMutexFind(prob, &state, &ms);
    planMutexGroupSetSort(&ms);
    for (i = 0; i < ms.group_size; ++i){
        printf("fa-mutex[%d]: is-goal: %d", i, ms.group[i].is_goal);
        for (j = 0; j < ms.group[i].fact_size; ++j)
            printf(" %d:%d", ms.group[i].fact[j].var, ms.group[i].fact[j].val);
        printf("\n");
    }
    planMutexGroupSetFree(&ms);
    planProblemDel(prob);
}

TEST(testFAMutex)
{
    faMutex("proto/simple.proto");
    faMutex("proto/depot-pfile1.proto");
    faMutex("proto/depot-pfile2.proto");
    faMutex("proto/depot-pfile5.proto");
    faMutex("proto/driverlog-pfile1.proto");
    faMutex("proto/driverlog-pfile3.proto");
    faMutex("proto/openstacks-p03.proto");
    faMutex("proto/rovers-p01.proto");
    faMutex("proto/rovers-p02.proto");
    faMutex("proto/rovers-p03.proto");
    faMutex("proto/rovers-p15.proto");
}
