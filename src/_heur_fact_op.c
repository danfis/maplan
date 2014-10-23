#include <boruvka/alloc.h>
#include "plan/problem.h"

/***
 * Internal Structures for Heuristic Algorithms
 * =============================================
 *
 * Available options for this module:
 * HEUR_FACT_OP_FACT_T
 *      Define as name of structure if you want to have .fact[] array
 *      available.
 * HEUR_FACT_OP_FACT_INIT(fact)
 *      Must be defined for initialization of HEUR_FACT_OP_FACT_T
 *      structure.
 * HEUR_FACT_OP_EXTRA_OP
 *      Defines extra members of plan_heur_op_t structure.
 * HEUR_FACT_OP_EXTRA_OP_INIT(dst, src, is_cond_eff)
 *      Initializes plan_heur_op_t structure -- this is called after
 *      initialization of fixed members plan_heur_op_t.
 * HEUR_FACT_OP_ARTIFICIAL_GOAL
 *      Define if you want to have defined artificial goal.
 * HEUR_FACT_OP_NO_PRE_FACT
 *      Artificical fact as precondition for operators w/o preconditions is
 *      created
 * HEUR_FACT_OP_FACT_EFF
 *      If defined .fact_eff[] will contain IDs of operators that have the
 *      corresponding fact as an effect.
 * HEUR_FACT_OP_SIMPLIFY
 *      Simplifies internal representation of operators so that there are
 *      no two operators applicable in the same time with the same effect.
 */

#ifdef HEUR_FACT_OP_FACT_T
# ifndef HEUR_FACT_OP_FACT_INIT
#  error "Initialization macro HEUR_FACT_OP_FACT_INIT must be defined"
# endif /* HEUR_FACT_OP_FACT_INIT */
#endif /* HEUR_FACT_OP_FACT_T */

/**
 * Struture representing an operator
 */
struct _plan_heur_op_t {
    int unsat;         /*!< Number of unsatisfied preconditions */
    plan_cost_t value; /*!< Value assigned to the operator */
    plan_cost_t cost;  /*!< Cost of the operator */

#ifdef HEUR_FACT_OP_EXTRA_OP
    HEUR_FACT_OP_EXTRA_OP;
#endif /* HEUR_FACT_OP_EXTRA_OP */
};
typedef struct _plan_heur_op_t plan_heur_op_t;

/**
 * IDs of operators for wich the fact is effect or precondition.
 */
struct _plan_heur_oparr_t {
    int *op;  /*!< List of operator IDs */
    int size; /*!< Size of op[] array */
};
typedef struct _plan_heur_oparr_t plan_heur_oparr_t;

/**
 * Effects or preconditions of the operator.
 */
struct _plan_heur_factarr_t {
    int *fact; /*!< List of fact IDs */
    int size;  /*!< Size of fact[] array */
};
typedef struct _plan_heur_factarr_t plan_heur_factarr_t;

/**
 * Structure for translation of variable's value to fact id.
 */
struct _plan_heur_fact_id_t {
    int **fact_id;
    int fact_size;
    int var_size;
};
typedef struct _plan_heur_fact_id_t plan_heur_fact_id_t;

/**
 * Unrolled cross-referenced facts and operators.
 */
struct _plan_heur_fact_op_t {
    plan_heur_fact_id_t fact_id; /*!< Translation from variable-value pair
                                      to fact ID */
#ifdef HEUR_FACT_OP_FACT_T
    HEUR_FACT_OP_FACT_T *fact; /*!< List of facts */
#endif /* HEUR_FACT_OP_FACT_T */

