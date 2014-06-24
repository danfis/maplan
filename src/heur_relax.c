#include <boruvka/alloc.h>
#include <boruvka/list.h>
#include <boruvka/bucketheap.h>
#include "plan/heur.h"

#define FACT_SET_ADDED(fact) ((fact)->state |= 0x1u)
#define FACT_SET_OPEN(fact) ((fact)->state |= 0x2u)
#define FACT_SET_CLOSED(fact) ((fact)->state &= 0xdu)
#define FACT_SET_GOAL(fact) ((fact)->state |= 0x4u)
#define FACT_SET_RELAXED_PLAN_VISITED(fact) ((fact)->state |= 0x8u)
#define FACT_UNSET_GOAL(fact) ((fact)->state &= 0xbu)
#define FACT_ADDED(fact) (((fact)->state & 0x1u) == 0x1u)
#define FACT_OPEN(fact) (((fact)->state & 0x2u) == 0x2u)
#define FACT_CLOSED(fact) (((fact)->state & 0x2u) == 0x0u)
#define FACT_GOAL(fact) (((fact)->state & 0x4u) == 0x4u)
#define FACT_RELAXED_PLAN_VISITED(fact) (((fact)->state & 0x8u) == 0x8u)

#define TYPE_ADD 0
#define TYPE_MAX 1
#define TYPE_FF  2

/**
 * Structure representing a single fact.
 */
struct _fact_t {
    plan_cost_t value;
    int id;
    unsigned state;
    int reached_by_op;
    bor_bucketheap_node_t heap;
};
typedef struct _fact_t fact_t;

/**
 * Context for relaxation algorithm.
 */
struct _ctx_t {
    int *op_unsat;
    plan_cost_t *op_value;
    fact_t *facts;
    bor_bucketheap_t *heap;
    int goal_unsat;
};
typedef struct _ctx_t ctx_t;


struct _plan_heur_relax_t {
    plan_heur_t heur;
    const plan_operator_t *ops;
    int ops_size;
    const plan_var_t *var;
    int var_size;
    const plan_part_state_t *goal;

    int **val_id;      /*!< ID of each variable value (val_id[var][val]) */
    int val_size;      /*!< Number of all values */
    int **precond;     /*!< Operator IDs indexed by precondition ID */
    int *precond_size; /*!< Size of each subarray */
    int *op_unsat;     /*!< Preinitialized counters of unsatisfied
                            preconditions per operator */
    int *op_value;     /*!< Preinitialized values of operators */

    int type;

    ctx_t ctx;
};
typedef struct _plan_heur_relax_t plan_heur_relax_t;


/** Initializes and frees .val_id[] structure */
static void valueIdInit(plan_heur_relax_t *heur);
static void valueIdFree(plan_heur_relax_t *heur);
/** Initializes and frees .precond and .op_* structures */
static void precondInit(plan_heur_relax_t *heur);
static void precondFree(plan_heur_relax_t *heur);

/** Initializes main structure for computing relaxed solution. */
static void ctxInit(plan_heur_relax_t *heur);
/** Free allocated resources */
static void ctxFree(plan_heur_relax_t *heur);
/** Adds initial state facts to the heap. */
static void ctxAddInitState(plan_heur_relax_t *heur,
                            const plan_state_t *state);
/** Add operators effects to the heap if possible */
static void ctxAddEffects(plan_heur_relax_t *heur, int op_id);
/** Process a single operator during relaxation */
static void ctxProcessOp(plan_heur_relax_t *heur, int op_id, fact_t *fact);
/** Main loop of algorithm for solving relaxed problem. */
static int ctxMainLoop(plan_heur_relax_t *heur);
/** Computes final heuristic from the values computed in previous steps. */
static plan_cost_t ctxHeur(plan_heur_relax_t *heur);

/** Main function that returns heuristic value. */
static plan_cost_t planHeurRelax(void *heur, const plan_state_t *state);
/** Delete method */
static void planHeurRelaxDel(void *_heur);

