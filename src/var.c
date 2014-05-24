#include <boruvka/alloc.h>
#include "plan/var.h"

void planVarInit(plan_var_t *plan)
{
    plan->name = NULL;
    plan->range = -1;
    plan->fact_name = NULL;
    plan->axiom_layer = -1;
}

void planVarFree(plan_var_t *plan)
{
    int i;

    if (plan->name)
        BOR_FREE(plan->name);

    if (plan->fact_name != NULL){
        for (i = 0; i < plan->range; ++i){
            BOR_FREE(plan->fact_name[i]);
        }
        BOR_FREE(plan->fact_name);
    }
}

