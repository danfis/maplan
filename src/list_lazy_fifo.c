#include <boruvka/alloc.h>
#include <boruvka/list.h>
#include "plan/list_lazy.h"

struct _plan_list_lazy_fifo_t {
    plan_list_lazy_t list;
    bor_list_t fifo;
};
typedef struct _plan_list_lazy_fifo_t plan_list_lazy_fifo_t;

struct _plan_list_lazy_fifo_el_t {
    plan_state_id_t parent_state_id;
    plan_operator_t *op;
    bor_list_t fifo;
};
typedef struct _plan_list_lazy_fifo_el_t plan_list_lazy_fifo_el_t;

static void planListLazyFifoDel(void *l);
static void planListLazyFifoPush(void *l,
                                 plan_cost_t cost,
                                 plan_state_id_t parent_state_id,
                                 plan_operator_t *op);
static int planListLazyFifoPop(void *l,
                               plan_state_id_t *parent_state_id,
                               plan_operator_t **op);
static void planListLazyFifoClear(void *l);


plan_list_lazy_t *planListLazyFifoNew(void)
{
    plan_list_lazy_fifo_t *l;

    l = BOR_ALLOC(plan_list_lazy_fifo_t);
    borListInit(&l->fifo);
    planListLazyInit(&l->list,
                     planListLazyFifoDel,
                     planListLazyFifoPush,
                     planListLazyFifoPop,
                     planListLazyFifoClear);

    return &l->list;
}

void planListLazyFifoDel(void *_l)
{
    plan_list_lazy_fifo_t *l = _l;
    planListLazyFree(&l->list);
    planListLazyFifoClear(l);
    BOR_FREE(l);
}

static void planListLazyFifoPush(void *_l,
                                 plan_cost_t cost,
                                 plan_state_id_t parent_state_id,
                                 plan_operator_t *op)
{
    plan_list_lazy_fifo_t *l = _l;
    plan_list_lazy_fifo_el_t *el;

    el = BOR_ALLOC(plan_list_lazy_fifo_el_t);
    el->parent_state_id = parent_state_id;
    el->op = op;
    borListInit(&el->fifo);
    borListAppend(&l->fifo, &el->fifo);
}

static int planListLazyFifoPop(void *_l,
                               plan_state_id_t *parent_state_id,
                               plan_operator_t **op)
{
    plan_list_lazy_fifo_t *l = _l;
    bor_list_t *item;
    plan_list_lazy_fifo_el_t *el;

    if (borListEmpty(&l->fifo))
        return -1;

    // get next element from front of the list
    item = borListNext(&l->fifo);
    borListDel(item);
    el = BOR_LIST_ENTRY(item, plan_list_lazy_fifo_el_t, fifo);

    // copy values to the output args
    *parent_state_id = el->parent_state_id;
    *op = el->op;

    // free allocated memory
    BOR_FREE(el);

    return 0;
}

static void planListLazyFifoClear(void *_l)
{
    plan_list_lazy_fifo_t *l = _l;
    bor_list_t *item;
    plan_list_lazy_fifo_el_t *el;

    while (!borListEmpty(&l->fifo)){
        item = borListNext(&l->fifo);
        borListDel(item);
        el = BOR_LIST_ENTRY(item, plan_list_lazy_fifo_el_t, fifo);
        BOR_FREE(el);
    }
}
