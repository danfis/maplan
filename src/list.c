#include "plan/list.h"

void _planListInit(plan_list_t *l,
                   plan_list_del_fn del_fn,
                   plan_list_push_fn push_fn,
                   plan_list_pop_fn pop_fn,
                   plan_list_clear_fn clear_fn)
{
    l->del_fn   = del_fn;
    l->push_fn  = push_fn;
    l->pop_fn   = pop_fn;
    l->clear_fn = clear_fn;
}

void _planListFree(plan_list_t *l)
{
}