    plan_heur_op_t *op;    /*!< List of operators */
    int *op_id;            /*!< Mapping between index in .op[] and actual
                                index of operator. This is here because of
                                conditional effects. Note that
                                .op_id[i] != i only for i >= .actual_op_size */
    plan_heur_oparr_t *fact_pre; /*!< Operators for which the corresponding
                                   fact is precondition -- the size of this
                                   array equals to .fact_size */
#ifdef HEUR_FACT_OP_FACT_EFF
    plan_heur_oparr_t *fact_eff; /*!< Operators for which the corresponding
                                      fact is effect. This array is created
                                      only in case of lm-cut heuristic. */
#endif /* HEUR_FACT_OP_FACT_EFF */
    plan_heur_factarr_t *op_eff; /*!< Unrolled effects of operators. Size
                                      of this array equals to .op_size */
    plan_heur_factarr_t *op_pre; /*!< Precondition facts of operators. Size
                                      of this array is .op_size */
    plan_heur_factarr_t goal;    /*!< Copied goal in terms of fact ID */
    int fact_size;               /*!< Number of the facts */
    int op_size;                 /*!< Number of operators */
    int actual_op_size;          /*!< The actual number of operators
                                      (op_size without conditional effects). */

#ifdef HEUR_FACT_OP_ARTIFICIAL_GOAL
    int artificial_goal;   /*!< Fact ID of the artificial goal */
#endif /* HEUR_FACT_OP_ARTIFICIAL_GOAL */

#ifdef HEUR_FACT_OP_NO_PRE_FACT
    int no_pre_fact;       /*!< Artificial precondition fact for all
                                operators without preconditions */
#endif /* HEUR_FACT_OP_NO_PRE_FACT */
};
typedef struct _plan_heur_fact_op_t plan_heur_fact_op_t;

/**
 * Initializes heur_fact_op_t structure.
 */
static void planHeurFactOpInit(plan_heur_fact_op_t *fact_op,
                               const plan_var_t *var, int var_size,
                               const plan_part_state_t *goal,
                               const plan_op_t *op, int op_size,
                               const plan_succ_gen_t *_succ_gen);

/**
 * Frees heur_fact_op_t structure.
 */
static void planHeurFactOpFree(plan_heur_fact_op_t *fact_op);



/** Initializes struct for translating variable value to fact ID */
static void planHeurFactIdInit(plan_heur_fact_id_t *factid,
                               const plan_var_t *var, int var_size);
/** Frees val_to_id_t resources */
static void planHeurFactIdFree(plan_heur_fact_id_t *factid);
/** Translates variable value to fact ID */
_bor_inline int planHeurFactId(const plan_heur_fact_id_t *factid,
                               plan_var_id_t var, plan_val_t val);

/** Frees plan_heur_oparr_t array */
static void planHeurOparrFree(plan_heur_oparr_t *oparr, int size);
/** Frees plan_heur_factarr_t array */
static void planHeurFactarrFree(plan_heur_factarr_t *factarr, int size);
/** Initializes and frees .goal structure */
static void goalInit(plan_heur_fact_op_t *fact_op,
                     const plan_part_state_t *goal);
static void goalFree(plan_heur_fact_op_t *fact_op);

#ifdef HEUR_FACT_OP_FACT_T
/** Initializes and frees .fact structures */
static void factInit(plan_heur_fact_op_t *fact_op);
static void factFree(plan_heur_fact_op_t *fact_op);
#endif /* HEUR_FACT_OP_FACT_T */

/** Initializes and frees .op* srtuctures */
static void opInit(plan_heur_fact_op_t *fact_op,
                   const plan_op_t *ops, int ops_size);
static void opFree(plan_heur_fact_op_t *fact_op);
/** Initializes and frees .precond structures. */
static void opPreInit(plan_heur_fact_op_t *fact_op,
                      const plan_op_t *ops, int ops_size);
/** Initializes and frees .eff* structures */
static void opEffInit(plan_heur_fact_op_t *fact_op,
                      const plan_op_t *ops, int ops_size);
/** Initializes .fact_pre structures */
static void factPreInit(plan_heur_fact_op_t *fact_op);
#ifdef HEUR_FACT_OP_FACT_EFF
/** Initializes .fact_eff structures */
static void factEffInit(plan_heur_fact_op_t *fact_op);
#endif /* HEUR_FACT_OP_FACT_EFF */

