#include <boruvka/alloc.h>

#include "plan/search_ehc.h"

plan_search_ehc_t *planSearchEHCNew(plan_t *plan)
{
    plan_search_ehc_t *ehc;

    ehc = BOR_ALLOC(plan_search_ehc_t);
    ehc->plan = plan;
    return ehc;
}

void planSearchEHCDel(plan_search_ehc_t *ehc)
{
    BOR_FREE(ehc);
}
