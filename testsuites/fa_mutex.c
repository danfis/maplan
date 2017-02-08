#include <cu/cu.h>
#include <plan/fa_mutex.h>

void faMutex(const char *fn)
{
    plan_problem_t *prob;
    plan_fa_mutex_set_t ms;
    int i, fact_id;

    printf("---- %s ----\n", fn);
    prob = planProblemFromProto(fn, PLAN_PROBLEM_USE_CG);
    PLAN_STATE_STACK(state, prob->var_size);
    planStatePoolGetState(prob->state_pool, prob->initial_state, &state);

    //for (i = 0; i < prob->var_size; ++i)
    //    printf("var[%d]: %d\n", i, prob->var[i].range);

    planFAMutexSetInit(&ms);
    planFAMutexFind(prob, &state, &ms);
    planFAMutexSetSort(&ms);
    for (i = 0; i < ms.size; ++i){
        printf("fa-mutex[%d]:", i);
        PLAN_ARR_INT_FOR_EACH(ms.fa_mutex + i, fact_id)
            printf(" %d", fact_id);
        printf("\n");
    }
    planFAMutexSetFree(&ms);
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
