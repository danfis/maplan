#include <boruvka/alloc.h>
#include <boruvka/list.h>
#include <boruvka/pairheap.h>
#include "plan/heur_relax.h"

#define FACT_SET_ADDED(fact) ((fact)->state |= 0x1)
#define FACT_SET_OPEN(fact) ((fact)->state |= 0x2)
#define FACT_SET_CLOSED(fact) ((fact)->state &= 0xd)
#define FACT_SET_GOAL(fact) ((fact)->state |= 0x4)
#define FACT_SET_RELAXED_PLAN_VISITED(fact) ((fact)->state |= 0x8)
#define FACT_UNSET_GOAL(fact) ((fact)->state &= 0xb)
#define FACT_ADDED(fact) (((fact)->state & 0x1) == 0x1)
#define FACT_OPEN(fact) (((fact)->state & 0x2) == 0x2)
#define FACT_CLOSED(fact) (((fact)->state & 0x2) == 0x0)
#define FACT_GOAL(fact) (((fact)->state & 0x4) == 0x4)
#define FACT_RELAXED_PLAN_VISITED(fact) (((fact)->state & 0x8) == 0x8)

#define TYPE_ADD 0
#define TYPE_MAX 1
#define TYPE_FF  2

/**
 * Structure representing a single fact.
 */
struct _fact_t {
    unsigned value;
    int id;
    int state;
    int reached_by_op;
    bor_pairheap_node_t heap;
};
typedef struct _fact_t fact_t;

/**
 * Context for relaxation algorithm.
 */
struct _relax_t {
    int *op_unsat;
    int *op_value;
    fact_t *facts;
    bor_pairheap_t *heap;
    int goal_unsat;
};
typedef struct _relax_t relax_t;


/** Initializes and frees .val_id[] structure */
static void valueIdInit(plan_heur_relax_t *heur);
static void valueIdFree(plan_heur_relax_t *heur);
/** Initializes and frees .precond and .op_* structures */
static void precondInit(plan_heur_relax_t *heur);
static void precondFree(plan_heur_relax_t *heur);

/** Comparator for heap */
static int heapLT(const bor_pairheap_node_t *a,
                  const bor_pairheap_node_t *b,
                  void *_);

/** Initializes main structure for computing relaxed solution. */
static void relaxInit(plan_heur_relax_t *heur, relax_t *r);
/** Free allocated resources */
static void relaxFree(plan_heur_relax_t *heur, relax_t *r);
/** Adds initial state facts to the heap. */
static void relaxAddInitState(plan_heur_relax_t *heur, relax_t *r,
                              const plan_state_t *state);
/** Add operators effects to the heap if possible */
static void relaxAddEffects(plan_heur_relax_t *heur, relax_t *r, int op_id);
/** Process a single operator during relaxation */
static void relaxProcessOp(plan_heur_relax_t *heur, relax_t *r,
                           int op_id, fact_t *fact);
/** Main loop of algorithm for solving relaxed problem. */
static int relaxMainLoop(plan_heur_relax_t *heur, relax_t *r);
/** Computes final heuristic from the values computed in previous steps. */
static unsigned relaxHeur(plan_heur_relax_t *heur, relax_t *r);


static plan_heur_relax_t *planHeurRelaxNew(const plan_problem_t *prob,
                                           int type)
{
    plan_heur_relax_t *heur;

    heur = BOR_ALLOC(plan_heur_relax_t);
    heur->ops      = prob->op;
    heur->ops_size = prob->op_size;
    heur->var      = prob->var;
    heur->var_size = prob->var_size;
    heur->goal     = prob->goal;
    heur->type     = type;

    valueIdInit(heur);
    precondInit(heur);

    return heur;
}

plan_heur_relax_t *planHeurRelaxAddNew(const plan_problem_t *prob)
{
    return planHeurRelaxNew(prob, TYPE_ADD);
}

plan_heur_relax_t *planHeurRelaxMaxNew(const plan_problem_t *prob)
{
    return planHeurRelaxNew(prob, TYPE_MAX);
}

plan_heur_relax_t *planHeurRelaxFFNew(const plan_problem_t *prob)
{
    return planHeurRelaxNew(prob, TYPE_FF);
}


