#include <cu/cu.h>
#include <boruvka/alloc.h>
#include "plan/heur.h"

#include "state_pool.h"

typedef plan_heur_t *(*heur_new_fn)(const plan_problem_t *prob);

plan_heur_t *seqHeurMaxNew(const plan_problem_t *prob)
{
    return planHeurRelaxMaxNew(prob->var, prob->var_size, prob->goal,
                               prob->op, prob->op_size);
}

plan_heur_t *seqHeurLMCutNew(const plan_problem_t *prob)
{
    return planHeurLMCutNew(prob->var, prob->var_size, prob->goal,
                            prob->op, prob->op_size);
}

static plan_ma_msg_t *nextMsg(plan_ma_comm_t **comm,
                              int agent_size, int *agent_id)
{
    int i;
    plan_ma_msg_t *msg = NULL;

    while (msg == NULL){
        for (i = 0; i < agent_size; ++i){
            msg = planMACommRecv(comm[i]);
            if (msg){
                *agent_id = i;
                return msg;
            }
        }
    }

    return NULL;
}

static int cnt = 0;
static plan_cost_t testMAHeur(plan_heur_t **heur, plan_ma_comm_t **comm,
                              int agent_size, int agent_id,
                              const plan_state_t *state,
                              plan_heur_t *seq_heur)
{
    plan_heur_res_t res, seq_res;
    plan_ma_msg_t *msg;
    int msg_agent_id;
    int status;
    int i;

    ++cnt;
    /*
    if (cnt != 1378)
        return;
    for (i = 0; i < planStateSize(state); ++i)
        fprintf(stdout, " %d", planStateGet(state, i));
    fprintf(stdout, "\n");
    */
    //fprintf(stdout, "CNT %d\n", cnt);


    planHeurResInit(&res);
    status = planHeurMAState(heur[agent_id], comm[agent_id], state, &res);
    while (status != 0){
        msg = nextMsg(comm, agent_size, &msg_agent_id);
        if (planMAMsgHeurType(msg) == PLAN_MA_MSG_HEUR_REQUEST){
            planHeurMARequest(heur[msg_agent_id], comm[msg_agent_id], msg);

        }else if (planMAMsgHeurType(msg) == PLAN_MA_MSG_HEUR_UPDATE){
            status = planHeurMAUpdate(heur[msg_agent_id], comm[msg_agent_id],
                                      msg, &res);

        }else{
            fprintf(stderr, "Error: Unexpected message: %d\n",
                    planMAMsgType(msg));
            assertTrue(0);
            return PLAN_HEUR_DEAD_END;
        }

        planMAMsgDel(msg);
    }

    if (res.heur == PLAN_HEUR_DEAD_END){
        printf("DEAD END");
    }else{
        printf("%d", (int)res.heur);
    }
    for (i = 0; i < planStateSize(state); ++i){
        printf(" %d", planStateGet(state, i));
    }
    printf("\n");

    if (seq_heur){
        planHeurResInit(&seq_res);
        planHeurState(seq_heur, state, &seq_res);
        assertEquals(res.heur, seq_res.heur);
        if (res.heur != seq_res.heur){
            fprintf(stderr, "%d: ma: %d, seq: %d\n", cnt, res.heur, seq_res.heur);
        }
    }

    return res.heur;
}

static void testHeurMAProb(const plan_problem_agents_t *p,
                           state_pool_t *state_pool,
                           const char *costs,
                           heur_new_fn heur_new,
                           unsigned int limit,
                           heur_new_fn seq_heur_new)
{
    int agent_size = p->agent_size;
    plan_heur_t *ma_heur[agent_size];
    plan_heur_t *seq_heur = NULL;
    plan_state_t *seq_state;
    plan_ma_comm_t *comm[agent_size];
    plan_ma_state_t *ma_state[agent_size];
    FILE *fcosts;
    int i, cost, cost_optimal;
    unsigned int line;

    for (i = 0; i < agent_size; ++i){
        ma_state[i] = planMAStateNew(p->agent[i].state_pool, agent_size, i);
        ma_heur[i] = heur_new(p->agent + i);
        planHeurMAInit(ma_heur[i], agent_size, i, ma_state[i]);
        comm[i] = planMACommInprocNew(i, agent_size);
    }
    if (seq_heur_new)
        seq_heur = seq_heur_new(&p->glob);

    seq_state = planStateNew(p->glob.var_size);

    for (i = 0; i < agent_size; ++i){
        statePoolReset(state_pool);
        if (costs)
            fcosts = fopen(costs, "r");

        for (line = 0;
                statePoolNext(state_pool, seq_state) == 0 && line < limit;
                ++line){
            cost = testMAHeur(ma_heur, comm, agent_size, i, seq_state, seq_heur);
            if (costs){
                fscanf(fcosts, "%d", &cost_optimal);
                assertTrue(cost <= cost_optimal);
            }
        }

        if (costs)
            fclose(fcosts);
    }

    planStateDel(seq_state);
    for (i = 0; i < agent_size; ++i){
        planHeurDel(ma_heur[i]);
        planMACommDel(comm[i]);
        planMAStateDel(ma_state[i]);
    }

    if (seq_heur)
        planHeurDel(seq_heur);
}