#ifdef HEUR_FACT_OP_SIMPLIFY
/** Remove same effects where its cost is higher */
static void opSimplifyEffects(plan_heur_factarr_t *ref_eff,
                              plan_heur_factarr_t *op_eff,
                              plan_cost_t ref_cost, plan_cost_t op_cost);
/** Simplifies internal representation of operators so that there are no
 *  two operators applicable in the same time with the same effect. */
static void opSimplify(plan_heur_fact_op_t *fact_op,
                       const plan_op_t *op,
                       const plan_succ_gen_t *succ_gen);
#endif /* HEUR_FACT_OP_SIMPLIFY */


static void planHeurFactOpInit(plan_heur_fact_op_t *fact_op,
                               const plan_var_t *var, int var_size,
                               const plan_part_state_t *goal,
                               const plan_op_t *op, int op_size,
                               const plan_succ_gen_t *_succ_gen)
{
#ifdef HEUR_FACT_OP_SIMPLIFY
    plan_succ_gen_t *succ_gen;
#endif /* HEUR_FACT_OP_SIMPLIFY */

    bzero(fact_op, sizeof(*fact_op));
    planHeurFactIdInit(&fact_op->fact_id, var, var_size);
    goalInit(fact_op, goal);

    fact_op->fact_size = fact_op->fact_id.fact_size;

#ifdef HEUR_FACT_OP_ARTIFICIAL_GOAL
    fact_op->fact_size += 1;
    fact_op->artificial_goal = fact_op->fact_size - 1;
#endif /* HEUR_FACT_OP_ARTIFICIAL_GOAL */

#ifdef HEUR_FACT_OP_NO_PRE_FACT
    fact_op->fact_size += 1;
    fact_op->no_pre_fact = fact_op->fact_size - 1;
#endif /* HEUR_FACT_OP_NO_PRE_FACT */

#ifdef HEUR_FACT_OP_FACT_T
    factInit(fact_op);
#endif /* HEUR_FACT_OP_FACT_T */

    opInit(fact_op, op, op_size);
    opPreInit(fact_op, op, op_size);
    opEffInit(fact_op, op, op_size);

#ifdef HEUR_FACT_OP_SIMPLIFY
    succ_gen = (plan_succ_gen_t *)_succ_gen;
    if (_succ_gen == NULL)
        succ_gen = planSuccGenNew(op, op_size, NULL);
    opSimplify(fact_op, op, succ_gen);
    if (_succ_gen == NULL)
        planSuccGenDel(succ_gen);
#endif /* HEUR_FACT_OP_SIMPLIFY */

    factPreInit(fact_op);

#ifdef HEUR_FACT_OP_FACT_EFF
    factEffInit(fact_op);
#endif /* HEUR_FACT_OP_FACT_EFF */
}

static void planHeurFactOpFree(plan_heur_fact_op_t *fact_op)
{
    planHeurFactIdFree(&fact_op->fact_id);
    planHeurOparrFree(fact_op->fact_pre, fact_op->fact_size);
#ifdef HEUR_FACT_OP_FACT_EFF
    planHeurOparrFree(fact_op->fact_eff, fact_op->fact_size);
#endif /* HEUR_FACT_OP_FACT_EFF */
    planHeurFactarrFree(fact_op->op_pre, fact_op->op_size);
    planHeurFactarrFree(fact_op->op_eff, fact_op->op_size);
    opFree(fact_op);

#ifdef HEUR_FACT_OP_FACT_T
    factFree(fact_op);
#endif /* HEUR_FACT_OP_FACT_T */

    goalFree(fact_op);
    bzero(fact_op, sizeof(*fact_op));
}


static void planHeurFactIdInit(plan_heur_fact_id_t *fid,
                               const plan_var_t *var, int var_size)
{
    int i, j, id;

    // allocate the array corresponding to the variables
    fid->fact_id = BOR_ALLOC_ARR(int *, var_size);

    for (id = 0, i = 0; i < var_size; ++i){
        // allocate array for variable's values
        fid->fact_id[i] = BOR_ALLOC_ARR(int, var[i].range);

        // fill values with IDs
        for (j = 0; j < var[i].range; ++j)
            fid->fact_id[i][j] = id++;
    }

    // remember number of values
    fid->fact_size = id;
    fid->var_size = var_size;
}

