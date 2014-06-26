#include <boruvka/alloc.h>
#include <boruvka/list.h>
#include <boruvka/bucketheap.h>
#include "plan/heur.h"

struct _bucket_t {
    int *value;
    int size;
    int alloc;
};
typedef struct _bucket_t bucket_t;

struct _bheap_t {
    bucket_t *bucket;
    int bucket_size;
    int lowest_key;
    int size;
};
typedef struct _bheap_t bheap_t;

void bheapInit(bheap_t *h)
{
    h->bucket_size = 1024;
    h->bucket = BOR_ALLOC_ARR(bucket_t, h->bucket_size);
    bzero(h->bucket, sizeof(bucket_t) * h->bucket_size);
    h->lowest_key = 1025;
    h->size = 0;
}

void bheapFree(bheap_t *h)
{
    int i;

    for (i = 0; i < h->bucket_size; ++i){
        if (h->bucket[i].value)
            BOR_FREE(h->bucket[i].value);
    }
    BOR_FREE(h->bucket);
}

void bheapPush(bheap_t *h, int key, int value)
{
    bucket_t *bucket;

    if (key > 1024){
        fprintf(stderr, "EPIC FAIL: :)\n");
        exit(-1);
    }

    bucket = h->bucket + key;
    if (bucket->value == NULL){
        bucket->alloc = 64;
        bucket->value = BOR_ALLOC_ARR(int, bucket->alloc);
    }else if (bucket->size + 1 > bucket->alloc){
        bucket->alloc *= 2;
        bucket->value = BOR_REALLOC_ARR(bucket->value,
                                        int, bucket->alloc);
    }
    bucket->value[bucket->size++] = value;
    ++h->size;

    if (key < h->lowest_key)
        h->lowest_key = key;
}

int bheapPop(bheap_t *h, int *key)
{
    bucket_t *bucket;
    int val;

    bucket = h->bucket + h->lowest_key;
    while (bucket->size == 0){
        ++h->lowest_key;
        bucket += 1;
    }

    val = bucket->value[--bucket->size];
    *key = h->lowest_key;
    --h->size;
    return val;
}

int bheapEmpty(bheap_t *h)
{
    return h->size == 0;
}

#define FACT_SET_GOAL(fact) ((fact)->state |= 0x4u)
#define FACT_SET_RELAXED_PLAN_VISITED(fact) ((fact)->state |= 0x8u)
#define FACT_UNSET_GOAL(fact) ((fact)->state &= 0xbu)
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
    unsigned state;
    int reached_by_op;
};
typedef struct _fact_t fact_t;

/**
 * Context for relaxation algorithm.
 */
struct _ctx_t {
    int *op_unsat;
    plan_cost_t *op_value;
    bor_bucketheap_t *heap;
    bheap_t bheap;
    int goal_unsat;
};
typedef struct _ctx_t ctx_t;


struct _val_to_id_t {
    int **val_id;
    int var_size;
    int val_size;
};
typedef struct _val_to_id_t val_to_id_t;


struct _plan_heur_relax_t {
    plan_heur_t heur;
    const plan_operator_t *ops;
    int ops_size;
    const plan_var_t *var;
    int var_size;
    const plan_part_state_t *goal;

    val_to_id_t vid;

    int **op_fact_id;
    int **precond;     /*!< Operator IDs indexed by precondition ID */
    int *precond_size; /*!< Size of each subarray */
    int *init_op_unsat;     /*!< Preinitialized counters of unsatisfied
                            preconditions per operator */
    int *init_op_value;     /*!< Preinitialized values of operators */
    fact_t *facts;
    int facts_size;

    int type;

    ctx_t ctx;
};
typedef struct _plan_heur_relax_t plan_heur_relax_t;


/** Initializes struct for translating variable value to fact ID */
static void valToIdInit(val_to_id_t *vid,
                        const plan_var_t *var, int var_size);
