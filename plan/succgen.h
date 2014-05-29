#ifndef __PLAN_SUCCGEN_H__
#define __PLAN_SUCCGEN_H__

#include <plan/operator.h>

struct _plan_succ_gen_tree_t;

/**
 * Successor generator.
 */
struct _plan_succ_gen_t {
    struct _plan_succ_gen_tree_t *root; /*!< Root of decision tree */
    size_t num_operators;
};
typedef struct _plan_succ_gen_t plan_succ_gen_t;


/**
 * Creates a new successor generator for the given operators.
 */
plan_succ_gen_t *planSuccGenNew(plan_operator_t *op, size_t opsize);

/**
 * Deletes successor generator.
 */
void planSuccGenDel(plan_succ_gen_t *sg);

/**
 * Returns number of operators stored in successor generator.
 */
_bor_inline size_t planSuccGenNumOperators(const plan_succ_gen_t *sg);

/**
 * Finds all applicable operators to the given state.
 * The operators are written to the output array {op} up to its size.
 * The overall number of found operators is returned.
 */
size_t planSuccGenFind(const plan_succ_gen_t *sg,
                       const plan_state_t *state,
                       plan_operator_t **op, size_t op_size);


void planSuccGenDump(const struct _plan_succ_gen_tree_t *tree,
                     size_t prefix, FILE *fout);

/**** INLINES ****/
_bor_inline size_t planSuccGenNumOperators(const plan_succ_gen_t *sg)
{
    return sg->num_operators;
}

#endif /* __PLAN_SUCCGEN_H__ */
