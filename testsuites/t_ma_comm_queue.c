#include <cu/cu.h>
#include <stdio.h>
#include "plan/ma_comm_queue.h"

#define NUM_NODES 8

struct _th_t {
    int id;
    plan_ma_comm_queue_t *queue;
    pthread_t thread;
};
typedef struct _th_t th_t;

void *thRun(void *_th)
{
    th_t *th = _th;
    plan_ma_msg_t *msg;
    int i;
    char agent_name[3];
    char state[10];
    int cost, heuristic;
    int got_msg[NUM_NODES];

    bzero(got_msg, sizeof(int) * NUM_NODES);

    msg = planMAMsgNew();
    planMAMsgSetPublicState(msg, "agent", "aa", 2, th->id, 2 * th->id);
    planMACommQueueSendToAll(th->queue, msg);
    planMAMsgDel(msg);

    for (i = 0; i < NUM_NODES - 1; ++i){
        msg = planMACommQueueRecvBlock(th->queue);
        assertNotEquals(msg, NULL);

        if (msg){
            assertTrue(planMAMsgIsPublicState(msg));
            planMAMsgGetPublicState(msg, agent_name, 3, state, 10, &cost, &heuristic);
            assertEquals(strcmp(agent_name, "ag"), 0);
            assertEquals(2 * cost, heuristic);
            got_msg[cost] += 1;
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

    msg = planMACommQueueRecv(th->queue);
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
