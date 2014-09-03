#ifndef __PLAN_PATH_H__
#define __PLAN_PATH_H__

#include <boruvka/list.h>
#include <plan/op.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Structure holding the path from an initial state to a goal.
 */
typedef bor_list_t plan_path_t;

/**
 * One step in path.
 */
struct _plan_path_op_t {
    char *name;
    plan_cost_t cost;
    plan_op_t *op;
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
 * Copies path from src to dst.
 */
void planPathCopy(plan_path_t *dst, const plan_path_t *src);

/**
 * Prepends an operator into path.
 */
void planPathPrepend(plan_path_t *path, plan_op_t *op,
                     plan_state_id_t from, plan_state_id_t to);
void planPathPrepend2(plan_path_t *path, const char *op_name,
                      plan_cost_t op_cost);

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

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PATH_H__ */
