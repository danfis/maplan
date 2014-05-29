#include <boruvka/alloc.h>
#include "plan/succgen.h"

struct _plan_succ_gen_tree_t {
    unsigned var;                       /*!< Decision variable */
    plan_operator_t **ops;              /*!< List of immediate operators
                                             that are returned once this
                                             node is reached */
    size_t ops_size;
    struct _plan_succ_gen_tree_t **val; /*!< Subtrees indexed by the value
                                             of the decision variable */
    size_t val_size;
    struct _plan_succ_gen_tree_t *def;  /*!< Default subtree containing
                                             operators without precondition
                                             on the decision variable */
};
typedef struct _plan_succ_gen_tree_t plan_succ_gen_tree_t;

static plan_succ_gen_tree_t *treeNew(unsigned start_var,
                                     plan_operator_t **ops,
                                     size_t len);
static void treeDel(plan_succ_gen_tree_t *tree);

static int opsSortCmp(const void *a, const void *b)
{
    const plan_operator_t *opa = *(const plan_operator_t **)a;
    const plan_operator_t *opb = *(const plan_operator_t **)b;
    size_t i, size = planPartStateSize(opa->pre);
    int aset, bset;
    unsigned aval, bval;

    for (i = 0; i < size; ++i){
        aset = planPartStateIsSet(opa->pre, i);
        bset = planPartStateIsSet(opb->pre, i);

        if (aset && bset){
            aval = planPartStateGet(opa->pre, i);
            bval = planPartStateGet(opb->pre, i);

            if (aval < bval){
                return -1;
            }else if (aval > bval){
                return 1;
            }
        }else if (!aset && bset){
            return -1;
        }else if (aset && !bset){
            return 1;
        }
    }

    return 0;
}

plan_succ_gen_t *planSuccGenNew(plan_operator_t *op, size_t opsize)
{
    plan_succ_gen_t *sg;
    plan_operator_t **sorted_ops;
    size_t i;

    // prepare array for sorting operators
    sorted_ops = BOR_ALLOC_ARR(plan_operator_t *, opsize);
    for (i = 0; i < opsize; ++i)
        sorted_ops[i] = op + i;

    // sort operators by its preconditions
    qsort(sorted_ops, opsize, sizeof(plan_operator_t *), opsSortCmp);

    size_t j;
    for (j = 0; j < op[0].pre->num_vars; ++j){
        for (i = 0; i < opsize; ++i){
            if (planPartStateIsSet(sorted_ops[i]->pre, j)){
                fprintf(stderr, " % 3d", (int)planPartStateGet(sorted_ops[i]->pre, j));
            }else{
                fprintf(stderr, " % 3d", -1);
            }
        }
        fprintf(stderr, "\n");
    }


    sg = BOR_ALLOC(plan_succ_gen_t);
    sg->root = treeNew(0, sorted_ops, opsize);
    sg->num_operators = opsize;

    BOR_FREE(sorted_ops);
    return sg;
}

void planSuccGenDel(plan_succ_gen_t *sg)
{
    if (sg->root)
        treeDel(sg->root);

    BOR_FREE(sg);
}

size_t planSuccGenFind(const plan_succ_gen_t *sg,
                       const plan_state_t *state,
                       plan_operator_t **op, size_t op_size)
{
    return 0;
}

void planSuccGenDump(const plan_succ_gen_tree_t *tree,
                     size_t prefix, FILE *fout)
{
    size_t i;

    for (i = 0; i < prefix; ++i)
        fprintf(fout, " ");
    fprintf(fout, "var: %u %lu\n", tree->var, tree->ops_size);

    if (tree->def == NULL){
        for (i = 0; i < prefix; ++i)
            fprintf(fout, " ");
        fprintf(fout, "  NULL\n");
    }else{
        planSuccGenDump(tree->def, prefix + 2, fout);
    }

    for (i = 0; i < tree->val_size; ++i){
        if (tree->val[i] == NULL){
            for (i = 0; i < prefix; ++i)
                fprintf(fout, " ");
            fprintf(fout, "  NULL\n");
        }else{
            planSuccGenDump(tree->val[i], prefix + 2, fout);
        }
    }
}


