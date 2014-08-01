#include "plan/ma_search.h"

int planMASearchInitStep(plan_ma_agent_t *agent, plan_ma_search_t *search)
{
    return search->init_step_fn(agent, search);
}

int planMASearchStep(plan_ma_agent_t *agent, plan_ma_search_t *search)
{
    return search->step_fn(agent, search);
}

int planMASearchUpdate(plan_ma_agent_t *agent, plan_ma_search_t *search,
                       const plan_ma_msg_t *msg)
{
    return search->update_fn(agent, search, msg);
}

void _planMASearchInit(plan_ma_search_t *search,
                       plan_ma_search_init_step_fn init_step_fn,
                       plan_ma_search_step_fn step_fn,
                       plan_ma_search_update_fn update_fn)
{
    bzero(search, sizeof(*search));
    search->init_step_fn = init_step_fn;
    search->step_fn = step_fn;
    search->update_fn = update_fn;
}