static void planHeurFactIdFree(plan_heur_fact_id_t *fid)
{
    int i;
    for (i = 0; i < fid->var_size; ++i)
        BOR_FREE(fid->fact_id[i]);
    BOR_FREE(fid->fact_id);
}

_bor_inline int planHeurFactId(const plan_heur_fact_id_t *fid,
                               plan_var_id_t var, plan_val_t val)
{
    return fid->fact_id[var][val];
}

static void planHeurOparrFree(plan_heur_oparr_t *oparr, int size)
{
    int i;

    if (oparr == NULL)
        return;

    for (i = 0; i < size; ++i){
        if (oparr[i].op != NULL)
            BOR_FREE(oparr[i].op);
    }
    BOR_FREE(oparr);
}

static void planHeurFactarrFree(plan_heur_factarr_t *factarr, int size)
{
    int i;

    if (factarr == NULL)
        return;

    for (i = 0; i < size; ++i){
        if (factarr[i].fact != NULL)
            BOR_FREE(factarr[i].fact);
    }
    BOR_FREE(factarr);
}

static void goalInit(plan_heur_fact_op_t *fact_op,
                     const plan_part_state_t *goal)
{
    int i, id;
    plan_var_id_t var;
    plan_val_t val;

    fact_op->goal.size = goal->vals_size;
    fact_op->goal.fact = BOR_ALLOC_ARR(int, fact_op->goal.size);
    PLAN_PART_STATE_FOR_EACH(goal, i, var, val){
        id = planHeurFactId(&fact_op->fact_id, var, val);
        fact_op->goal.fact[i] = id;
    }
}

static void goalFree(plan_heur_fact_op_t *fact_op)
{
    if (fact_op->goal.fact)
        BOR_FREE(fact_op->goal.fact);
}

#ifdef HEUR_FACT_OP_FACT_T
static void factInit(plan_heur_fact_op_t *fact_op)
{
    int i;

    // prepare fact arrays
    fact_op->fact = BOR_ALLOC_ARR(HEUR_FACT_OP_FACT_T, fact_op->fact_size);

    // set up initial values
    for (i = 0; i < fact_op->fact_size; ++i){
        HEUR_FACT_OP_FACT_INIT(fact_op->fact + i);
    }
}

static void factFree(plan_heur_fact_op_t *fact_op)
{
    BOR_FREE(fact_op->fact);
}
#endif /* HEUR_FACT_OP_FACT_T */

