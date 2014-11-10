#include <cu/cu.h>
#include <stdio.h>
#include <strings.h>
#include <pthread.h>
#include "plan/ma_comm.h"

#define NUM_NODES 8

struct _th_t {
    int id;
    plan_ma_comm_t *queue;
    pthread_t thread;
};
typedef struct _th_t th_t;

static void *thRun(void *_th)
{
    th_t *th = _th;
    plan_ma_msg_t *msg;
    int i;
    int agent_id;
    int got_msg[NUM_NODES];

    bzero(got_msg, sizeof(int) * NUM_NODES);

    msg = planMAMsgNew(PLAN_MA_MSG_TERMINATE,
                       PLAN_MA_MSG_TERMINATE_REQUEST, th->id);
    planMAMsgTerminateSetAgent(msg, th->id);
    planMACommSendToAll(th->queue, msg);
    planMAMsgDel(msg);

    for (i = 0; i < NUM_NODES - 1; ++i){
        msg = planMACommRecvBlock(th->queue, 0);
        assertNotEquals(msg, NULL);

        if (msg){
            assertEquals(planMAMsgType(msg), PLAN_MA_MSG_TERMINATE);
            assertEquals(planMAMsgSubType(msg), PLAN_MA_MSG_TERMINATE_REQUEST);
            agent_id = planMAMsgAgent(msg);
            assertNotEquals(agent_id, th->id);
            got_msg[agent_id] += 1;
            planMAMsgDel(msg);
        }
    }

    for (i = 0; i < NUM_NODES - 1; ++i){
        if (i == th->id){
            assertEquals(got_msg[i], 0);
        }else{
            assertEquals(got_msg[i], 1);
        }
    }

    msg = planMACommRecv(th->queue);
    assertEquals(msg, NULL);

    return NULL;
}

TEST(maCommQueue1)
{
    plan_ma_comm_queue_pool_t *pool;
    int i;
    th_t th[NUM_NODES];

    pool = planMACommQueuePoolNew(NUM_NODES);
    for (i = 0; i < NUM_NODES; ++i){
        th[i].id = i;
        th[i].queue = planMACommQueue(pool, i);

        pthread_create(&th[i].thread, NULL, thRun, &th[i]);
    }

    for (i = 0; i < NUM_NODES; ++i){
        pthread_join(th[i].thread, NULL);
    }

    planMACommQueuePoolDel(pool);
}
