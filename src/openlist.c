#include <boruvka/alloc.h>
#include "plan/openlist.h"

struct _plan_open_list_node_t {
    bor_pairheap_node_t heap;
    unsigned heuristic;
    plan_state_id_t state_id;
};
typedef struct _plan_open_list_node_t plan_open_list_node_t;

/** Comparator for pairing heap */
static int heapLessThan(const bor_pairheap_node_t *n1,
                        const bor_pairheap_node_t *n2,
                        void *data);

plan_open_list_t *planOpenListNew(plan_state_pool_t *state_pool)
{
    plan_open_list_t *os;
    plan_open_list_node_t elinit;

    os = BOR_ALLOC(plan_open_list_t);
    os->heap = borPairHeapNew(heapLessThan, os);

    os->state_pool = state_pool;

    elinit.heuristic = -1;
    elinit.state_id  = PLAN_NO_STATE;
    os->data_id = planStatePoolDataReserve(os->state_pool,
                                           sizeof(plan_open_list_node_t),
                                           &elinit);

    return os;
}

void planOpenListDel(plan_open_list_t *os)
{
    if (os->heap)
        borPairHeapDel(os->heap);
    BOR_FREE(os);
}



static int heapLessThan(const bor_pairheap_node_t *n1,
                        const bor_pairheap_node_t *n2,
                        void *data)
{
    plan_open_list_node_t *el1 = bor_container_of(n1, plan_open_list_node_t, heap);
    plan_open_list_node_t *el2 = bor_container_of(n2, plan_open_list_node_t, heap);
    return el1->heuristic < el2->heuristic;
}