/** Frees val_to_id_t resources */
static void valToIdFree(val_to_id_t *vid);
/** Translates variable value to fact ID */
_bor_inline int valToId(val_to_id_t *vid, plan_var_id_t var, plan_val_t val);
/** Returns number of fact IDs */
_bor_inline int valToIdSize(val_to_id_t *vid);

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

    valToIdInit(&heur->vid, heur->var, heur->var_size);
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
    valToIdFree(&heur->vid);
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
    int i, j;
    plan_var_id_t var;
    plan_val_t val;

    heur->op_fact_id = BOR_ALLOC_ARR(int *, heur->ops_size);
    for (i = 0; i < heur->ops_size; ++i){
        heur->op_fact_id[i] = BOR_ALLOC_ARR(int,
                heur->ops[i].eff->vals_size);
        PLAN_PART_STATE_FOR_EACH(heur->ops[i].eff, j, var, val){
            heur->op_fact_id[i][j] = valToId(&heur->vid, var, val);
        }
    }
}

static void valueIdFree(plan_heur_relax_t *heur)
{
    int i;

    for (i = 0; i < heur->ops_size; ++i)
        BOR_FREE(heur->op_fact_id[i]);
    BOR_FREE(heur->op_fact_id);
}

static void precondInit(plan_heur_relax_t *heur)
{
    int i, j, id, size;
    plan_var_id_t var;
    plan_val_t val;

    // prepare precond array
    size = valToIdSize(&heur->vid);
    heur->precond = BOR_ALLOC_ARR(int *, size);
    heur->precond_size = BOR_ALLOC_ARR(int, size);
    for (i = 0; i < size; ++i){
        heur->precond_size[i] = 0;
        heur->precond[i] = NULL;
    }

    // prepare initialization arrays
    heur->init_op_unsat = BOR_ALLOC_ARR(int, heur->ops_size);
    heur->init_op_value = BOR_ALLOC_ARR(plan_cost_t, heur->ops_size);

    // create mapping between precondition and operator
    for (i = 0; i < heur->ops_size; ++i){
        heur->init_op_unsat[i] = 0;
        heur->init_op_value[i] = heur->ops[i].cost;

        PLAN_PART_STATE_FOR_EACH(heur->ops[i].pre, j, var, val){
            id = valToId(&heur->vid, var, val);
            ++heur->precond_size[id];
            heur->precond[id] = BOR_REALLOC_ARR(heur->precond[id], int,
                                                heur->precond_size[id]);
            heur->precond[id][heur->precond_size[id] - 1] = i;

            heur->init_op_unsat[i] += 1;
        }
    }

    heur->ctx.op_unsat = BOR_ALLOC_ARR(int, heur->ops_size);
    heur->ctx.op_value = BOR_ALLOC_ARR(plan_cost_t, heur->ops_size);

    heur->facts_size = valToIdSize(&heur->vid);
    heur->facts      = BOR_ALLOC_ARR(fact_t, heur->facts_size);
}

static void precondFree(plan_heur_relax_t *heur)
{
    int i, size;
    size = valToIdSize(&heur->vid);
    for (i = 0; i < size; ++i){
        BOR_FREE(heur->precond[i]);
    }
    BOR_FREE(heur->precond);
    BOR_FREE(heur->precond_size);
    BOR_FREE(heur->init_op_unsat);
    BOR_FREE(heur->init_op_value);
    if (heur->facts)
        BOR_FREE(heur->facts);
    BOR_FREE(heur->ctx.op_value);
    BOR_FREE(heur->ctx.op_unsat);
}


static void ctxInit(plan_heur_relax_t *heur)
{
    int i, id;
    plan_var_id_t var;
    plan_val_t val;
    fact_t *fact;

    /*
    for (i = 0; i < heur->ops_size; ++i){
        heur->ctx.op_unsat[i] = heur->ops[i].pre->vals_size;
    }
    */
    memcpy(heur->ctx.op_unsat, heur->init_op_unsat, sizeof(int) * heur->ops_size);
    memcpy(heur->ctx.op_value, heur->init_op_value, sizeof(plan_cost_t) * heur->ops_size);
    for (i = 0; i < heur->facts_size; ++i){
        fact = heur->facts + i;
        fact->value = -1;
        fact->state = 0;
        fact->reached_by_op = -1;
    }

    //heur->ctx.heap = borBucketHeapNew();
    bheapInit(&heur->ctx.bheap);

    // set fact goals
    heur->ctx.goal_unsat = 0;
    PLAN_PART_STATE_FOR_EACH(heur->goal, i, var, val){
        id = valToId(&heur->vid, var, val);
        FACT_SET_GOAL(heur->facts + id);
        ++heur->ctx.goal_unsat;
    }
}