static void opInit(plan_heur_fact_op_t *fact_op,
                   const plan_op_t *ops, int ops_size)
{
    int i, j, cond_eff_size;
    int cond_eff_ins;
    plan_op_cond_eff_t *cond_eff;

    // determine number of conditional effects in operators
    cond_eff_size = 0;
    for (i = 0; i < ops_size; ++i){
        cond_eff_size += ops[i].cond_eff_size;
    }

    // prepare operator arrays
    fact_op->op_size = ops_size + cond_eff_size;
#ifdef HEUR_FACT_OP_ARTIFICIAL_GOAL
    fact_op->op_size += 1;
#endif /* HEUR_FACT_OP_ARTIFICIAL_GOAL */
    fact_op->actual_op_size = ops_size;
    fact_op->op = BOR_ALLOC_ARR(plan_heur_op_t, fact_op->op_size);
    fact_op->op_id = BOR_ALLOC_ARR(int, fact_op->op_size);

    // Determine starting position for insertion of conditional effects.
    // The conditional effects are inserted as a (somehow virtual)
    // operators at the end of the fact_op->op array.
    cond_eff_ins = ops_size;

    for (i = 0; i < ops_size; ++i){
        fact_op->op[i].unsat = ops[i].pre->vals_size;
        fact_op->op[i].value = ops[i].cost;
        fact_op->op[i].cost  = ops[i].cost;
        fact_op->op_id[i] = i;

#ifdef HEUR_FACT_OP_EXTRA_OP_INIT
        HEUR_FACT_OP_EXTRA_OP_INIT(fact_op->op + i, ops + i, 0);
#endif /* HEUR_FACT_OP_EXTRA_OP_INIT */

        // Insert conditional effects
        for (j = 0; j < ops[i].cond_eff_size; ++j){
            cond_eff = ops[i].cond_eff + j;
            fact_op->op[cond_eff_ins].unsat  = cond_eff->pre->vals_size;
            fact_op->op[cond_eff_ins].unsat += ops[i].pre->vals_size;
            fact_op->op[cond_eff_ins].value = ops[i].cost;
            fact_op->op[cond_eff_ins].cost  = ops[i].cost;
            fact_op->op_id[cond_eff_ins] = i;

#ifdef HEUR_FACT_OP_EXTRA_OP_INIT
            HEUR_FACT_OP_EXTRA_OP_INIT(fact_op->op + cond_eff_ins, ops + i, 1);
#endif /* HEUR_FACT_OP_EXTRA_OP_INIT */

            ++cond_eff_ins;
        }
    }

#ifdef HEUR_FACT_OP_ARTIFICIAL_GOAL
    fact_op->op[fact_op->op_size - 1].unsat = fact_op->goal.size;
    fact_op->op[fact_op->op_size - 1].value = 0;
    fact_op->op[fact_op->op_size - 1].cost  = 0;
    fact_op->op_id[i] = -1;
#endif /* HEUR_FACT_OP_ARTIFICIAL_GOAL */
}

static void opFree(plan_heur_fact_op_t *fact_op)
{
    BOR_FREE(fact_op->op);
    BOR_FREE(fact_op->op_id);
}

static void opPrecondCondEffInit(plan_heur_factarr_t *precond,
                                 const plan_heur_fact_id_t *fid,
                                 const plan_op_t *op,
                                 const plan_op_cond_eff_t *cond_eff)
{
    int i, size, id;
    plan_var_id_t var;
    plan_val_t val;
    const plan_part_state_t *op_pre, *cond_eff_pre;

    op_pre = op->pre;
    cond_eff_pre = cond_eff->pre;

    precond->size = op_pre->vals_size + cond_eff_pre->vals_size;
    precond->fact = BOR_ALLOC_ARR(int, precond->size);

    size = planPartStateSize(cond_eff_pre);
    for (i = 0, var = 0; var < size; ++var){
        if (planPartStateIsSet(op_pre, var)){
            val = planPartStateGet(op_pre, var);
            id = planHeurFactId(fid, var, val);
            precond->fact[i++] = id;

        }else if (planPartStateIsSet(cond_eff_pre, var)){
            val = planPartStateGet(cond_eff_pre, var);
            id = planHeurFactId(fid, var, val);
            precond->fact[i++] = id;
        }
    }
}

static void opPreInit(plan_heur_fact_op_t *fact_op,
                      const plan_op_t *ops, int ops_size)
{
    int i, j, id, cond_eff_ins;
    plan_var_id_t var;
    plan_val_t val;
    plan_heur_factarr_t *precond;

    precond = fact_op->op_pre = BOR_ALLOC_ARR(plan_heur_factarr_t,
                                              fact_op->op_size);

    // The conditional effects are after "ordinary" operators
    cond_eff_ins = ops_size;

    for (i = 0; i < ops_size; ++i, ++precond){
        precond->size = ops[i].pre->vals_size;
        precond->fact = BOR_ALLOC_ARR(int, precond->size);

        PLAN_PART_STATE_FOR_EACH(ops[i].pre, j, var, val){
            id = planHeurFactId(&fact_op->fact_id, var, val);
            precond->fact[j] = id;
        }

#ifdef HEUR_FACT_OP_NO_PRE_FACT
        if (precond->size == 0){
            precond->size = 1;
            precond->fact = BOR_REALLOC_ARR(precond->fact, int, 1);
            precond->fact[0] = fact_op->no_pre_fact;
        }
#endif /* HEUR_FACT_OP_NO_PRE_FACT */

        for (j = 0; j < ops[i].cond_eff_size; ++j){
            opPrecondCondEffInit(fact_op->op_pre + cond_eff_ins,
                                 &fact_op->fact_id,
                                 ops + i, ops[i].cond_eff + j);
            ++cond_eff_ins;
        }
    }

#ifdef HEUR_FACT_OP_ARTIFICIAL_GOAL
    precond = fact_op->op_pre + fact_op->op_size - 1;
    precond->size = fact_op->goal.size;
    precond->fact = BOR_ALLOC_ARR(int, precond->size);
    memcpy(precond->fact, fact_op->goal.fact, sizeof(int) * precond->size);
#endif /* HEUR_FACT_OP_ARTIFICIAL_GOAL */
}