static plan_heur_t *planHeurRelaxNew(const plan_problem_t *prob, int type)
{
    plan_heur_relax_t *heur;

    heur = BOR_ALLOC(plan_heur_relax_t);
    planHeurInit(&heur->heur,
                 planHeurRelaxDel,
                 planHeurRelax);
    heur->ops      = prob->op;
    heur->ops_size = prob->op_size;
    heur->var      = prob->var;
    heur->var_size = prob->var_size;
    heur->goal     = prob->goal;
    heur->type     = type;

    valueIdInit(heur);
    precondInit(heur);

    return &heur->heur;
}

plan_heur_t *planHeurRelaxAddNew(const plan_problem_t *prob)
{
    return planHeurRelaxNew(prob, TYPE_ADD);
}

plan_heur_t *planHeurRelaxMaxNew(const plan_problem_t *prob)
{
    return planHeurRelaxNew(prob, TYPE_MAX);
}

plan_heur_t *planHeurRelaxFFNew(const plan_problem_t *prob)
{
    return planHeurRelaxNew(prob, TYPE_FF);
}


static void planHeurRelaxDel(void *_heur)
{
    plan_heur_relax_t *heur = _heur;
    valueIdFree(heur);
    precondFree(heur);
    planHeurFree(&heur->heur);
    BOR_FREE(heur);
}

static plan_cost_t planHeurRelax(void *_heur, const plan_state_t *state)
{
    plan_heur_relax_t *heur = _heur;
    plan_cost_t h = PLAN_HEUR_DEAD_END;

    ctxInit(heur);
    ctxAddInitState(heur, state);
    if (ctxMainLoop(heur) == 0){
        h = ctxHeur(heur);
    }
    ctxFree(heur);

    return h;
}



static void valueIdInit(plan_heur_relax_t *heur)
{
    int i, j, id;

    // allocate the array corresponding to the variables
    heur->val_id = BOR_ALLOC_ARR(int *, heur->var_size);

    for (id = 0, i = 0; i < heur->var_size; ++i){
        // allocate array for variable's values
        heur->val_id[i] = BOR_ALLOC_ARR(int, heur->var[i].range);

        // fill values with IDs
        for (j = 0; j < heur->var[i].range; ++j)
            heur->val_id[i][j] = id++;
    }

    // remember number of values
    heur->val_size = id;
}

static void valueIdFree(plan_heur_relax_t *heur)
{
    int i;

    for (i = 0; i < heur->var_size; ++i)
        BOR_FREE(heur->val_id[i]);
    BOR_FREE(heur->val_id);
}

static void precondInit(plan_heur_relax_t *heur)
{
    int i, j, id;
    plan_var_id_t var;
    plan_val_t val;

    // prepare precond array
    heur->precond = BOR_ALLOC_ARR(int *, heur->val_size);
    heur->precond_size = BOR_ALLOC_ARR(int, heur->val_size);
    for (i = 0; i < heur->val_size; ++i){
        heur->precond_size[i] = 0;
        heur->precond[i] = NULL;
    }

    // prepare initialization arrays
    heur->op_unsat = BOR_ALLOC_ARR(int, heur->ops_size);
    heur->op_value = BOR_ALLOC_ARR(plan_cost_t, heur->ops_size);

    // create mapping between precondition and operator
    for (i = 0; i < heur->ops_size; ++i){
        heur->op_unsat[i] = 0;
        heur->op_value[i] = heur->ops[i].cost;

        PLAN_PART_STATE_FOR_EACH(heur->ops[i].pre, j, var, val){
            id = heur->val_id[var][val];
            ++heur->precond_size[id];
            heur->precond[id] = BOR_REALLOC_ARR(heur->precond[id], int,
                                                heur->precond_size[id]);
            heur->precond[id][heur->precond_size[id] - 1] = i;

            heur->op_unsat[i] += 1;
        }
    }

    heur->ctx.op_unsat = BOR_ALLOC_ARR(int, heur->ops_size);
    heur->ctx.op_value = BOR_ALLOC_ARR(plan_cost_t, heur->ops_size);
    heur->ctx.facts    = BOR_ALLOC_ARR(fact_t, heur->val_size);
}

