#ifndef __PLAN_SEARCH_EHC_H__
#define __PLAN_SEARCH_EHC_H__

#include <plan/plan.h>
#include <plan/statespace.h>
#include <plan/succgen.h>
#include <plan/heuristic/goalcount.h>

/**
 * Enforced Hill Climbing Search
 * ==============================
 */


struct _plan_search_ehc_t {
    plan_t *plan;
    plan_state_space_t *state_space;
    plan_heuristic_goalcount_t *heur;
    plan_state_t *state;
    plan_succ_gen_t *succ_gen;
    plan_operator_t **succ_op;
    unsigned best_heur;
    unsigned counter;
};
typedef struct _plan_search_ehc_t plan_search_ehc_t;

plan_search_ehc_t *planSearchEHCNew(plan_t *plan);
void planSearchEHCDel(plan_search_ehc_t *ehc);

void planSearchEHCInit(plan_search_ehc_t *ehc);
int planSearchEHCStep(plan_search_ehc_t *ehc);
int planSearchEHCRun(plan_search_ehc_t *ehc);

#endif /* __PLAN_SEARCH_EHC_H__ */
