#include <boruvka/alloc.h>
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

/** Forward declaration */
typedef struct _op_t op_t;

/**
 * Structure representing a single fact (proposition).
 */
struct _fact_t {
    plan_cost_t value;
    unsigned state;
    op_t *reached_by;

    int id;
    op_t **precond;
    int precond_size;
};
typedef struct _fact_t fact_t;

struct _op_t {
    int unsat;         /*!< Number of unsatisfied preconditions */
    plan_cost_t value; /*!< Current value associated with the operator */

    const plan_operator_t *op; /*!< Pointer to the actual operator */
    fact_t **eff; /*!< Array of effect facts */
    int eff_size; /*!< Number of effect facts */
    int num_precond; /*!< Number of preconditions */
};

struct _val_to_id_t {
    int **val_id;
    int var_size;
    int val_size;
};
typedef struct _val_to_id_t val_to_id_t;

/**
 * Main structure.
 */
struct _plan_heur_relax_t {
    plan_heur_t heur;
    const plan_operator_t *ops;
    int ops_size;
    const plan_var_t *var;
    int var_size;
    const plan_part_state_t *goal;

    int type; /*!< Type of the heuristic, see TYPE_* consts */

    val_to_id_t val_to_id;
    fact_t *fact;
    int fact_size;
    op_t *op;
    int op_size;
    int goal_unsat; /*!< Number of unsatisfied goals */

    bheap_t bheap;
};
typedef struct _plan_heur_relax_t plan_heur_relax_t;

/** Main function that returns heuristic value. */
static plan_cost_t planHeurRelax(void *heur, const plan_state_t *state);
/** Delete method */
static void planHeurRelaxDel(void *_heur);

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



static void dataInit(plan_heur_relax_t *heur)
{
    int i, j, id;
    plan_var_id_t var;
    plan_val_t val;
    fact_t *fact;

    // Initialize facts
    heur->fact_size = valToIdSize(&heur->val_to_id);
    heur->fact = BOR_ALLOC_ARR(fact_t, heur->fact_size);
    for (i = 0; i < heur->fact_size; ++i){
        heur->fact[i].id = i;
        heur->fact[i].precond = 0;
        heur->fact[i].precond_size = 0;
    }

    // Initialize operators
    heur->op_size = heur->ops_size;
    heur->op = BOR_ALLOC_ARR(op_t, heur->op_size);
    for (i = 0; i < heur->op_size; ++i){
        heur->op[i].num_precond = 0;
        heur->op[i].op = heur->ops + i;
        heur->op[i].eff = NULL;
        heur->op[i].eff_size = 0;
    }

    // create mapping between precondition and operator
    for (i = 0; i < heur->ops_size; ++i){
        PLAN_PART_STATE_FOR_EACH(heur->ops[i].pre, j, var, val){
            id = valToId(&heur->val_to_id, var, val);
            fact = heur->fact + id;

            ++fact->precond_size;
            fact->precond = BOR_REALLOC_ARR(fact->precond, op_t *,
                                            fact->precond_size);
            fact->precond[fact->precond_size - 1] = heur->op + i;
            heur->op[i].num_precond += 1;
        }
    }

    // unroll all effects
    for (i = 0; i < heur->op_size; ++i){
        heur->op[i].eff_size = heur->ops[i].eff->vals_size;
        heur->op[i].eff = BOR_ALLOC_ARR(fact_t *, heur->op[i].eff_size);
        PLAN_PART_STATE_FOR_EACH(heur->ops[i].eff, j, var, val){
            id = valToId(&heur->val_to_id, var, val);
            heur->op[i].eff[j] = heur->fact + id;
        }
    }
}

void dataFree(plan_heur_relax_t *heur)
{
    int i;

    for (i = 0; i < heur->fact_size; ++i)
        BOR_FREE(heur->fact[i].precond);
    BOR_FREE(heur->fact);

    for (i = 0; i < heur->op_size; ++i)
        BOR_FREE(heur->op[i].eff);
    BOR_FREE(heur->op);
}

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

    valToIdInit(&heur->val_to_id, heur->var, heur->var_size);
    dataInit(heur);

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
    dataFree(heur);
    valToIdFree(&heur->val_to_id);
    BOR_FREE(heur);
}

