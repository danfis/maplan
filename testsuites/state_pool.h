#ifndef _STATE_POOL_H_
#define _STATE_POOL_H_

#include <plan/state.h>

struct _state_pool_t {
    int *val;
    int val_size;
    int cur;
};
typedef struct _state_pool_t state_pool_t;

void statePoolInit(state_pool_t *pool, const char *fn);
void statePoolFree(state_pool_t *pool);
int statePoolNext(state_pool_t *pool, plan_state_t *state);
void statePoolReset(state_pool_t *pool);

#endif /* _STATE_POOL_H_ */