static void runTestHeurMA(const char *name, const char *proto,
                          const char *states, const char *costs,
                          heur_new_fn heur_new,
                          unsigned int limit, heur_new_fn seq_heur_new)
{
    plan_problem_agents_t *p;
    state_pool_t state_pool;

    printf("---- %s: %s ----\n", name, proto);

    p = planProblemAgentsFromProto(proto, PLAN_PROBLEM_USE_CG);
    if (p == NULL)
        exit(-1);

    statePoolInit(&state_pool, states);

    testHeurMAProb(p, &state_pool, costs, heur_new, limit, seq_heur_new);

    statePoolFree(&state_pool);
    planProblemAgentsDel(p);

    printf("---- %s: %s END ----\n", name, proto);
}

TEST(testHeurMAFF)
{
    runTestHeurMA("ma-ff", "proto/simple.proto",
                  "states/simple.txt", NULL, planHeurMARelaxFFNew, -1, NULL);
    runTestHeurMA("ma-ff", "proto/depot-pfile1.proto",
                  "states/depot-pfile1.txt", NULL, planHeurMARelaxFFNew, -1, NULL);
    runTestHeurMA("ma-ff", "proto/depot-pfile5.proto",
                  "states/depot-pfile5.txt", NULL, planHeurMARelaxFFNew, 200, NULL);
    runTestHeurMA("ma-ff", "proto/rovers-p03.proto",
                  "states/rovers-p03.txt", NULL, planHeurMARelaxFFNew, -1, NULL);
    runTestHeurMA("ma-ff", "proto/rovers-p15.proto",
                  "states/rovers-p15.txt", NULL, planHeurMARelaxFFNew, -1, NULL);
}

TEST(testHeurMAMax)
{
    runTestHeurMA("ma-max", "proto/simple.proto",
                  "states/simple.txt", NULL, planHeurMARelaxMaxNew, -1,
                  seqHeurMaxNew);
    runTestHeurMA("ma-max", "proto/depot-pfile1.proto",
                  "states/depot-pfile1.txt", NULL, planHeurMARelaxMaxNew, -1,
                  seqHeurMaxNew);
    runTestHeurMA("ma-max", "proto/depot-pfile5.proto",
                  "states/depot-pfile5.txt", NULL, planHeurMARelaxMaxNew, 200,
                  seqHeurMaxNew);
    runTestHeurMA("ma-max", "proto/rovers-p03.proto",
                  "states/rovers-p03.txt", NULL, planHeurMARelaxMaxNew, -1,
                  seqHeurMaxNew);
    runTestHeurMA("ma-max", "proto/rovers-p15.proto",
                  "states/rovers-p15.txt", NULL, planHeurMARelaxMaxNew, -1,
                  seqHeurMaxNew);
}

TEST(testHeurMALMCut)
{
    runTestHeurMA("ma-lm-cut", "proto/simple.proto",
                  "states/simple.txt", NULL, planHeurMALMCutNew, -1, NULL);
    runTestHeurMA("ma-lm-cut", "proto/depot-pfile1.proto",
                  "states/depot-pfile1.txt", "states/depot-pfile1.cost.txt",
                  planHeurMALMCutNew, -1, NULL);
    runTestHeurMA("ma-lm-cut", "proto/depot-pfile5.proto",
                  "states/depot-pfile5.txt", NULL, planHeurMALMCutNew, 200, NULL);
    runTestHeurMA("ma-lm-cut", "proto/rovers-p03.proto",
                  "states/rovers-p03.txt", "states/rovers-p03.cost.txt",
                  planHeurMALMCutNew, -1, NULL);
    runTestHeurMA("ma-lm-cut", "proto/rovers-p15.proto",
                  "states/rovers-p15.txt", NULL, planHeurMALMCutNew, -1, NULL);
    runTestHeurMA("ma-lm-cut", "proto/sokoban-p01.proto",
                  "states/sokoban-p01.txt", NULL, planHeurMALMCutNew, -1, NULL);
}

TEST(testHeurMADTG)
{
    runTestHeurMA("ma-dtg", "proto/simple.proto",
                  "states/simple.txt", NULL, planHeurMADTGNew, -1, NULL);
    runTestHeurMA("ma-dtg", "proto/depot-pfile1.proto",
                  "states/depot-pfile1.txt", NULL, planHeurMADTGNew, -1, NULL);
    runTestHeurMA("ma-dtg", "proto/depot-pfile5.proto",
                  "states/depot-pfile5.txt", NULL, planHeurMADTGNew, 200, NULL);
    runTestHeurMA("ma-dtg", "proto/rovers-p03.proto",
                  "states/rovers-p03.txt", NULL, planHeurMADTGNew, -1, NULL);
    runTestHeurMA("ma-dtg", "proto/rovers-p15.proto",
                  "states/rovers-p15.txt", NULL, planHeurMADTGNew, -1, NULL);
}
