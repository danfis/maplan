#include <cu/cu.h>
#include <stdio.h>
#include "ma_comm_queue.h"

#define NUM_NODES 3

struct _th_t {
    int id;
    plan_ma_comm_queue_t *queue;
    pthread_t thread;
};
typedef struct _th_t th_t;

void *thRun(void *_th)
{
    th_t *th = _th;
    PlanMultiAgentMsg msg = PLAN_MULTI_AGENT_MSG__INIT;
    PlanMultiAgentMsg *msg_in;
    int i;

    msg.cost = th->id;
    planMACommQueueSendToAll(th->queue, &msg);

    for (i = 0; i < NUM_NODES - 1; ++i){
        msg_in = planMACommQueueRecvBlock(th->queue);
        if (msg_in){
            plan_multi_agent_msg__free_unpacked(msg_in, NULL);
        }
    }

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
