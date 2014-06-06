#include <boruvka/alloc.h>
#include <boruvka/list.h>
#include <boruvka/pairheap.h>
#include "plan/heur_relax.h"

/**
 * Structure representing a single fact.
 */
struct _fact_t {
    unsigned value;
    int visited;
    int id;
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
static void relaxMainLoop(plan_heur_relax_t *heur, relax_t *r);
/** Computes final heuristic from the values computed in previous steps. */
static unsigned relaxHeur(plan_heur_relax_t *heur, relax_t *r);


plan_heur_relax_t *planHeurRelaxNew(const plan_operator_t *ops,
                                    size_t ops_size,
                                    const plan_var_t *var,
                                    size_t var_size,
                                    const plan_part_state_t *goal)
{
    plan_heur_relax_t *heur;

    heur = BOR_ALLOC(plan_heur_relax_t);
    heur->ops      = ops;
    heur->ops_size = ops_size;
    heur->var      = var;
    heur->var_size = var_size;
    heur->goal     = goal;

    valueIdInit(heur);
    precondInit(heur);

    return heur;
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
    unsigned h;

    relaxInit(heur, &relax);
    relaxAddInitState(heur, &relax, state);
    relaxMainLoop(heur, &relax);
    h = relaxHeur(heur, &relax);
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

        for (j = 0; j < heur->var_size; ++j){
            if (planPartStateIsSet(heur->ops[i].pre, j)){
                id = heur->val_id[j][planPartStateGet(heur->ops[i].pre, j)];
                ++heur->precond_size[id];
                heur->precond[id] = BOR_REALLOC_ARR(heur->precond[id], int,
                                                    heur->precond_size[id]);
                heur->precond[id][heur->precond_size[id] - 1] = i;

                heur->op_unsat[i] += 1;
            }
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
    r->op_unsat = BOR_ALLOC_ARR(int, heur->ops_size);
    memcpy(r->op_unsat, heur->op_unsat, sizeof(int) * heur->ops_size);

    r->op_value = BOR_ALLOC_ARR(int, heur->ops_size);
    memcpy(r->op_value, heur->op_value, sizeof(int) * heur->ops_size);

    r->facts = (fact_t *)calloc(heur->val_size, sizeof(fact_t));

    r->heap = borPairHeapNew(heapLT, NULL);
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
        r->facts[id].visited = 2;
        r->facts[id].id = id;
        borPairHeapAdd(r->heap, &r->facts[id].heap);
    }
}

static void relaxAddEffects(plan_heur_relax_t *heur, relax_t *r,
                            int op_id)
{
    const plan_operator_t *op = heur->ops + op_id;
    int i, id;

    for (i = 0; i < heur->var_size; ++i){
        if (planPartStateIsSet(op->eff, i)){
            id = heur->val_id[i][planPartStateGet(op->eff, i)];

            if (!r->facts[id].visited
                    || r->facts[id].value > r->op_value[op_id]){

                r->facts[id].value = r->op_value[op_id];
                if (r->facts[id].visited == 2){
                    borPairHeapDecreaseKey(r->heap, &r->facts[id].heap);

                }else{ // facts[id].visited in [0, 1]
                    r->facts[id].visited = 2;
                    r->facts[id].id = id;
                    borPairHeapAdd(r->heap, &r->facts[id].heap);
                }
            }
        }
    }
}

static void relaxProcessOp(plan_heur_relax_t *heur, relax_t *r,
                           int op_id, fact_t *fact)
{
    // TODO: Add/Max op
    r->op_value[op_id] += fact->value;

    // satisfy operator precondition
    r->op_unsat[op_id] = BOR_MAX(r->op_unsat[op_id] - 1, 0);

    if (r->op_unsat[op_id] == 0){
        relaxAddEffects(heur, r, op_id);
    }
}

static void relaxMainLoop(plan_heur_relax_t *heur, relax_t *r)
{
    bor_pairheap_node_t *heap_node;
    fact_t *fact;
    int i, size, *precond;

    while (!borPairHeapEmpty(r->heap)){
        heap_node = borPairHeapExtractMin(r->heap);
        fact = bor_container_of(heap_node, fact_t, heap);
        fact->visited = 1;

        size = heur->precond_size[fact->id];
        precond = heur->precond[fact->id];
        for (i = 0; i < size; ++i){
            relaxProcessOp(heur, r, precond[i], fact);
        }
    }
}

static unsigned relaxHeur(plan_heur_relax_t *heur, relax_t *r)
{
    int i, id;
    unsigned h;

    // TODO: Add/Max op
    h = 0;
    for (i = 0; i < heur->var_size; ++i){
        if (planPartStateIsSet(heur->goal, i)){
            id = heur->val_id[i][planPartStateGet(heur->goal, i)];
            h += r->facts[id].value;
        }
    }

    return h;
}
