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
#include <plan/state.h>
#include <plan/part_state.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PLAN_FACT_ID_H2 0x1u
#define PLAN_FACT_ID_REVERSE_MAP 0x2u

/**
 * Structure for translation of variable's value to fact id.
 */
struct _plan_fact_id_t {
    unsigned flags;
    int **var; /*!< [var][val] -> fact_id */
    int var_size;
    int fact_size;
    int *fact_offset;
    int fact1_size;

    /** Mapping from fact to var/val */
    plan_var_id_t *fact_to_var;
    plan_val_t *fact_to_val;

    int *state_buf;
    int *part_state_buf;
};
typedef struct _plan_fact_id_t plan_fact_id_t;

#define PLAN_FACT_ID_FOR_EACH_STATE(fid, state, fact_id) \
    for (int ___i = 0, ___size = __planFactIdState((fid), (state)); \
            ___i < ___size && ((fact_id) = (fid)->state_buf[___i]) >= 0; \
            ++___i)

#define PLAN_FACT_ID_FOR_EACH_PART_STATE(fid, pstate, fact_id) \
    for (int ___i = 0, ___size = __planFactIdPartState((fid), (pstate)); \
            ___i < ___size && ((fact_id) = (fid)->part_state_buf[___i]) >= 0; \
            ++___i)

/**
 * Initializes struct for translating variable value to fact ID
 */
void planFactIdInit(plan_fact_id_t *fid, const plan_var_t *var, int var_size,
                    unsigned flags);

/**
 * Frees val_to_id_t resources
 */
void planFactIdFree(plan_fact_id_t *factid);

/**
 * Translates variable value to fact ID
 */
_bor_inline int planFactIdVar(const plan_fact_id_t *factid,
                              plan_var_id_t var, plan_val_t val);

/**
 * Returns ID corresponding to the combination of two facts.
 * It is always assumed that var1 and var2 differ and that the object was
 * initialized with PLAN_FACT_ID_H2 flag.
 */
_bor_inline int planFactIdVar2(const plan_fact_id_t *f,
                               plan_var_id_t var1, plan_val_t val1,
                               plan_var_id_t var2, plan_val_t val2);

/**
 * Returns ID corresponding to the combination of two facts.
 * It is always assumed that fid1 and fid2 are IDs of unary facts and they
 * does not belong to the same variable. It is also assumed that
 * PLAN_FACT_ID_H2 flag was used for initialization.
 */
_bor_inline int planFactIdFact2(const plan_fact_id_t *f,
                                int fid1, int fid2);


/**
 * Takes fact ID and converts it back to a pair of facts.
 */
void planFactIdFromFactId(const plan_fact_id_t *f, int fid,
                          int *fid1, int *fid2);

/**
 * Returns a list of fact IDs corresponding to the state.
 * The returned array always begins with state->size IDs that correspond to
 * unary facts.
 * Returns pointer to the internal buffer that is rewritten after each call
 * of this function!.
 */
_bor_inline const int *planFactIdState(const plan_fact_id_t *f,
                                       const plan_state_t *state,
                                       int *size);

/**
 * Same as planFactIdState() but a new array is returned.
 */
int *planFactIdState2(const plan_fact_id_t *f, const plan_state_t *state,
                      int *size);

/**
 * Same as planFactIdState() but translates a partial state and first
 * part_state->vals_size IDs correspond to unary facts.
 */
_bor_inline const int *planFactIdPartState(const plan_fact_id_t *f,
                                           const plan_part_state_t *part_state,
                                           int *size);

/**
 * Same as planFactIdPartState() but a new array is returned.
 */
int *planFactIdPartState2(const plan_fact_id_t *f,
                          const plan_part_state_t *part_state,
                          int *size);

/**
 * Returns var/val value corresponding to the fact ID.
 * Works only if PLAN_FACT_ID_REVERSE_MAP is enabled during initialization.
 */
_bor_inline void planFactIdVarFromFact(const plan_fact_id_t *f,
                                       int fact_id,
                                       plan_var_id_t *var,
                                       plan_val_t *val);

int __planFactIdState(const plan_fact_id_t *f, const plan_state_t *state);
int __planFactIdPartState(const plan_fact_id_t *f,
                          const plan_part_state_t *part_state);
/**** INLINES ****/

_bor_inline int planFactIdVar(const plan_fact_id_t *fid,
                              plan_var_id_t var, plan_val_t val)
{
    if (fid->var[var] != NULL)
        return fid->var[var][val];
    return -1;
}

_bor_inline int planFactIdVar2(const plan_fact_id_t *f,
                               plan_var_id_t var1, plan_val_t val1,
                               plan_var_id_t var2, plan_val_t val2)
{
    if (var1 == var2 && val1 == val2)
        return planFactIdVar(f, var1, val1);
    return planFactIdFact2(f, planFactIdVar(f, var1, val1),
                              planFactIdVar(f, var2, val2));
}

_bor_inline int planFactIdFact2(const plan_fact_id_t *f,
                                int fid1, int fid2)
{
    if (fid1 < fid2){
        return fid2 + f->fact_offset[fid1];
    }else{
        return fid1 + f->fact_offset[fid2];
    }
}

_bor_inline const int *planFactIdState(const plan_fact_id_t *f,
                                       const plan_state_t *state,
                                       int *size)
{
    *size = __planFactIdState(f, state);
    return f->state_buf;
}

_bor_inline const int *planFactIdPartState(const plan_fact_id_t *f,
                                           const plan_part_state_t *part_state,
                                           int *size)
{
    *size = __planFactIdPartState(f, part_state);
    return f->part_state_buf;
}

_bor_inline void planFactIdVarFromFact(const plan_fact_id_t *f,
                                       int fact_id,
                                       plan_var_id_t *var,
                                       plan_val_t *val)
{
    *var = f->fact_to_var[fact_id];
    *val = f->fact_to_val[fact_id];
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_FACT_ID_H__ */
