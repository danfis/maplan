#include <cu/cu.h>
#include <plan/fa_mutex.h>

void faMutex(const char *fn, unsigned flags)
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
    planFAMutexFind(prob, &state, &ms, flags);
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
    faMutex("proto/simple.proto", 0);
    faMutex("proto/depot-pfile1.proto", 0);
    faMutex("proto/depot-pfile2.proto", 0);
    faMutex("proto/depot-pfile5.proto", 0);
    faMutex("proto/driverlog-pfile1.proto", 0);
    faMutex("proto/driverlog-pfile3.proto", 0);
    faMutex("proto/openstacks-p03.proto", 0);
    faMutex("proto/rovers-p01.proto", 0);
    faMutex("proto/rovers-p02.proto", 0);
    faMutex("proto/rovers-p03.proto", 0);
    faMutex("proto/rovers-p15.proto", 0);
}

TEST(testFAMutexGoal)
{
    unsigned flags;

    flags = PLAN_FA_MUTEX_ONLY_GOAL;

    faMutex("proto/simple.proto", flags);
    faMutex("proto/depot-pfile1.proto", flags);
    faMutex("proto/depot-pfile2.proto", flags);
    faMutex("proto/depot-pfile5.proto", flags);
    faMutex("proto/driverlog-pfile1.proto", flags);
    faMutex("proto/driverlog-pfile3.proto", flags);
    faMutex("proto/openstacks-p03.proto", flags);
    faMutex("proto/rovers-p01.proto", flags);
    faMutex("proto/rovers-p02.proto", flags);
    faMutex("proto/rovers-p03.proto", flags);
    faMutex("proto/rovers-p15.proto", flags);
}
