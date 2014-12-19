#include <pthread.h>
#include <cu/cu.h>
#include <plan/ma_comm.h>

static void *thPingPong(void *arg)
{
    plan_ma_comm_t *comm = (plan_ma_comm_t *)arg;
    int agent_id = comm->node_id;
    plan_ma_msg_t *msg, *rmsg;
    int i, len;

    msg = planMAMsgNew(PLAN_MA_MSG_PUBLIC_STATE, 0, agent_id);
    if (agent_id == 0){
        planMACommSendToNode(comm, (agent_id + 1) % 2, msg);
        fprintf(stdout, "[%d] PING\n", agent_id);
        fflush(stdout);
    }

    len = 5;
    for (i = 0; i < len; ++i){
        //while ((rmsg = planMACommRecv(comm)) == NULL);
        rmsg = planMACommRecvBlock(comm, -1);
        fprintf(stdout, "[%d] PONG from %d\n", agent_id,
                planMAMsgAgent(rmsg));
        fflush(stdout);
        planMAMsgDel(rmsg);

        if (i == len - 1 && agent_id == 0)
            break;

        planMACommSendToNode(comm, (agent_id + 1) % 2, msg);
        fprintf(stdout, "[%d] PING\n", agent_id);
        fflush(stdout);
    }

    planMAMsgDel(msg);
    return NULL;
}

TEST(testMACommNanomsgPingPong)
{
    plan_ma_comm_t *comm[2];
    pthread_t th[2];

    fprintf(stdout, "---- PingPong Inproc ----\n");
    comm[0] = planMACommInprocNew(0, 2);
    comm[1] = planMACommInprocNew(1, 2);

    assertEquals(planMACommId(comm[0]), 0);
    assertEquals(planMACommId(comm[1]), 1);
    assertEquals(planMACommSize(comm[0]), 2);
    assertEquals(planMACommSize(comm[1]), 2);

    pthread_create(th + 0, NULL, thPingPong, comm[0]);
    pthread_create(th + 1, NULL, thPingPong, comm[1]);
    pthread_join(th[0], NULL);
    pthread_join(th[1], NULL);

    planMACommDel(comm[0]);
    planMACommDel(comm[1]);

    fprintf(stdout, "---- PingPong Inproc END ----\n");
}
