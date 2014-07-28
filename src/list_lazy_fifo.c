#include <unistd.h>
#include <boruvka/alloc.h>
#include <boruvka/fifo.h>
#include "plan/list_lazy.h"

struct _plan_list_lazy_fifo_t {
    plan_list_lazy_t list;
    bor_fifo_t *fifo;
};
typedef struct _plan_list_lazy_fifo_t plan_list_lazy_fifo_t;

struct _plan_list_lazy_fifo_el_t {
    plan_state_id_t parent_state_id;
    plan_operator_t *op;
};
typedef struct _plan_list_lazy_fifo_el_t plan_list_lazy_fifo_el_t;

#define LIST_FROM_PARENT(_list) \
    bor_container_of((_list), plan_list_lazy_fifo_t, list)

static void planListLazyFifoDel(plan_list_lazy_t *l);
static void planListLazyFifoPush(plan_list_lazy_t *l,
                                 plan_cost_t cost,
                                 plan_state_id_t parent_state_id,
                                 plan_operator_t *op);
static int planListLazyFifoPop(plan_list_lazy_t *l,
                               plan_state_id_t *parent_state_id,
                               plan_operator_t **op);
static void planListLazyFifoClear(plan_list_lazy_t *l);


plan_list_lazy_t *planListLazyFifoNew(void)
{
    plan_list_lazy_fifo_t *l;
    size_t segment_size;

    l = BOR_ALLOC(plan_list_lazy_fifo_t);
    planListLazyInit(&l->list,
                     planListLazyFifoDel,
                     planListLazyFifoPush,
                     planListLazyFifoPop,
                     planListLazyFifoClear);

    segment_size = sysconf(_SC_PAGESIZE);
    segment_size *= 4;
    l->fifo = borFifoNewSize(sizeof(plan_list_lazy_fifo_el_t), segment_size);


    return &l->list;
}

void planListLazyFifoDel(plan_list_lazy_t *_l)
{
    plan_list_lazy_fifo_t *l = LIST_FROM_PARENT(_l);
    planListLazyFree(&l->list);
    borFifoDel(l->fifo);
    BOR_FREE(l);
}

static void planListLazyFifoPush(plan_list_lazy_t *_l,
                                 plan_cost_t cost,
                                 plan_state_id_t parent_state_id,
                                 plan_operator_t *op)
{
    plan_list_lazy_fifo_t *l = LIST_FROM_PARENT(_l);
    plan_list_lazy_fifo_el_t el;

    el.parent_state_id = parent_state_id;
    el.op = op;

    borFifoPush(l->fifo, &el);
}

static int planListLazyFifoPop(plan_list_lazy_t *_l,
                               plan_state_id_t *parent_state_id,
                               plan_operator_t **op)
{
    plan_list_lazy_fifo_t *l = LIST_FROM_PARENT(_l);
    plan_list_lazy_fifo_el_t *el;

    if (borFifoEmpty(l->fifo))
        return -1;

    el = borFifoFront(l->fifo);

    // copy values to the output args
    *parent_state_id = el->parent_state_id;
    *op = el->op;

    borFifoPop(l->fifo);

    return 0;
}

static void planListLazyFifoClear(plan_list_lazy_t *_l)
{
    plan_list_lazy_fifo_t *l = LIST_FROM_PARENT(_l);
    borFifoClear(l->fifo);
}
