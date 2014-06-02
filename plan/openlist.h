#ifndef __PLAN_OPENLIST_H__
#define __PLAN_OPENLIST_H__

#include <boruvka/pairheap.h>

#include <plan/state.h>

struct _plan_open_list_t {
    bor_pairheap_t *heap;
    plan_state_pool_t *state_pool;
    size_t data_id;
};
typedef struct _plan_open_list_t plan_open_list_t;

/**
 * Creates a new open list that uses provided state_pool.
 */
plan_open_list_t *planOpenListNew(plan_state_pool_t *state_pool);

/**
 * Frees all allocated resources of the open list.
 */
void planOpenListDel(plan_open_list_t *os);

#endif /* __PLAN_OPENLIST_H__ */
