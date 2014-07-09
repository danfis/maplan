#ifndef __PLAN_PATH_H__
#define __PLAN_PATH_H__

#include <boruvka/list.h>
#include <plan/operator.h>

/**
 * Structure holding the path from an initial state to a goal.
 */
typedef bor_list_t plan_path_t;

/**
 * One step in path.
 */
struct _plan_path_op_t {
    plan_operator_t *op;
    plan_state_id_t from_state;
    plan_state_id_t to_state;
    bor_list_t path;
};
typedef struct _plan_path_op_t plan_path_op_t;

/**
 * Initializes an empty path.
 */
void planPathInit(plan_path_t *path);

/**
 * Frees all resources connected with the path.
 */
void planPathFree(plan_path_t *path);

/**
 * Prepends an operator into path.
 */
void planPathPrepend(plan_path_t *path, plan_operator_t *op,
                     plan_state_id_t from, plan_state_id_t to);

/**
 * Prints path to the given file.
 */
void planPathPrint(const plan_path_t *path, FILE *fout);

/**
 * Returns a cost of a full path.
 */
plan_cost_t planPathCost(const plan_path_t *path);

/**
 * Returns true if the path is empty.
 */
_bor_inline int planPathEmpty(const plan_path_t *path);

/**
 * Returns first operator in the path.
 */
_bor_inline plan_path_op_t *planPathFirstOp(plan_path_t *path);


/**** INLINES: ****/
_bor_inline int planPathEmpty(const plan_path_t *path)
{
    return borListEmpty(path);
}

_bor_inline plan_path_op_t *planPathFirstOp(plan_path_t *path)
{
    return BOR_LIST_ENTRY(borListNext(path), plan_path_op_t, path);
}
#endif /* __PLAN_PATH_H__ */
