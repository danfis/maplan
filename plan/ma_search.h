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

typedef int (*plan_ma_search_init_step_fn)(plan_ma_agent_t *agent,
                                           plan_ma_search_t *search);

typedef int (*plan_ma_search_step_fn)(plan_ma_agent_t *agent,
                                      plan_ma_search_t *search);

typedef int (*plan_ma_search_update_fn)(plan_ma_agent_t *agent,
                                        plan_ma_search_t *search,
                                        const plan_ma_msg_t *msg);

struct _plan_ma_search_t {
    plan_ma_search_init_step_fn init_step_fn;
    plan_ma_search_step_fn step_fn;
    plan_ma_search_update_fn update_fn;

    plan_state_pool_t *state_pool;
    plan_state_space_t *state_space;
};

plan_ma_search_t *planMASearchEHCNew(void);

int planMASearchInitStep(plan_ma_agent_t *agent, plan_ma_search_t *search);
int planMASearchStep(plan_ma_agent_t *agent, plan_ma_search_t *search);
int planMASearchUpdate(plan_ma_agent_t *agent, plan_ma_search_t *search,
                       const plan_ma_msg_t *msg);



void _planMASearchInit(plan_ma_search_t *search,
                       plan_ma_search_init_step_fn init_step_fn,
                       plan_ma_search_step_fn step_fn,
                       plan_ma_search_update_fn update_fn);

#endif /* __PLAN_MA_SEARCH_H__ */
