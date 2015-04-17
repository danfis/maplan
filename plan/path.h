/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

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
    int global_id;
    int owner;
    plan_state_id_t from_state;
    plan_state_id_t to_state;
    int timestamp;
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
 * Copy a factored version of path into dst.
 */
void planPathCopyFactored(plan_path_t *dst, const plan_path_t *src,
                          int agent_id);

/**
 * Prepends an operator into path.
 */
void planPathPrepend(plan_path_t *path, const char *name,
                     plan_cost_t cost, int global_id, int owner,
                     plan_state_id_t from, plan_state_id_t to);
void planPathPrependOp(plan_path_t *path, plan_op_t *op,
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
 * Number of operators in path.
 */
int planPathLen(const plan_path_t *path);

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
