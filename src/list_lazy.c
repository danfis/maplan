#include "plan/list_lazy.h"

void planListLazyInit(plan_list_lazy_t *l,
                      plan_list_lazy_del_fn del_fn,
                      plan_list_lazy_push_fn push_fn,
                      plan_list_lazy_pop_fn pop_fn,
                      plan_list_lazy_clear_fn clear_fn)
{
    l->del_fn   = del_fn;
    l->push_fn  = push_fn;
    l->pop_fn   = pop_fn;
    l->clear_fn = clear_fn;
}

void planListLazyFree(plan_list_lazy_t *l)
{
}
