#include <cu/cu.h>
#include <boruvka/alloc.h>
#include "plan/heur.h"

#include "state_pool.h"

typedef plan_heur_t *(*heur_new_fn)(const plan_problem_t *prob);

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

static void testMAHeur(plan_heur_t **heur, plan_ma_comm_t **comm,
                       int agent_size, int agent_id,
                       const plan_state_t *state)
{
    plan_heur_res_t res;
    plan_ma_msg_t *msg;
    int msg_agent_id;
    int status;
    int i;

    planHeurResInit(&res);
    status = planHeurMA(heur[agent_id], comm[agent_id], state, &res);
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
            return;
        }

        planMAMsgDel(msg);
    }

    printf("%d", (int)res.heur);
    for (i = 0; i < planStateSize(state); ++i){
        printf(" %d", planStateGet(state, i));
    }
    printf("\n");
}

static void testHeurMAProb(const plan_problem_agents_t *p,
                           state_pool_t *state_pool,
                           heur_new_fn heur_new,
                           unsigned int limit)
{
    int agent_size = p->agent_size;
    plan_heur_t *ma_heur[agent_size];
    plan_state_t *seq_state;
    plan_ma_comm_queue_pool_t *comm_pool;
    plan_ma_comm_t *comm[agent_size];
    int i;
    unsigned int line;

    comm_pool = planMACommQueuePoolNew(agent_size);
    for (i = 0; i < agent_size; ++i){
        ma_heur[i] = heur_new(p->agent + i);
        comm[i] = planMACommQueue(comm_pool, i);
    }

    seq_state = planStateNew(p->glob.state_pool);

    for (i = 0; i < agent_size; ++i){
        statePoolReset(state_pool);
        for (line = 0;
                statePoolNext(state_pool, seq_state) == 0 && line < limit;
                ++line){
            testMAHeur(ma_heur, comm, agent_size, i, seq_state);
        }
    }

    planStateDel(seq_state);
    for (i = 0; i < agent_size; ++i){
        planHeurDel(ma_heur[i]);
    }
    planMACommQueuePoolDel(comm_pool);
}

static void runTestHeurMA(const char *name, const char *proto,
                          const char *states, heur_new_fn heur_new,
                          unsigned int limit)
{
    plan_problem_agents_t *p;
    state_pool_t state_pool;

    printf("---- %s: %s ----\n", name, proto);

    p = planProblemAgentsFromProto(proto, PLAN_PROBLEM_USE_CG);
    if (p == NULL)
        exit(-1);

    statePoolInit(&state_pool, states);

    testHeurMAProb(p, &state_pool, heur_new, limit);

    statePoolFree(&state_pool);
    planProblemAgentsDel(p);

    printf("---- %s: %s END ----\n", name, proto);
}

TEST(testHeurMAFF)
{
    runTestHeurMA("ma-ff", "../data/simple/pfile.proto",
                  "states/simple.txt", planHeurMARelaxFFNew, -1);
    runTestHeurMA("ma-ff", "../data/ma-benchmarks/depot/pfile1.proto",
                  "states/depot-pfile1.txt", planHeurMARelaxFFNew, -1);
    runTestHeurMA("ma-ff", "../data/ma-benchmarks/depot/pfile5.proto",
                  "states/depot-pfile5.txt", planHeurMARelaxFFNew, 200);
    runTestHeurMA("ma-ff", "../data/ma-benchmarks/rovers/p03.proto",
                  "states/rovers-p03.txt", planHeurMARelaxFFNew, -1);
    runTestHeurMA("ma-ff", "../data/ma-benchmarks/rovers/p15.proto",
                  "states/rovers-p15.txt", planHeurMARelaxFFNew, -1);
}

/*

TEST(testHeurMAMaxSimple)
{
    runTestHeurMAMax("../data/simple/pfile.proto",
                     "states/simple.txt");
}

TEST(testHeurMAMaxDepotPfile1)
{
    runTestHeurMAMax("../data/ma-benchmarks/depot/pfile1.proto",
                     "states/depot-pfile1.txt");
}

TEST(testHeurMAMaxDepotPfile2)
{
    runTestHeurMAMax("../data/ma-benchmarks/depot/pfile5.proto",
                     "states/depot-pfile5.txt");
}

TEST(testHeurMAMaxRoversP03)
{
    runTestHeurMAMax("../data/ma-benchmarks/rovers/p03.proto",
                     "states/rovers-p03.txt");
}

TEST(testHeurMAMaxRoversP15){
    runTestHeurMAMax("../data/ma-benchmarks/rovers/p15.proto",
                     "states/rovers-p15.txt");
}
*/
