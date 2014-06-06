#include <boruvka/alloc.h>
#include "plan/list_lazy_fifo.h"

struct _plan_list_lazy_fifo_el_t {
    plan_state_id_t parent_state_id;
    plan_operator_t *op;
    bor_list_t fifo;
};
typedef struct _plan_list_lazy_fifo_el_t plan_list_lazy_fifo_el_t;


plan_list_lazy_fifo_t *planListLazyFifoNew(void)
{
    plan_list_lazy_fifo_t *l;

    l = BOR_ALLOC(plan_list_lazy_fifo_t);
    borListInit(l);

    return l;
}

void planListLazyFifoDel(plan_list_lazy_fifo_t *l)
{
    planListLazyFifoClear(l);
    BOR_FREE(l);
}

void planListLazyFifoPush(plan_list_lazy_fifo_t *l,
                          plan_state_id_t parent_state_id,
                          plan_operator_t *op)
{
    plan_list_lazy_fifo_el_t *el;

    el = BOR_ALLOC(plan_list_lazy_fifo_el_t);
    el->parent_state_id = parent_state_id;
    el->op = op;
    borListInit(&el->fifo);
    borListAppend(l, &el->fifo);
}

int planListLazyFifoPop(plan_list_lazy_fifo_t *l,
                        plan_state_id_t *parent_state_id,
                        plan_operator_t **op)
{
    bor_list_t *item;
    plan_list_lazy_fifo_el_t *el;

    if (borListEmpty(l))
        return -1;

    // get next element from front of the list
    item = borListNext(l);
    borListDel(item);
    el = BOR_LIST_ENTRY(item, plan_list_lazy_fifo_el_t, fifo);

    // copy values to the output args
    *parent_state_id = el->parent_state_id;
    *op = el->op;

    // free allocated memory
    BOR_FREE(el);

    return 0;
}

void planListLazyFifoClear(plan_list_lazy_fifo_t *l)
{
    bor_list_t *item;
    plan_list_lazy_fifo_el_t *el;

    while (!borListEmpty(l)){
        item = borListNext(l);
        borListDel(item);
        el = BOR_LIST_ENTRY(item, plan_list_lazy_fifo_el_t, fifo);
        BOR_FREE(el);
    }
}
