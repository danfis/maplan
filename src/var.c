#include <string.h>
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

void planVarCopy(plan_var_t *dst, const plan_var_t *src)
{
    int i;

    dst->name = strdup(src->name);
    dst->range = src->range;
    dst->fact_name = BOR_ALLOC_ARR(char *, dst->range);
    for (i = 0; i < dst->range; ++i){
        dst->fact_name[i] = strdup(src->fact_name[i]);
    }

    dst->axiom_layer = src->axiom_layer;
}