static void treeSetOps(plan_succ_gen_tree_t *tree,
                       plan_operator_t **ops, size_t len)
{
    size_t i;

    if (tree->ops)
        BOR_FREE(tree->ops);

    tree->ops_size = len;
    tree->ops = BOR_ALLOC_ARR(plan_operator_t *, tree->ops_size);

    for (i = 0; i < len; ++i)
        tree->ops[i] = ops[i];
}

static void treeAddValSubtree(plan_succ_gen_tree_t *tree,
                              unsigned var,
                              unsigned val,
                              plan_operator_t **ops,
                              size_t len)
{
    size_t i, oldsize;

    for (size_t i = 0; i < tree->val_size; ++i){
        fprintf(stderr, "PRE: %lx\n", (long)tree->val[i]);
    }

    // resize array of subtrees and set to NULL where not set
    oldsize = tree->val_size;
    tree->val = BOR_REALLOC_ARR(tree->val, plan_succ_gen_tree_t *, val + 1);
    tree->val_size = val + 1;
    for (i = oldsize; i < tree->val_size; ++i)
        tree->val[i] = NULL;

    for (size_t i = 0; i < tree->val_size; ++i){
        fprintf(stderr, "POST: %lx\n", (long)tree->val[i]);
    }

    // create a subtree
    tree->val[val] = treeNew(var + 1, ops, len);
    tree->var = var;
}

static plan_succ_gen_tree_t *treeNew(unsigned start_var,
                                     plan_operator_t **ops,
                                     size_t len)
{
    plan_succ_gen_tree_t *tree;
    unsigned var, num_vars, val, val2;
    size_t i, start;
    int found_split, set1, set2;
    fprintf(stderr, "NEW %u %lx %lu\n", start_var, (long)ops, len);

    tree = BOR_ALLOC(plan_succ_gen_tree_t);
    tree->var = -1;
    tree->ops = NULL;
    tree->ops_size = 0;
    tree->val = NULL;
    tree->val_size = 0;
    tree->def = NULL;

    num_vars = planPartStateSize(ops[0]->pre);
    for (var = start_var; var < num_vars; ++var){
        if (len == 1){
            fprintf(stderr, "  len == 1\n");
            if (planPartStateIsSet(ops[0]->pre, var)){
                val = planPartStateGet(ops[0]->pre, var);
                fprintf(stderr, "    val == %u\n", val);
                treeAddValSubtree(tree, var, val, ops, 1);
            }else{
                continue;
            }
        }

        found_split = 0;
        start = 0;
        for (i = 1; i < len; ++i){
            set1 = planPartStateIsSet(ops[i - 1]->pre, var);
            set2 = planPartStateIsSet(ops[i]->pre, var);
            val  = planPartStateGet(ops[i - 1]->pre, var);
            val2 = planPartStateGet(ops[i]->pre, var);
            fprintf(stderr, "var: %u[%lu], %u-%u %d %d\n", var, i, val,
                    val2, set1, set2);
            fflush(stderr);
            if (set1 != set2 || val != val2){
                found_split = 1;
                if (set1 && !set2){
                    fprintf(stderr, "ERRR\n");
                }

                if (!set1){
                    fprintf(stderr, "DEF %lu %lu\n", start, i - start);
                    tree->def = treeNew(var + 1, ops + start, i - start);
                    tree->var = var;
                }else{
                    fprintf(stderr, "SUB %u %lu %lu\n", val, start, i - start);
                    treeAddValSubtree(tree, var, val, ops + start, i - start);
                }

                start = i;
            }
        }

        if (found_split){
            fprintf(stderr, "found split\n");
            fflush(stderr);
            val = planPartStateGet(ops[len - 1]->pre, var);
            fprintf(stderr, "SUB2 %u %lu\n", val, start);
            treeAddValSubtree(tree, var, val, ops + start, len - start);
            fprintf(stderr, "XXX\n");
            fflush(stderr);
            return tree;
        }
        fprintf(stderr, "  [%u]found_split == %d\n", var, found_split);
    }

    treeSetOps(tree, ops, len);

    return tree;
}

static void treeDel(plan_succ_gen_tree_t *tree)
{
    size_t i;

    if (tree->ops)
        BOR_FREE(tree->ops);

    if (tree->val){
        for (i = 0; i < tree->val_size; ++i)
            if (tree->val[i])
                treeDel(tree->val[i]);
        BOR_FREE(tree->val);
    }

    if (tree->def)
        treeDel(tree->def);

    BOR_FREE(tree);
}
