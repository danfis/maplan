#ifndef __PLAN_MA_SEARCH_H__
#define __PLAN_MA_SEARCH_H__

#include <plan/ma_agent.h>
#include <plan/search.h>

/**
 * Multi-Agent Search Algorithms
 * ==============================
 */

/** Forward declaration */
typedef struct _plan_ma_search_t plan_ma_search_t;

struct _plan_ma_search_t {
    plan_state_pool_t *state_pool;
    plan_state_space_t *state_space;
};

plan_ma_search_t *planMASearchEHCNew(void);

int planMASearchInitStep(plan_ma_agent_t *agent, plan_ma_search_t *search);
int planMASearchStep(plan_ma_agent_t *agent, plan_ma_search_t *search);
int planMASearchUpdate(plan_ma_agent_t *agent, plan_ma_search_t *search,
                       const plan_ma_msg_t *msg);

#endif /* __PLAN_MA_SEARCH_H__ */