static void opEffCondEffInit(plan_heur_fact_op_t *fact_op,
                             plan_heur_factarr_t *eff,
                             const plan_op_cond_eff_t *cond_eff)
{
    int j;
    plan_var_id_t var;
    plan_val_t val;
    const plan_part_state_t *cond_eff_eff;

    cond_eff_eff = cond_eff->eff;

    // Use only the conditioned effects because the rest of the effects are
    // handled in the original operator (w/o conditional effects).
    eff->size = cond_eff_eff->vals_size;
    eff->fact = BOR_ALLOC_ARR(int, eff->size);
    PLAN_PART_STATE_FOR_EACH(cond_eff_eff, j, var, val){
        eff->fact[j++] = planHeurFactId(&fact_op->fact_id, var, val);
    }
}

static void opEffInit(plan_heur_fact_op_t *fact_op,
                      const plan_op_t *ops, int ops_size)
{
    int i, j, cond_eff_ins;
    plan_var_id_t var;
    plan_val_t val;
    plan_heur_factarr_t *eff;

    eff = fact_op->op_eff = BOR_ALLOC_ARR(plan_heur_factarr_t, fact_op->op_size);

    // Conditional effects are after all operators
    cond_eff_ins = ops_size;

    // First insert effects ignoring conditional effects
    for (i = 0; i < ops_size; ++i, ++eff){
        eff->size = ops[i].eff->vals_size;
        eff->fact = BOR_ALLOC_ARR(int, eff->size);
        PLAN_PART_STATE_FOR_EACH(ops[i].eff, j, var, val){
            eff->fact[j] = planHeurFactId(&fact_op->fact_id, var, val);
        }

        for (j = 0; j < ops[i].cond_eff_size; ++j){
            opEffCondEffInit(fact_op, fact_op->op_eff + cond_eff_ins,
                             ops[i].cond_eff + j);
            ++cond_eff_ins;
        }
    }

#ifdef HEUR_FACT_OP_ARTIFICIAL_GOAL
    eff = fact_op->op_eff + fact_op->op_size - 1;
    eff->size = 1;
    eff->fact = BOR_ALLOC_ARR(int, 1);
    eff->fact[0] = fact_op->artificial_goal;
#endif /* HEUR_FACT_OP_ARTIFICIAL_GOAL */
}

static void crossReferenceInit(plan_heur_oparr_t **dst, int fact_size,
                               plan_heur_factarr_t *src, int op_size,
                               plan_heur_factarr_t *test)
{
    int i, opi, fact_id;
    plan_heur_oparr_t *fact, *update;
    plan_heur_factarr_t *cur, *end;

    *dst = BOR_CALLOC_ARR(plan_heur_oparr_t, fact_size);
    fact = *dst;

    end = src + op_size;
    for (cur = src, opi = 0; cur < end; ++cur, ++opi){
        if (test && test[opi].size == 0)
            continue;

        for (i = 0; i < cur->size; ++i){
            fact_id = cur->fact[i];
            update  = fact + fact_id;

            ++update->size;
            update->op = BOR_REALLOC_ARR(update->op, int, update->size);
            update->op[update->size - 1] = opi;
        }
    }
}