static void ctxFree(plan_heur_relax_t *heur)
{
    //borBucketHeapDel(heur->ctx.heap);
    bheapFree(&heur->ctx.bheap);
}

static void ctxAddInitState(plan_heur_relax_t *heur,
                            const plan_state_t *state)
{
    int i, id;

    // insert all facts from the initial state into priority queue
    for (i = 0; i < heur->var_size; ++i){
        id = valToId(&heur->vid, i, planStateGet(state, i));
        //heur->facts[id].id = id;
        heur->facts[id].value = 0;
        bheapPush(&heur->ctx.bheap, heur->facts[id].value, id);
        /*
        borBucketHeapAdd(heur->ctx.heap, heur->facts[id].value,
                         &heur->facts[id].heap);
        */
        heur->facts[id].reached_by_op = -1;
    }
}

static void ctxAddEffects(plan_heur_relax_t *heur, int op_id)
{
    const plan_operator_t *op = heur->ops + op_id;
    int i, id;
    fact_t *fact;
    int *eff_facts, eff_facts_len;
    int op_value = heur->ctx.op_value[op_id];

    eff_facts = heur->op_fact_id[op_id];
    eff_facts_len = op->eff->vals_size;
    for (i = 0; i < eff_facts_len; ++i){
        id = eff_facts[i];
        fact = heur->facts + id;

        if (fact->value == -1 || fact->value > op_value){
            fact->value = op_value;
            fact->reached_by_op = op_id;
            bheapPush(&heur->ctx.bheap, fact->value, id);
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
    fact_t *fact;
    int i, size, *precond, id;
    plan_cost_t value;

    while (!bheapEmpty(&heur->ctx.bheap)){
        id = bheapPop(&heur->ctx.bheap, &value);
        fact = heur->facts + id;
        if (fact->value != value)
            continue;

        if (FACT_GOAL(fact)){
            FACT_UNSET_GOAL(fact);
            --heur->ctx.goal_unsat;
            if (heur->ctx.goal_unsat == 0){
                return 0;
            }
        }

        size = heur->precond_size[id];
        precond = heur->precond[id];
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
        id = valToId(&heur->vid, var, val);
        h = relaxHeurValue(heur->type, h, heur->facts[id].value);
    }

    return h;
}

static void markRelaxedPlan(plan_heur_relax_t *heur, int *relaxed_plan, int id)
{
    fact_t *fact = heur->facts + id;
    const plan_operator_t *op;
    int i, id2;
    plan_var_id_t var;
    plan_val_t val;

    if (!FACT_RELAXED_PLAN_VISITED(fact) && fact->reached_by_op != -1){
        FACT_SET_RELAXED_PLAN_VISITED(fact);
        op = heur->ops + fact->reached_by_op;

        PLAN_PART_STATE_FOR_EACH(op->pre, i, var, val){
            id2 = valToId(&heur->vid, var, val);
            if (!FACT_RELAXED_PLAN_VISITED(heur->facts + id2)
                    && heur->facts[id2].reached_by_op != -1){
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
        id = valToId(&heur->vid, var, val);
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


static void valToIdInit(val_to_id_t *vid,
                        const plan_var_t *var, int var_size)
{
    int i, j, id;

    // allocate the array corresponding to the variables
    vid->val_id = BOR_ALLOC_ARR(int *, var_size);

    for (id = 0, i = 0; i < var_size; ++i){
        // allocate array for variable's values
        vid->val_id[i] = BOR_ALLOC_ARR(int, var[i].range);

        // fill values with IDs
        for (j = 0; j < var[i].range; ++j)
            vid->val_id[i][j] = id++;
    }

    // remember number of values
    vid->val_size = id;
    vid->var_size = var_size;
}

static void valToIdFree(val_to_id_t *vid)
{
    int i;
    for (i = 0; i < vid->var_size; ++i)
        BOR_FREE(vid->val_id[i]);
    BOR_FREE(vid->val_id);
}

_bor_inline int valToId(val_to_id_t *vid,
                        plan_var_id_t var, plan_val_t val)
{
    return vid->val_id[var][val];
}

_bor_inline int valToIdSize(val_to_id_t *vid)
{
    return vid->val_size;
}
