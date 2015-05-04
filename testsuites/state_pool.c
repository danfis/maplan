#include <stdio.h>
#include <boruvka/alloc.h>
#include "state_pool.h"

void statePoolInit(state_pool_t *pool, const char *fn)
{
    FILE *fin;
    int val, alloc_size;

    fin = fopen(fn, "r");
    if (fin == NULL){
        fprintf(stderr, "Error: Could not read states from `%s'\n", fn);
        exit(-1);
    }

    pool->val_size = 0;
    alloc_size = 200;
    pool->val = BOR_ALLOC_ARR(int, alloc_size);
    pool->cur = 0;

    while (fscanf(fin, "%d", &val) == 1){
        if (pool->val_size == alloc_size){
            alloc_size *= 2;
            pool->val = BOR_REALLOC_ARR(pool->val, int, alloc_size);
        }
        pool->val[pool->val_size++] = val;
    }

    fclose(fin);

    pool->val = BOR_REALLOC_ARR(pool->val, int, pool->val_size);
}

void statePoolFree(state_pool_t *pool)
{
    BOR_FREE(pool->val);
}

int statePoolNext(state_pool_t *pool, plan_state_t *state)
{
    int state_size = planStateSize(state);
    int i;

    if (pool->cur + state_size > pool->val_size)
        return -1;

    for (i = 0; i < state_size; ++i)
        planStateSet(state, i, pool->val[pool->cur++]);

    return 0;
}

void statePoolReset(state_pool_t *pool)
{
    pool->cur = 0;
}
