#ifndef __PLAN_FACT_OP_CROSS_REF_H__
#define __PLAN_FACT_OP_CROSS_REF_H__

#include "plan/problem.h"
#include "fact_id.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_oparr_t {
    int *op;  /*!< List of operators' IDs */
    int size; /*!< Size of array */
};
typedef struct _plan_oparr_t plan_oparr_t;

struct _plan_factarr_t {
    int *fact; /*!< List of facts' IDs */
    int size;  /*!< Size of array */
};
typedef struct _plan_factarr_t plan_factarr_t;

struct _plan_fake_pre_t {
    int fact_id;
    plan_cost_t value;
};
typedef struct _plan_fake_pre_t plan_fake_pre_t;

struct _plan_fact_op_cross_ref_t {
    plan_fact_id_t fact_id; /*!< Translation from var-val pair to fact ID */

    int fact_size;          /*!< Number of facts -- value from .fact_id */
    int op_size;            /*!< Number of operators */
    plan_oparr_t *fact_pre; /*!< Operators for which the corresponding fact
                                 is precondition -- the size of this array
                                 equals to .fact_size */
    plan_oparr_t *fact_eff; /*!< Operators for which the corresponding fact
                                 is effect. This array is created only in
                                 case of lm-cut heuristic. */
    plan_factarr_t *op_pre; /*!< Precondition facts of operators. Size of
                                 this array is .op_size */
    plan_factarr_t *op_eff; /*!< Unrolled effects of operators. Size of
                                 this array equals to .op_size */
    int goal_id;            /*!< Fact ID of the artificial goal */
    int goal_op_id;         /*!< ID of artificial operator that has
                                 .goal_id as an effect */
    plan_fake_pre_t *fake_pre; /*!< Artificial precondition facts */
    int fake_pre_size;      /*!< Number of precondition facts */
    int *op_id;             /*!< Returns original operator ID. This is here
                                 mainly because of the conditional effects */
};
typedef struct _plan_fact_op_cross_ref_t plan_fact_op_cross_ref_t;


/**
 * Initialize cross reference table
 */
void planFactOpCrossRefInit(plan_fact_op_cross_ref_t *cr,
                            const plan_var_t *var, int var_size,
                            const plan_part_state_t *goal,
                            const plan_op_t *op, int op_size);

/**
 * Frees allocated resources.
 */
void planFactOpCrossRefFree(plan_fact_op_cross_ref_t *cr);

#define PLAN_OPARR_FOR_EACH(oparr, i, op_id) \
    for ((i) = 0; (i) < (size) && ((op_id) = (oparr)->op[(i)], 1); ++(i))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_FACT_OP_CROSS_REF_H__ */