static void init(plan_heur_relax_t *heur, const plan_state_t *state)
{
    int i, id;
    plan_var_id_t var;
    plan_val_t val;
    fact_t *fact;

    for (i = 0; i < heur->op_size; ++i){
        heur->op[i].unsat = heur->op[i].num_precond;
        heur->op[i].value = 0;
    }

    for (i = 0; i < heur->fact_size; ++i){
        heur->fact[i].value = 0;
        heur->fact[i].state = 0;
        heur->fact[i].reached_by = NULL;
    }

    // initialize heap
    bheapInit(&heur->bheap);

    // set up goal facts
    heur->goal_unsat = 0;
    PLAN_PART_STATE_FOR_EACH(heur->goal, i, var, val){
        id = valToId(&heur->val_to_id, var, val);
        fact = heur->fact + id;
        FACT_SET_GOAL(fact);
        ++heur->goal_unsat;
    }

    // set up initial state
    for (i = 0; i < heur->var_size; ++i){
        id = valToId(&heur->val_to_id, i, planStateGet(state, i));
        fact = heur->fact + id;
        fact->value = 0;
        bheapPush(&heur->bheap, 0, id);
    }
}

static void finalize(plan_heur_relax_t *heur)
{
    bheapFree(&heur->bheap);
}

_bor_inline plan_cost_t opValue(int type,
                                plan_cost_t op_cost,
                                plan_cost_t op_value,
                                plan_cost_t fact_value)
{
    if (type == TYPE_ADD || type == TYPE_FF)
        return op_value + fact_value;
    return BOR_MAX(op_cost + fact_value, op_value);
}

static void addEffects(plan_heur_relax_t *heur, op_t *op)
{
    int i;
    fact_t *fact;

    for (i = 0; i < op->eff_size; ++i){
        fact = op->eff[i];
        if (fact->value == -1 || fact->value > op->value){
            fact->value = op->value;
            fact->reached_by = op;
            bheapPush(&heur->bheap, fact->value, fact->id);
        }
    }
}

static void processOp(plan_heur_relax_t *heur,
                      op_t *op, fact_t *fact)
{
    op->value = opValue(heur->type, op->op->cost,
                        op->value, fact->value);

    op->unsat = BOR_MAX(op->unsat - 1, 0);
    if (op->unsat == 0){
        addEffects(heur, op);
    }
}

static int mainLoop(plan_heur_relax_t *heur)
{
    int i, id, value;
    fact_t *fact;

    while (!bheapEmpty(&heur->bheap)){
        id = bheapPop(&heur->bheap, &value);
        fact = heur->fact + id;

        if (fact->value != value)
            continue;

        if (FACT_GOAL(fact)){
            FACT_UNSET_GOAL(fact);
            --heur->goal_unsat;
            if (heur->goal_unsat == 0){
                return 0;
            }
        }

        for (i = 0; i < fact->precond_size; ++i){
            processOp(heur, fact->precond[i], fact);
        }
    }
    return -1;
}

_bor_inline plan_cost_t heurValue(int type,
                                  plan_cost_t value1,
                                  plan_cost_t value2)
{
    if (type == TYPE_ADD)
        return value1 + value2;
    return BOR_MAX(value1, value2);
}

static plan_cost_t heurAddMax(plan_heur_relax_t *heur)
{
    int i, id;
    plan_cost_t h;
    plan_var_id_t var;
    plan_val_t val;

    h = PLAN_COST_ZERO;
    PLAN_PART_STATE_FOR_EACH(heur->goal, i, var, val){
        id = valToId(&heur->val_to_id, var, val);
        h = heurValue(heur->type, h, heur->fact[id].value);
    }

    return h;
}

static plan_cost_t computeHeur(plan_heur_relax_t *heur)
{
    if (heur->type == TYPE_ADD || heur->type == TYPE_MAX){
        return heurAddMax(heur);
    }else{ // heur->type == TYPE_FF
        // TODO
        //return heurFF(heur);
        return 0;
    }
}


static plan_cost_t planHeurRelax(void *_heur, const plan_state_t *state)
{
    plan_heur_relax_t *heur = _heur;
    plan_cost_t h = PLAN_HEUR_DEAD_END;

    init(heur, state);
    if (mainLoop(heur) == 0){
        h = computeHeur(heur);
    }
    finalize(heur);

    return h;
}
