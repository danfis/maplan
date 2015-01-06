#ifndef __PLAN_SUCCGEN_H__
#define __PLAN_SUCCGEN_H__

#include <plan/op.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_succ_gen_tree_t;

/**
 * Successor generator.
 */
struct _plan_succ_gen_t {
    struct _plan_succ_gen_tree_t *root; /*!< Root of decision tree */
    int num_operators;
};
typedef struct _plan_succ_gen_t plan_succ_gen_t;


/**
 * Creates a new successor generator for the given operators.
 * If optional argument var_order is specified, the given order of
 * variables is used (and only variables stated there are used). The given
 * array must end with PLAN_VAR_ID_UNDEFINED.
 */
plan_succ_gen_t *planSuccGenNew(const plan_op_t *op, int opsize,
                                const plan_var_id_t *var_order);

/**
 * Deletes successor generator.
 */
void planSuccGenDel(plan_succ_gen_t *sg);

/**
 * Returns number of operators stored in successor generator.
 */
_bor_inline int planSuccGenNumOperators(const plan_succ_gen_t *sg);

/**
 * Finds all applicable operators to the given state.
 * The operators are written to the output array {op} up to its size.
 * The overall number of found operators is returned.
 */
int planSuccGenFind(const plan_succ_gen_t *sg,
                    const plan_state_t *state,
                    plan_op_t **op, int op_size);

/**
 * Same as planSuccGenFind() but uses partial state instead of full state.
 */
int planSuccGenFindPart(const plan_succ_gen_t *sg,
                        const plan_part_state_t *part_state,
                        plan_op_t **op, int op_size);


/**** INLINES ****/
_bor_inline int planSuccGenNumOperators(const plan_succ_gen_t *sg)
{
    return sg->num_operators;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_SUCCGEN_H__ */