static void factPreInit(plan_heur_fact_op_t *fact_op)
{
#ifdef HEUR_FACT_OP_NO_OPTIMIZE_FACT_PRE
    crossReferenceInit(&fact_op->fact_pre, fact_op->fact_size,
                       fact_op->op_pre, fact_op->op_size, NULL);
#else /* HEUR_FACT_OP_NO_OPTIMIZE_FACT_PRE */
    crossReferenceInit(&fact_op->fact_pre, fact_op->fact_size,
                       fact_op->op_pre, fact_op->op_size,
                       fact_op->op_eff);
#endif /* HEUR_FACT_OP_NO_OPTIMIZE_FACT_PRE */
}

#ifdef HEUR_FACT_OP_FACT_EFF
static void factEffInit(plan_heur_fact_op_t *fact_op)
{
    crossReferenceInit(&fact_op->fact_eff, fact_op->fact_size,
                       fact_op->op_eff, fact_op->op_size, NULL);
}
#endif /* HEUR_FACT_OP_FACT_EFF */

#ifdef HEUR_FACT_OP_SIMPLIFY
static void effRemoveId(plan_heur_factarr_t *eff, int pos)
{
    int i;
    for (i = pos + 1; i < eff->size; ++i)
        eff->fact[i - 1] = eff->fact[i];
    --eff->size;
}

static void opSimplifyEffects(plan_heur_factarr_t *ref_eff,
                              plan_heur_factarr_t *op_eff,
                              plan_cost_t ref_cost, plan_cost_t op_cost)
{
    int ref_i, op_i;
    int ref_id, op_id;

    // We assume both .fact[] array are sorted in ascending order and the
    // arrays are kept this way.
    for (ref_i = 0, op_i = 0; ref_i < ref_eff->size && op_i < op_eff->size;){
        ref_id = ref_eff->fact[ref_i];
        op_id  = op_eff->fact[op_i];

        if (ref_id == op_id){
            // Both operators have same effect -- remove the one with
            // higher cost and keep the array of facts in ascending order
            if (op_cost <= ref_cost){
                effRemoveId(ref_eff, ref_i);
                ++op_i;
            }else{
                effRemoveId(op_eff, op_i);
                ++ref_i;
            }

        }else if (ref_id < op_id){
            ++ref_i;

        }else{
            ++op_i;
        }
    }
}

static void opSimplify(plan_heur_fact_op_t *fact_op,
                       const plan_op_t *op,
                       const plan_succ_gen_t *succ_gen)
{
    int i, ref_i, op_i;
    const plan_op_t *ref_op; // reference operator
    plan_op_t **ops;         // list of operators applicable in ref_op->pre
    int ops_size;            // number of applicable operators

    // Allocate array for applicable operators
    ops = BOR_ALLOC_ARR(plan_op_t *, fact_op->op_size);

    for (ref_i = 0; ref_i < fact_op->op_size; ++ref_i){
        if (fact_op->op_id[ref_i] < 0)
            continue;
        ref_op = op + fact_op->op_id[ref_i];

        // skip operators with no effects
        if (fact_op->op_eff[ref_i].size == 0)
            continue;

        // get all applicable operators
        ops_size = planSuccGenFindPart(succ_gen, ref_op->pre,
                                       ops, fact_op->op_size);

        for (i = 0; i < ops_size; ++i){
            // don't compare two identical operators
            if (ref_op == ops[i])
                continue;

            // compute id of the operator
            op_i = ((unsigned long)ops[i] - (unsigned long)op)
                        / sizeof(plan_op_t);

            // simplify effects, if possible delete in reference operator
            opSimplifyEffects(fact_op->op_eff + ref_i, fact_op->op_eff + op_i,
                              ref_op->cost, ops[i]->cost);
        }
    }

    BOR_FREE(ops);
}
#endif /* HEUR_FACT_OP_SIMPLIFY */
