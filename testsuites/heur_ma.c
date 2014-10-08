#include <cu/cu.h>
#include <boruvka/alloc.h>
#include "plan/heur.h"

#include "state_pool.h"


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
static int testMAHeur(plan_heur_t **heur, plan_ma_comm_t **comm,
                      int agent_size, int agent_id,
                      const plan_state_t *state, plan_cost_t target)
{
    plan_heur_res_t res;
    plan_ma_msg_t *msg;
    int msg_agent_id;
    int status;

    planHeurResInit(&res);
    status = planHeurMA(heur[agent_id], comm[agent_id], state, &res);
    while (status != 0){
        msg = nextMsg(comm, agent_size, &msg_agent_id);
        if (planMAMsgIsHeurMaxRequest(msg)){
            planHeurMARequest(heur[msg_agent_id], comm[msg_agent_id], msg);

        }else if (planMAMsgIsHeurMaxResponse(msg)){
            status = planHeurMAUpdate(heur[msg_agent_id], comm[msg_agent_id],
                                      msg, &res);

        }else{
            fprintf(stderr, "Error: Unexpected message: %d\n",
                    planMAMsgType(msg));
            assertTrue(0);
            return -1;
        }

        planMAMsgDel(msg);
    }

    assertEquals(res.heur, target);
    if (res.heur != target)
        return res.heur;
    return -1;
}

static void testHeurMAMaxProb(const plan_problem_agents_t *p,
                              state_pool_t *state_pool)
{
    int agent_size = p->agent_size;
    plan_heur_t *ma_heur[agent_size];
    plan_heur_t *seq_heur;
    plan_state_t *seq_state;
    plan_ma_comm_queue_pool_t *comm_pool;
    plan_ma_comm_t *comm[agent_size];
    int i, line, r;
    plan_heur_res_t res;

    comm_pool = planMACommQueuePoolNew(agent_size);
    for (i = 0; i < agent_size; ++i){
        ma_heur[i] = planHeurMARelaxMaxNew(p->agent + i);
        comm[i] = planMACommQueue(comm_pool, i);
    }

    seq_heur = planHeurRelaxMaxNew(p->glob.var, p->glob.var_size,
                                   p->glob.goal,
                                   p->glob.op, p->glob.op_size,
                                   p->glob.succ_gen);
    seq_state = planStateNew(p->glob.state_pool);

    for (i = 0; i < agent_size; ++i){
        statePoolReset(state_pool);
        line = 0;
        while (statePoolNext(state_pool, seq_state) == 0){
            ++line;
            planHeurResInit(&res);
            planHeur(seq_heur, seq_state, &res);

            r = testMAHeur(ma_heur, comm, agent_size, i, seq_state, res.heur);
            if (r >= 0){
                fprintf(stderr, "Failed with agent %d on %d'th line"
                                " (target: %d, got: %d)\n",
                        i, line, res.heur, r);
            }
        }
    }

    planHeurDel(seq_heur);
    planStateDel(seq_state);
    for (i = 0; i < agent_size; ++i){
        planHeurDel(ma_heur[i]);
    }
    planMACommQueuePoolDel(comm_pool);
}

static void runTestHeurMAMax(const char *proto, const char *states)
{
    plan_problem_agents_t *p;
    state_pool_t state_pool;

    p = planProblemAgentsFromProto(proto, PLAN_PROBLEM_USE_CG);
    if (p == NULL)
        exit(-1);

    statePoolInit(&state_pool, states);

    testHeurMAMaxProb(p, &state_pool);

    statePoolFree(&state_pool);
    planProblemAgentsDel(p);
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
