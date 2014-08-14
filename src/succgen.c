#include <boruvka/alloc.h>
#include "plan/succgen.h"

#ifndef _GNU_SOURCE
/** Declaration of qsort_r() function that should be available in libc */
void qsort_r(void *base, size_t nmemb, size_t size,
             int (*compar)(const void *, const void *, void *),
             void *arg);
#endif

/**
 * Base building structure for a tree node of successor generator.
 */
struct _plan_succ_gen_tree_t {
    plan_var_id_t var;                  /*!< Decision variable */
    plan_operator_t **ops;              /*!< List of immediate operators
                                             that are returned once this
                                             node is reached */
    int ops_size;
    struct _plan_succ_gen_tree_t **val; /*!< Subtrees indexed by the value
                                             of the decision variable */
    int val_size;
    struct _plan_succ_gen_tree_t *def;  /*!< Default subtree containing
                                             operators without precondition
                                             on the decision variable */
};
typedef struct _plan_succ_gen_tree_t plan_succ_gen_tree_t;

struct _var_order_t {
    int *var;
    int var_size;
};
typedef struct _var_order_t var_order_t;

/** Creates a new tree node (and recursively all subtrees) */
static plan_succ_gen_tree_t *treeNew(plan_operator_t **ops, int len,
                                     const plan_var_id_t *var);

/** Creates a new tree from the FD definition */
static plan_succ_gen_tree_t *treeFromFD(FILE *fin,
                                        const plan_var_t *vars,
                                        plan_operator_t *ops,
                                        int *num_ops);

/** Recursively deletes a tree */
static void treeDel(plan_succ_gen_tree_t *tree);

/** Finds applicable operators to the given state */
static int treeFind(const plan_succ_gen_tree_t *tree,
                    plan_val_t *vals,
                    plan_operator_t **op, int op_size);

/** Set immediate operators to tree node */
static void treeBuildSetOps(plan_succ_gen_tree_t *tree,
                            plan_operator_t **ops, int len);
/** Build default subtree */
static int treeBuildDef(plan_succ_gen_tree_t *tree,
                        plan_operator_t **ops, int len,
                        const plan_var_id_t *var);
/** Prepare array of .val[] subtrees */
static void treeBuildPrepareVal(plan_succ_gen_tree_t *tree,
                                plan_val_t val);
/** Build val subtree */
static int treeBuildVal(plan_succ_gen_tree_t *tree,
                        plan_operator_t **ops, int len,
                        const plan_var_id_t *var);

/** Comparator for qsort which sorts operators by its variables and its
 *  values. */
static int opsSortCmp(const void *a, const void *b, void *data);




plan_succ_gen_t *planSuccGenNew(const plan_operator_t *op, int opsize,
                                const plan_var_id_t *var_order)
{
    plan_succ_gen_t *sg;
    plan_operator_t **sorted_ops = NULL;
    plan_var_id_t *_var_order;
    int i;

    if (opsize > 0){
        if (var_order == NULL){
            int size = planPartStateSize(op[0].eff);
            _var_order = alloca(sizeof(plan_var_id_t) * (size + 1));
            for (i = 0; i < size; ++i)
                _var_order[i] = i;
            _var_order[size] = PLAN_VAR_ID_UNDEFINED;
            var_order = _var_order;
        }

        // prepare array for sorting operators
        sorted_ops = BOR_ALLOC_ARR(plan_operator_t *, opsize);
        for (i = 0; i < opsize; ++i)
            sorted_ops[i] = (plan_operator_t *)(op + i);

        // Sort operators by values of preconditions.
        qsort_r(sorted_ops, opsize, sizeof(plan_operator_t *),
                opsSortCmp, (void *)var_order);
    }

    sg = BOR_ALLOC(plan_succ_gen_t);
    sg->root = treeNew(sorted_ops, opsize, var_order);
    sg->num_operators = opsize;

    if (sorted_ops)
        BOR_FREE(sorted_ops);
    return sg;
}

plan_succ_gen_t *planSuccGenFromFD(FILE *fin,
                                   const plan_var_t *vars,
                                   plan_operator_t *op)
{
    plan_succ_gen_t *sg;
    sg = BOR_ALLOC(plan_succ_gen_t);
    sg->num_operators = 0;
    sg->root = treeFromFD(fin, vars, op, &sg->num_operators);
    return sg;
}

void planSuccGenDel(plan_succ_gen_t *sg)
{
    if (sg->root)
        treeDel(sg->root);

    BOR_FREE(sg);
}

