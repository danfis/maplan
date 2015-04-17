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

#ifndef __PLAN_OP_ID_TR_H__
#define __PLAN_OP_ID_TR_H__

#include <plan/op.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Translation between local operator ID and global ID.
 */
struct _plan_op_id_tr_t {
    int *loc_to_glob;
    int loc_to_glob_size;
    int *glob_to_loc;
    int glob_to_loc_size;
};
typedef struct _plan_op_id_tr_t plan_op_id_tr_t;

/**
 * Initialize translator.
 */
void planOpIdTrInit(plan_op_id_tr_t *tr, const plan_op_t *op, int op_size);

/**
 * Frees translator.
 */
void planOpIdTrFree(plan_op_id_tr_t *tr);

/**
 * Translates global ID to local ID.
 */
_bor_inline int planOpIdTrLoc(const plan_op_id_tr_t *tr, int glob_id);

/**
 * Translates local ID to global ID.
 */
_bor_inline int planOpIdTrGlob(const plan_op_id_tr_t *tr, int loc_id);


/**** INLINES: ****/
_bor_inline int planOpIdTrLoc(const plan_op_id_tr_t *tr, int glob_id)
{
    if (glob_id >= tr->glob_to_loc_size)
        return -1;
    return tr->glob_to_loc[glob_id];
}

_bor_inline int planOpIdTrGlob(const plan_op_id_tr_t *tr, int loc_id)
{
    if (loc_id >= tr->loc_to_glob_size)
        return -1;
    return tr->loc_to_glob[loc_id];
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_OP_ID_TR_H__ */
