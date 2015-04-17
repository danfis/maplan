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

#ifndef __PLAN_FACT_ID_H__
#define __PLAN_FACT_ID_H__

#include <plan/var.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Structure for translation of variable's value to fact id.
 */
struct _plan_fact_id_t {
    int **fact_id;
    int fact_size;
    int var_size;
};
typedef struct _plan_fact_id_t plan_fact_id_t;

/**
 * Initializes struct for translating variable value to fact ID
 */
void planFactIdInit(plan_fact_id_t *fid, const plan_var_t *var, int var_size);

/**
 * Frees val_to_id_t resources
 */
void planFactIdFree(plan_fact_id_t *factid);

/**
 * Translates variable value to fact ID
 */
_bor_inline int planFactId(const plan_fact_id_t *factid,
                           plan_var_id_t var, plan_val_t val);

/**** INLINES ****/

_bor_inline int planFactId(const plan_fact_id_t *fid,
                           plan_var_id_t var, plan_val_t val)
{
    if (fid->fact_id[var] != NULL)
        return fid->fact_id[var][val];
    return -1;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_FACT_ID_H__ */