static void precondFree(plan_heur_relax_t *heur)
{
    int i;
    for (i = 0; i < heur->val_size; ++i){
        BOR_FREE(heur->precond[i]);
    }
    BOR_FREE(heur->precond);
    BOR_FREE(heur->precond_size);
    BOR_FREE(heur->op_unsat);
    BOR_FREE(heur->op_value);
    BOR_FREE(heur->ctx.facts);
    BOR_FREE(heur->ctx.op_value);
    BOR_FREE(heur->ctx.op_unsat);
}


static void ctxInit(plan_heur_relax_t *heur)
{
    int i, id;
    plan_var_id_t var;
    plan_val_t val;

    memcpy(heur->ctx.op_unsat, heur->op_unsat, sizeof(int) * heur->ops_size);
    memcpy(heur->ctx.op_value, heur->op_value, sizeof(plan_cost_t) * heur->ops_size);
    bzero(heur->ctx.facts, sizeof(fact_t) * heur->val_size);

    heur->ctx.heap = borBucketHeapNew();

    // set fact goals
    heur->ctx.goal_unsat = 0;
    PLAN_PART_STATE_FOR_EACH(heur->goal, i, var, val){
        id = heur->val_id[var][val];
        FACT_SET_GOAL(heur->ctx.facts + id);
        ++heur->ctx.goal_unsat;
    }
}

static void ctxFree(plan_heur_relax_t *heur)
{
    borBucketHeapDel(heur->ctx.heap);
}

static void ctxAddInitState(plan_heur_relax_t *heur,
                            const plan_state_t *state)
{
    int i, id;

    // insert all facts from the initial state into priority queue
    for (i = 0; i < heur->var_size; ++i){
        id = heur->val_id[i][planStateGet(state, i)];
        heur->ctx.facts[id].id = id;
        FACT_SET_ADDED(heur->ctx.facts + id);
        FACT_SET_OPEN(heur->ctx.facts + id);
        borBucketHeapAdd(heur->ctx.heap, heur->ctx.facts[id].value,
                         &heur->ctx.facts[id].heap);
        heur->ctx.facts[id].reached_by_op = -1;
    }
}

static void ctxAddEffects(plan_heur_relax_t *heur, int op_id)
{
    const plan_operator_t *op = heur->ops + op_id;
    int i, id;
    plan_var_id_t var;
    plan_val_t val;

    PLAN_PART_STATE_FOR_EACH(op->eff, i, var, val){
        id = heur->val_id[var][val];

        if (!FACT_ADDED(heur->ctx.facts + id)
                || heur->ctx.facts[id].value > heur->ctx.op_value[op_id]){

            heur->ctx.facts[id].value = heur->ctx.op_value[op_id];
            heur->ctx.facts[id].reached_by_op = op_id;
            if (FACT_OPEN(heur->ctx.facts + id)){
                borBucketHeapDecreaseKey(heur->ctx.heap, &heur->ctx.facts[id].heap,
                                         heur->ctx.facts[id].value);

            }else{ // fact not in heap
                heur->ctx.facts[id].id = id;
                FACT_SET_ADDED(heur->ctx.facts + id);
                FACT_SET_OPEN(heur->ctx.facts + id);
                fprintf(stderr, "Add: %d\n", heur->ctx.facts[id].value);
                borBucketHeapAdd(heur->ctx.heap, heur->ctx.facts[id].value,
                                 &heur->ctx.facts[id].heap);
            }
        }
    }
}

_bor_inline plan_cost_t relaxHeurOpValue(int type,
                                         plan_cost_t op_cost,
                                         plan_cost_t op_value,
                                         plan_cost_t fact_value)
{
    if (type == TYPE_ADD || type == TYPE_FF)
        return op_value + fact_value;
    return BOR_MAX(op_cost + fact_value, op_value);
}

