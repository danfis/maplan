#ifndef __PLAN_PLAN_H__
#define __PLAN_PLAN_H__

#include <plan/var.h>
#include <plan/state.h>
#include <plan/operator.h>

struct _plan_t {
    plan_var_t *var;
    size_t var_size;
    plan_state_pool_t *state_pool;
    plan_state_t initial_state;
    plan_part_state_t goal;
    plan_operator_t *op;
    size_t op_size;
};
typedef struct _plan_t plan_t;


/**
 * Creates a new (empty) instance of a Fast Downward algorithm.
 */
plan_t *planNew(void);

/**
 * Deletes plan instance.
 */
void planDel(plan_t *plan);

/**
 * Loads definitions from specified file.
 */
int planLoadFromJsonFile(plan_t *plan, const char *filename);

#endif /* __PLAN_PLAN_H__ */