int planSuccGenFind(const plan_succ_gen_t *sg,
                    const plan_state_t *state,
                    plan_operator_t **op, int op_size)
{
    // Use variable-length array to speed it up.
    plan_val_t vals[state->num_vars];
    memcpy(vals, state->val, sizeof(plan_val_t) * state->num_vars);
    return treeFind(sg->root, vals, op, op_size);
}

int planSuccGenFindPart(const plan_succ_gen_t *sg,
                        const plan_part_state_t *part_state,
                        plan_operator_t **op, int op_size)
{
    plan_val_t vals[part_state->num_vars];
    memcpy(vals, part_state->val, sizeof(plan_val_t) * part_state->num_vars);
    return treeFind(sg->root, vals, op, op_size);
}






static int opsSortCmp(const void *a, const void *b, void *_var_order)
{
    const plan_operator_t *opa = *(const plan_operator_t **)a;
    const plan_operator_t *opb = *(const plan_operator_t **)b;
    const plan_var_id_t *var = _var_order;
    int aset, bset;
    plan_val_t aval, bval;

    for (; *var != PLAN_VAR_ID_UNDEFINED; ++var){
        aset = planPartStateIsSet(opa->pre, *var);
        bset = planPartStateIsSet(opb->pre, *var);

        if (aset && bset){
            aval = planPartStateGet(opa->pre, *var);
            bval = planPartStateGet(opb->pre, *var);

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

    // make the sort stable
    if (opa < opb)
        return -1;
    return 1;
}

static void treeBuildSetOps(plan_succ_gen_tree_t *tree,
                            plan_operator_t **ops, int len)
{
    int i;

    if (tree->ops)
        BOR_FREE(tree->ops);

    tree->ops_size = len;
    tree->ops = BOR_ALLOC_ARR(plan_operator_t *, tree->ops_size);

    for (i = 0; i < len; ++i)
        tree->ops[i] = ops[i];
}

static int treeBuildDef(plan_succ_gen_tree_t *tree,
                        plan_operator_t **ops, int len,
                        const plan_var_id_t *var)
{
    int size;

    for (size = 1;
         size < len && !planPartStateIsSet(ops[size]->pre, *var);
         ++size);

    tree->var = *var;
    tree->def = treeNew(ops, size, var + 1);

    return size;
}

static void treeBuildPrepareVal(plan_succ_gen_tree_t *tree,
                                plan_val_t val)
{
    int i;

    tree->val_size = val + 1;
    tree->val = BOR_ALLOC_ARR(plan_succ_gen_tree_t *, tree->val_size);

    for (i = 0; i < tree->val_size; ++i)
        tree->val[i] = NULL;
}

static int treeBuildVal(plan_succ_gen_tree_t *tree,
                        plan_operator_t **ops, int len,
                        const plan_var_id_t *var)
{
    int size;
    plan_val_t val;

    val = planPartStateGet(ops[0]->pre, *var);

    for (size = 1;
         size < len && planPartStateGet(ops[size]->pre, *var) == val;
         ++size);

    tree->var = *var;
    tree->val[val] = treeNew(ops, size, var + 1);

    return size;
}

static plan_succ_gen_tree_t *treeNew(plan_operator_t **ops, int len,
                                     const plan_var_id_t *var)
{
    plan_succ_gen_tree_t *tree;
    int start;

    tree = BOR_ALLOC(plan_succ_gen_tree_t);
    tree->var = PLAN_VAR_ID_UNDEFINED;
    tree->ops = NULL;
    tree->ops_size = 0;
    tree->val = NULL;
    tree->val_size = 0;
    tree->def = NULL;

    if (len == 0)
        return tree;

    // Find first variable that is set for at least one operator.
    // The operators are sorted so that it is enough to check the last
    // operator in the array.
    for (; *var != PLAN_VAR_ID_UNDEFINED; ++var){
        if (planPartStateIsSet(ops[len - 1]->pre, *var))
            break;
    }

    if (*var == PLAN_VAR_ID_UNDEFINED){
        // If there isn't any operator with set value anymore insert all
        // operators as immediate ops and exit.
        treeBuildSetOps(tree, ops, len);
        return tree;
    }

    // Now we now that array of operators contain at least one operator
    // with set value of current variable.

    // Prepare val array -- we now that the last operator in array has
    // largest value.
    treeBuildPrepareVal(tree, planPartStateGet(ops[len - 1]->pre, *var));

    // Initialize index of the first element with current value
    start = 0;

    // First check unset values from the beggining of the array
    if (!planPartStateIsSet(ops[0]->pre, *var)){
        start = treeBuildDef(tree, ops, len, var);
    }

    // Then build subtree for each value
    while (start < len){
        start += treeBuildVal(tree, ops + start, len - start, var);
    }

    return tree;
}

static void treeFromFDOps(plan_succ_gen_tree_t *tree, FILE *fin,
                          plan_operator_t *ops, int *num_ops_out)
{
    int i, num_ops, op_idx;

    if (fscanf(fin, "%d", &num_ops) != 1){
        fprintf(stderr, "Error: Invalid successor generator definition.\n");
        return;
    }

    if (num_ops == 0)
        return;

    *num_ops_out += num_ops;
    tree->ops_size = num_ops;
    tree->ops = BOR_ALLOC_ARR(plan_operator_t *, tree->ops_size);
    for (i = 0; i < num_ops; ++i){
        if (fscanf(fin, "%d", &op_idx) != 1){
            fprintf(stderr, "Error: Invalid successor generator definition.\n");
            return;
        }
        tree->ops[i] = ops + op_idx;
    }
}

static plan_succ_gen_tree_t *treeFromFD(FILE *fin,
                                        const plan_var_t *vars,
                                        plan_operator_t *ops,
                                        int *num_ops_out)
{
    plan_succ_gen_tree_t *tree;
    char type[128];
    int i, var;

    if (fscanf(fin, "%s", type) != 1){
        fprintf(stderr, "Error: Could not determine type\n");
        return NULL;
    }

    tree = BOR_ALLOC(plan_succ_gen_tree_t);
    tree->var = PLAN_VAR_ID_UNDEFINED;
    tree->ops = NULL;
    tree->ops_size = 0;
    tree->val = NULL;
    tree->val_size = 0;
    tree->def = NULL;

    if (strcmp(type, "switch") == 0){
        if (fscanf(fin, "%d", &var) != 1){
            fprintf(stderr, "Error: Invalid successor generator definition.\n");
            return NULL;
        }
        tree->var = var;

        if (fscanf(fin, "%s", type) != 1){
            fprintf(stderr, "Error: Invalid successor generator definition.\n");
            return NULL;
        }
        if (strcmp(type, "check") != 0){
            fprintf(stderr, "Error: Invalid successor generator definition."
                            " Expecting 'check'\n");
            return NULL;
        }
        treeFromFDOps(tree, fin, ops, num_ops_out);

        tree->val_size = vars[var].range;
        tree->val = BOR_CALLOC_ARR(plan_succ_gen_tree_t *, tree->val_size);
        for (i = 0; i < vars[var].range; ++i){
            tree->val[i] = treeFromFD(fin, vars, ops, num_ops_out);
        }

        tree->def = treeFromFD(fin, vars, ops, num_ops_out);

    }else if (strcmp(type, "check") == 0){
        treeFromFDOps(tree, fin, ops, num_ops_out);
        if (tree->ops_size == 0){
            BOR_FREE(tree);
            return NULL;
        }

    }else{
        fprintf(stderr, "Error: Unknown type: `%s'\n", type);
        return NULL;
    }

    return tree;
}

static void treeDel(plan_succ_gen_tree_t *tree)
{
    int i;

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

static int treeFind(const plan_succ_gen_tree_t *tree,
                    plan_val_t *vals,
                    plan_operator_t **op, int op_size)
{
    int i, found = 0, size;
    plan_val_t val;

    // insert all immediate operators
    if (tree->ops_size > 0){
        for (i = 0; i < tree->ops_size; ++i){
            if (op_size > i)
                op[i] = tree->ops[i];
        }
        found = tree->ops_size;
    }

    // check whether this node should check on any variable value
    if (tree->var != PLAN_VAR_ID_UNDEFINED){
        // get corresponding value from state
        val = vals[tree->var];

        // and use tree corresponding to the value if present
        if (val != PLAN_VAL_UNDEFINED && val < tree->val_size && tree->val[val]){
            size = BOR_MAX(0, (int)op_size - (int)found);
            found += treeFind(tree->val[val], vals, op + found, size);
        }

        // use default tree if present
        if (tree->def){
            size = BOR_MAX(0, (int)op_size - (int)found);
            found += treeFind(tree->def, vals, op + found, size);
        }
    }

    return found;
}
