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
    int agent_id;
    int cost, heuristic;
    int state_id;
    int got_msg[NUM_NODES];

    bzero(got_msg, sizeof(int) * NUM_NODES);

    msg = planMAMsgNew();
    planMAMsgSetPublicState(msg, 0, "aa", 2, 1, th->id, 2 * th->id);
    planMACommQueueSendToAll(th->queue, msg);
    planMAMsgDel(msg);

    for (i = 0; i < NUM_NODES - 1; ++i){
        msg = planMACommQueueRecvBlock(th->queue);
        assertNotEquals(msg, NULL);

        if (msg){
            assertTrue(planMAMsgIsPublicState(msg));
            agent_id  = planMAMsgPublicStateAgent(msg);
            state_id  = planMAMsgPublicStateStateId(msg);
            cost      = planMAMsgPublicStateCost(msg);
            heuristic = planMAMsgPublicStateHeur(msg);
            assertEquals(agent_id, 0);
            assertEquals(state_id, 1);
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
