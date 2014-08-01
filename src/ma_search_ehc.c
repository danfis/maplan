#include <boruvka/alloc.h>

#include "plan/ma_search.h"

struct _plan_ma_search_ehc_t {
    plan_ma_search_t search;
};
typedef struct _plan_ma_search_ehc_t plan_ma_search_ehc_t;

static int initStep(plan_ma_agent_t *agent, plan_ma_search_t *search);
static int step(plan_ma_agent_t *agent, plan_ma_search_t *search);
static int update(plan_ma_agent_t *agent, plan_ma_search_t *search,
                  const plan_ma_msg_t *msg);

plan_ma_search_t *planMASearchEHCNew(void)
{
    plan_ma_search_ehc_t *search;

    search = BOR_ALLOC(plan_ma_search_ehc_t);
    _planMASearchInit(&search->search, initStep, step, update);

    return &search->search;
}

static int initStep(plan_ma_agent_t *agent, plan_ma_search_t *search)
{
    return 0;
}

static int step(plan_ma_agent_t *agent, plan_ma_search_t *search)
{
    return 0;
}

static int update(plan_ma_agent_t *agent, plan_ma_search_t *search,
                  const plan_ma_msg_t *msg)
{
    return 0;
}