void planHeurRelaxDel(plan_heur_relax_t *heur)
{
    valueIdFree(heur);
    precondFree(heur);
    BOR_FREE(heur);
}

unsigned planHeurRelax(plan_heur_relax_t *heur,
                       const plan_state_t *state)
{
    relax_t relax;
    unsigned h = PLAN_HEUR_DEAD_END;

    relaxInit(heur, &relax);
    relaxAddInitState(heur, &relax, state);
    if (relaxMainLoop(heur, &relax) == 0){
        h = relaxHeur(heur, &relax);
    }
    relaxFree(heur, &relax);

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
    unsigned var, val;

    // prepare precond array
    heur->precond = BOR_ALLOC_ARR(int *, heur->val_size);
    heur->precond_size = BOR_ALLOC_ARR(int, heur->val_size);
    for (i = 0; i < heur->val_size; ++i){
        heur->precond_size[i] = 0;
        heur->precond[i] = NULL;
    }

    // prepare initialization arrays
    heur->op_unsat = BOR_ALLOC_ARR(int, heur->ops_size);
    heur->op_value = BOR_ALLOC_ARR(int, heur->ops_size);

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
}


static int heapLT(const bor_pairheap_node_t *a,
                  const bor_pairheap_node_t *b,
                  void *_)
{
    const fact_t *f1 = bor_container_of(a, const fact_t, heap);
    const fact_t *f2 = bor_container_of(b, const fact_t, heap);
    return f1->value < f2->value;
}

static void relaxInit(plan_heur_relax_t *heur, relax_t *r)
{
    int i, id;
    unsigned var, val;

    r->op_unsat = BOR_ALLOC_ARR(int, heur->ops_size);
    memcpy(r->op_unsat, heur->op_unsat, sizeof(int) * heur->ops_size);

    r->op_value = BOR_ALLOC_ARR(int, heur->ops_size);
    memcpy(r->op_value, heur->op_value, sizeof(int) * heur->ops_size);

    r->facts = BOR_CALLOC_ARR(fact_t, heur->val_size);

    r->heap = borPairHeapNew(heapLT, NULL);

    // set fact goals
    r->goal_unsat = 0;
    PLAN_PART_STATE_FOR_EACH(heur->goal, i, var, val){
        id = heur->val_id[var][val];
        FACT_SET_GOAL(r->facts + id);
        ++r->goal_unsat;
    }
}

static void relaxFree(plan_heur_relax_t *heur, relax_t *r)
{
    borPairHeapDel(r->heap);
    BOR_FREE(r->facts);
    BOR_FREE(r->op_value);
    BOR_FREE(r->op_unsat);
}

static void relaxAddInitState(plan_heur_relax_t *heur, relax_t *r,
                              const plan_state_t *state)
{
    int i, id;

    // insert all facts from the initial state into priority queue
    for (i = 0; i < heur->var_size; ++i){
        id = heur->val_id[i][planStateGet(state, i)];
        r->facts[id].id = id;
        FACT_SET_ADDED(r->facts + id);
        FACT_SET_OPEN(r->facts + id);
        borPairHeapAdd(r->heap, &r->facts[id].heap);
        r->facts[id].reached_by_op = -1;
    }
}

static void relaxAddEffects(plan_heur_relax_t *heur, relax_t *r,
                            int op_id)
{
    const plan_operator_t *op = heur->ops + op_id;
    int i, id;
    unsigned var, val;

    PLAN_PART_STATE_FOR_EACH(op->eff, i, var, val){
        id = heur->val_id[var][val];

        if (!FACT_ADDED(r->facts + id)
                || r->facts[id].value > r->op_value[op_id]){

            r->facts[id].value = r->op_value[op_id];
            r->facts[id].reached_by_op = op_id;
            if (FACT_OPEN(r->facts + id)){
                borPairHeapDecreaseKey(r->heap, &r->facts[id].heap);

            }else{ // fact not in heap
                r->facts[id].id = id;
                FACT_SET_ADDED(r->facts + id);
                FACT_SET_OPEN(r->facts + id);
                borPairHeapAdd(r->heap, &r->facts[id].heap);
            }
        }
    }
}