static void ctxProcessOp(plan_heur_relax_t *heur, int op_id, fact_t *fact)
{
    // update operator value
    heur->ctx.op_value[op_id] = relaxHeurOpValue(heur->type,
                                                 heur->ops[op_id].cost,
                                                 heur->ctx.op_value[op_id],
                                                 fact->value);

    // satisfy operator precondition
    heur->ctx.op_unsat[op_id] = BOR_MAX(heur->ctx.op_unsat[op_id] - 1, 0);

    if (heur->ctx.op_unsat[op_id] == 0){
        ctxAddEffects(heur, op_id);
    }
}

static int ctxMainLoop(plan_heur_relax_t *heur)
{
    bor_bucketheap_node_t *heap_node;
    fact_t *fact;
    int i, size, *precond;

    while (!borBucketHeapEmpty(heur->ctx.heap)){
        heap_node = borBucketHeapExtractMin(heur->ctx.heap, NULL);
        fact = bor_container_of(heap_node, fact_t, heap);
        FACT_SET_CLOSED(fact);

        if (FACT_GOAL(fact)){
            FACT_UNSET_GOAL(fact);
            --heur->ctx.goal_unsat;
            if (heur->ctx.goal_unsat == 0)
                return 0;
        }

        size = heur->precond_size[fact->id];
        precond = heur->precond[fact->id];
        for (i = 0; i < size; ++i){
            ctxProcessOp(heur, precond[i], fact);
        }
    }

    return -1;
}


_bor_inline plan_cost_t relaxHeurValue(int type,
                                       plan_cost_t value1,
                                       plan_cost_t value2)
{
    if (type == TYPE_ADD)
        return value1 + value2;
    return BOR_MAX(value1, value2);
}

static plan_cost_t relaxHeurAddMax(plan_heur_relax_t *heur)
{
    int i, id;
    plan_cost_t h;
    plan_var_id_t var;
    plan_val_t val;

    h = PLAN_COST_ZERO;
    PLAN_PART_STATE_FOR_EACH(heur->goal, i, var, val){
        id = heur->val_id[var][val];
        h = relaxHeurValue(heur->type, h, heur->ctx.facts[id].value);
    }

    return h;
}

static void markRelaxedPlan(plan_heur_relax_t *heur, int *relaxed_plan, int id)
{
    fact_t *fact = heur->ctx.facts + id;
    const plan_operator_t *op;
    int i, id2;
    plan_var_id_t var;
    plan_val_t val;

    if (!FACT_RELAXED_PLAN_VISITED(fact) && fact->reached_by_op != -1){
        FACT_SET_RELAXED_PLAN_VISITED(fact);
        op = heur->ops + fact->reached_by_op;

        PLAN_PART_STATE_FOR_EACH(op->pre, i, var, val){
            id2 = heur->val_id[var][val];
            if (!FACT_RELAXED_PLAN_VISITED(heur->ctx.facts + id2)
                    && heur->ctx.facts[id2].reached_by_op != -1){
                markRelaxedPlan(heur, relaxed_plan, id2);
            }
        }

        relaxed_plan[fact->reached_by_op] = 1;
    }
}

static plan_cost_t relaxHeurFF(plan_heur_relax_t *heur)
{
    int *relaxed_plan;
    int i, id;
    plan_cost_t h = PLAN_COST_ZERO;
    plan_var_id_t var;
    plan_val_t val;

    relaxed_plan = BOR_CALLOC_ARR(int, heur->ops_size);

    PLAN_PART_STATE_FOR_EACH(heur->goal, i, var, val){
        id = heur->val_id[var][val];
        markRelaxedPlan(heur, relaxed_plan, id);
    }

    for (i = 0; i < heur->ops_size; ++i){
        if (relaxed_plan[i]){
            h += heur->ops[i].cost;
        }
    }

    BOR_FREE(relaxed_plan);
    return h;
}

static plan_cost_t ctxHeur(plan_heur_relax_t *heur)
{
    if (heur->type == TYPE_ADD || heur->type == TYPE_MAX){
        return relaxHeurAddMax(heur);
    }else{ // heur->type == TYPE_FF
        return relaxHeurFF(heur);
    }
}
