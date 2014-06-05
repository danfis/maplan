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
void planPathPrepend(plan_path_t *path, plan_operator_t *op);

/**
 * Prints path to the given file.
 */
void planPathPrint(const plan_path_t *path, FILE *fout);

/**
 * Returns a cost of a full path.
 */
unsigned planPathCost(const plan_path_t *path);

#endif /* __PLAN_PATH_H__ */
