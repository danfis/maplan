#ifndef __PLAN_SEARCH_EHC_H__
#define __PLAN_SEARCH_EHC_H__

#include <plan/plan.h>

/**
 * Enforced Hill Climbing Search
 * ==============================
 */


struct _plan_search_ehc_t {
    plan_t *plan;
};
typedef struct _plan_search_ehc_t plan_search_ehc_t;

plan_search_ehc_t *planSearchEHCNew(plan_t *plan);
void planSearchEHCDel(plan_search_ehc_t *ehc);

#endif /* __PLAN_SEARCH_EHC_H__ */