_bor_inline unsigned relaxHeurOpValue(int type,
                                      unsigned op_cost,
                                      unsigned op_value,
                                      unsigned fact_value)
{
    if (type == TYPE_ADD || type == TYPE_FF)
        return op_value + fact_value;
    return BOR_MAX(op_cost + fact_value, op_value);
}

static void relaxProcessOp(plan_heur_relax_t *heur, relax_t *r,
                           int op_id, fact_t *fact)
{
    // update operator value
    r->op_value[op_id] = relaxHeurOpValue(heur->type,
                                          heur->ops[op_id].cost,
                                          r->op_value[op_id],
                                          fact->value);

    // satisfy operator precondition
    r->op_unsat[op_id] = BOR_MAX(r->op_unsat[op_id] - 1, 0);

    if (r->op_unsat[op_id] == 0){
        relaxAddEffects(heur, r, op_id);
    }
}

static int relaxMainLoop(plan_heur_relax_t *heur, relax_t *r)
{
    bor_pairheap_node_t *heap_node;
    fact_t *fact;
    int i, size, *precond;

    while (!borPairHeapEmpty(r->heap)){
        heap_node = borPairHeapExtractMin(r->heap);
        fact = bor_container_of(heap_node, fact_t, heap);
        FACT_SET_CLOSED(fact);

        if (FACT_GOAL(fact)){
            FACT_UNSET_GOAL(fact);
            --r->goal_unsat;
            if (r->goal_unsat == 0)
                return 0;
        }

        size = heur->precond_size[fact->id];
        precond = heur->precond[fact->id];
        for (i = 0; i < size; ++i){
            relaxProcessOp(heur, r, precond[i], fact);
        }
    }

    return -1;
}


_bor_inline unsigned relaxHeurValue(int type, unsigned value1, unsigned value2)
{
    if (type == TYPE_ADD)
        return value1 + value2;
    return BOR_MAX(value1, value2);
}

static unsigned relaxHeurAddMax(plan_heur_relax_t *heur, relax_t *r)
{
    int i, id;
    unsigned h, var, val;

    h = 0;
    PLAN_PART_STATE_FOR_EACH(heur->goal, i, var, val){
        id = heur->val_id[var][val];
        h = relaxHeurValue(heur->type, h, r->facts[id].value);
    }

    return h;
}

static void markRelaxedPlan(plan_heur_relax_t *heur, relax_t *r,
                            int *relaxed_plan, int id)
{
    fact_t *fact = r->facts + id;
    const plan_operator_t *op;
    int i, id2;
    unsigned var, val;

    if (!FACT_RELAXED_PLAN_VISITED(fact) && fact->reached_by_op != -1){
        FACT_SET_RELAXED_PLAN_VISITED(fact);
        op = heur->ops + fact->reached_by_op;

        PLAN_PART_STATE_FOR_EACH(op->pre, i, var, val){
            id2 = heur->val_id[var][val];
            if (!FACT_RELAXED_PLAN_VISITED(r->facts + id2)
                    && r->facts[id2].reached_by_op != -1){
                markRelaxedPlan(heur, r, relaxed_plan, id2);
            }
        }

        relaxed_plan[fact->reached_by_op] = 1;
    }
}

static unsigned relaxHeurFF(plan_heur_relax_t *heur, relax_t *r)
{
    int *relaxed_plan;
    int i, id;
    unsigned h = 0;
    unsigned var, val;

    relaxed_plan = BOR_CALLOC_ARR(int, heur->ops_size);

    PLAN_PART_STATE_FOR_EACH(heur->goal, i, var, val){
        id = heur->val_id[var][val];
        markRelaxedPlan(heur, r, relaxed_plan, id);
    }

    for (i = 0; i < heur->ops_size; ++i){
        if (relaxed_plan[i]){
            h += heur->ops[i].cost;
        }
    }

    BOR_FREE(relaxed_plan);
    return h;
}

static unsigned relaxHeur(plan_heur_relax_t *heur, relax_t *r)
{
    if (heur->type == TYPE_ADD || heur->type == TYPE_MAX){
        return relaxHeurAddMax(heur, r);
    }else{ // heur->type == TYPE_FF
        return relaxHeurFF(heur, r);
    }
}
