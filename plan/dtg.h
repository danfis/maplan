#ifndef __PLAN_DTG_H__
#define __PLAN_DTG_H__

#include <plan/var.h>
#include <plan/op.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Srtruct representing transition between pair of values.
 */
struct _dtg_trans_t {
    const plan_op_t **ops;
    int ops_size;
};
typedef struct _dtg_trans_t dtg_trans_t;

/**
 * Domain transition graph for one variable.
 */
struct _dtg_var_t {
    plan_var_id_t var;   /*!< ID of the variable */
    plan_val_t val_size; /*!< Number of values */
    dtg_trans_t *trans;  /*!< Incidence matrix containing transition
                              operators for each pair of values */
};
typedef struct _dtg_var_t dtg_var_t;

/**
 * Domain transition graph
 */
struct _dtg_t {
    int var_size;   /*!< Number of variables */
    dtg_var_t *dtg; /*!< DTG for each variable */
};
typedef struct _dtg_t dtg_t;

/**
 * Constructs domain transition graph for all variables.
 */
void planDTGInit(dtg_t *dtg, const plan_var_t *var, int var_size,
                 const plan_op_t *op, int op_size);

/**
 * Frees allocated resources.
 */
void planDTGFree(dtg_t *dtg);

/**
 * For debuf purposes
 */
void planDTGPrint(const dtg_t *dtg, FILE *fout);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_DTG_H__ */
