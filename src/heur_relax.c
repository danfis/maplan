#include <boruvka/alloc.h>
#include <boruvka/list.h>
#include <boruvka/bucketheap.h>
#include "plan/heur.h"
#include "plan/prioqueue.h"

#define TYPE_ADD 0
#define TYPE_MAX 1
#define TYPE_FF  2

/**
 * Structure representing a single fact.
 */
struct _fact_t {
    plan_cost_t value;               /*!< Value assigned to the fact */
    int reached_by_op:30;            /*!< ID of the operator that reached
                                          this fact */
    unsigned goal:1;                 /*!< True if the fact is goal fact */
    unsigned relaxed_plan_visited:1; /*!< True if fact was already visited
                                          during finding a relaxed plan */
};
typedef struct _fact_t fact_t;

/**
 * Struture representing an operator
 */
struct _op_t {
    int unsat;         /*!< Number of unsatisfied preconditions */
    plan_cost_t value; /*!< Value assigned to the operator */
    plan_cost_t cost;  /*!< Cost of the operator */
};
typedef struct _op_t op_t;

/**
 * Preconditions of a fact.
 */
struct _precond_t {
    int *op;  /*!< List of operators' IDs for which the fact is
                   precondition */
    int size; /*!< Size of op[] array */
};
typedef struct _precond_t precond_t;

/**
 * Effects of an operator.
 */
struct _eff_t {
    int *fact; /*!< List of facts that are effects of the operator */
    int size;  /*!< Size of fact[] array */
};
typedef struct _eff_t eff_t;

/**
 * Structure for translation of variable's value to fact id.
 */
struct _val_to_id_t {
    int **val_id;
    int var_size;
    int val_size;
};
typedef struct _val_to_id_t val_to_id_t;


struct _plan_heur_relax_t {
    plan_heur_t heur;
    int type;

    val_to_id_t vid;

    precond_t *precond; /*!< Operators for which the corresponding fact is
                             precondition -- the size of this array equals
                             to .fact_size */
    eff_t *eff;         /*!< Unrolled effects of operators. Size of this
                             array equals to .op_size */
    eff_t *op_precond;  /*!< Precondition facts of operators. Size of this
                             array is .op_size */

    fact_t *fact_init;  /*!< Initialization values for facts */
    fact_t *fact;       /*!< List of facts */
    int fact_size;      /*!< Number of the facts */

    op_t *op_init;      /*!< Initialization values for operators */
    op_t *op;           /*!< List of operators */
    int op_size;        /*!< Number of operators */

    int goal_unsat_init; /*!< Number of unsatisfied goal variables */
    int goal_unsat;      /*!< Counter of unsatisfied goals */
    eff_t goal;          /*!< Copied goal in terms of fact ID */

    plan_prio_queue_t queue;
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

/** Initializes and frees .goal structure */
static void goalInit(plan_heur_relax_t *heur,
                     const plan_part_state_t *goal);
static void goalFree(plan_heur_relax_t *heur);
/** Initializes and frees .fact* structures */
static void factInit(plan_heur_relax_t *heur,
                     const plan_part_state_t *goal);
static void factFree(plan_heur_relax_t *heur);
/** Initializes and frees .op* srtuctures */
static void opInit(plan_heur_relax_t *heur,
                   const plan_operator_t *ops, int ops_size);
static void opFree(plan_heur_relax_t *heur);
/** Initializes and frees .eff* structures */
static void effInit(plan_heur_relax_t *heur,
                    const plan_operator_t *ops, int ops_size);
static void effFree(plan_heur_relax_t *heur);
/** Initializes and frees .precond structures */
static void opPrecondInit(plan_heur_relax_t *heur,
                          const plan_operator_t *ops, int ops_size);
static void opPrecondFree(plan_heur_relax_t *heur);
/** Removes fact id from specified position */
static void effRemoveId(eff_t *eff, int pos);
/** Remove same effects where its cost is higher */
static void opSimplifyEffects(eff_t *ref_eff, eff_t *op_eff,
                              plan_cost_t ref_cost, plan_cost_t op_cost);
/** Simplifies internal representation of operators so that there are no
 *  two operators applicable in the same time with the same effect. */
static void opSimplify(plan_heur_relax_t *heur,
                       const plan_operator_t *op,
                       const plan_succ_gen_t *succ_gen,
                       plan_state_pool_t *state_pool);
/** Initializes and frees .precond structures */
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
    heur->type = type;

    valToIdInit(&heur->vid, prob->var, prob->var_size);
    goalInit(heur, prob->goal);
    factInit(heur, prob->goal);
    opInit(heur, prob->op, prob->op_size);
    opPrecondInit(heur, prob->op, prob->op_size);
    effInit(heur, prob->op, prob->op_size);
    opSimplify(heur, prob->op, prob->succ_gen, prob->state_pool);
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
    precondFree(heur);
    effFree(heur);
    opPrecondFree(heur);
    opFree(heur);
    factFree(heur);
    goalFree(heur);
    valToIdFree(&heur->vid);

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

static void goalInit(plan_heur_relax_t *heur,
                     const plan_part_state_t *goal)
{
    int i, id;
    plan_var_id_t var;
    plan_val_t val;

    heur->goal.size = goal->vals_size;
    heur->goal.fact = BOR_ALLOC_ARR(int, heur->goal.size);
    PLAN_PART_STATE_FOR_EACH(goal, i, var, val){
        id = valToId(&heur->vid, var, val);
        heur->goal.fact[i] = id;
    }
}

static void goalFree(plan_heur_relax_t *heur)
{
    if (heur->goal.fact)
        BOR_FREE(heur->goal.fact);
}

static void factInit(plan_heur_relax_t *heur,
                     const plan_part_state_t *goal)
{
    int i, id;
    fact_t *fact;
    plan_var_id_t var;
    plan_val_t val;

    // prepare fact arrays
    heur->fact_size = valToIdSize(&heur->vid);
    heur->fact_init = BOR_ALLOC_ARR(fact_t, heur->fact_size);
    heur->fact      = BOR_ALLOC_ARR(fact_t, heur->fact_size);

    // set up initial values
    for (i = 0; i < heur->fact_size; ++i){
        fact = heur->fact_init + i;
        fact->value = -1;
        fact->reached_by_op = -1;
        fact->goal = 0;
        fact->relaxed_plan_visited = 0;
    }

    // set up goal
    heur->goal_unsat_init = 0;
    PLAN_PART_STATE_FOR_EACH(goal, i, var, val){
        id = valToId(&heur->vid, var, val);
        heur->fact_init[id].goal = 1;
        ++heur->goal_unsat_init;
    }
}

static void factFree(plan_heur_relax_t *heur)
{
    BOR_FREE(heur->fact_init);
    BOR_FREE(heur->fact);
}

static void opInit(plan_heur_relax_t *heur,
                   const plan_operator_t *ops, int ops_size)
{
    int i;

    // prepare operator arrays
    heur->op_size = ops_size;
    heur->op = BOR_ALLOC_ARR(op_t, heur->op_size);
    heur->op_init = BOR_ALLOC_ARR(op_t, heur->op_size);

    for (i = 0; i < ops_size; ++i){
        heur->op_init[i].unsat = ops[i].pre->vals_size;
        heur->op_init[i].value = ops[i].cost;
        heur->op_init[i].cost  = ops[i].cost;
    }
}

static void opFree(plan_heur_relax_t *heur)
{
    BOR_FREE(heur->op_init);
    BOR_FREE(heur->op);
}

static void effInit(plan_heur_relax_t *heur,
                    const plan_operator_t *ops, int ops_size)
{
    int i, j;
    plan_var_id_t var;
    plan_val_t val;
    eff_t *eff;

    eff = heur->eff = BOR_ALLOC_ARR(eff_t, ops_size);
    for (i = 0; i < ops_size; ++i, ++eff){
        eff->size = ops[i].eff->vals_size;
        eff->fact = BOR_ALLOC_ARR(int, eff->size);
        PLAN_PART_STATE_FOR_EACH(ops[i].eff, j, var, val){
            eff->fact[j] = valToId(&heur->vid, var, val);
        }
    }
}

static void effFree(plan_heur_relax_t *heur)
{
    int i;

    for (i = 0; i < heur->op_size; ++i)
        BOR_FREE(heur->eff[i].fact);
    BOR_FREE(heur->eff);
}

static void opPrecondInit(plan_heur_relax_t *heur,
                          const plan_operator_t *ops, int ops_size)
{
    int i, j, id;
    plan_var_id_t var;
    plan_val_t val;

    heur->op_precond = BOR_ALLOC_ARR(eff_t, ops_size);
    for (i = 0; i < ops_size; ++i){
        heur->op_precond[i].size = ops[i].pre->vals_size;
        heur->op_precond[i].fact = BOR_ALLOC_ARR(int, heur->op_precond[i].size);

        PLAN_PART_STATE_FOR_EACH(ops[i].pre, j, var, val){
            id = valToId(&heur->vid, var, val);
            heur->op_precond[i].fact[j] = id;
        }
    }
}

static void opPrecondFree(plan_heur_relax_t *heur)
{
    int i;

    for (i = 0; i < heur->op_size; ++i)
        BOR_FREE(heur->op_precond[i].fact);
    BOR_FREE(heur->op_precond);
}

static void effRemoveId(eff_t *eff, int pos)
{
    int i;
    for (i = pos + 1; i < eff->size; ++i)
        eff->fact[i - 1] = eff->fact[i];
    --eff->size;
}

static void opSimplifyEffects(eff_t *ref_eff, eff_t *op_eff,
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

static void opSimplify(plan_heur_relax_t *heur,
                       const plan_operator_t *op,
                       const plan_succ_gen_t *succ_gen,
                       plan_state_pool_t *state_pool)
{
    int i, ref_i, op_i;
    const plan_operator_t *ref_op; // reference operator
    plan_operator_t **ops;   // list of operators applicable in ref_op->pre
    int ops_size;            // number of applicable operators
    plan_state_t *state;     // pre-allocated state

    // Allocate array for applicable operators
    ops = BOR_ALLOC_ARR(plan_operator_t *, heur->op_size);
    // Pre-allocate state
    state = planStateNew(state_pool);

    for (ref_i = 0; ref_i < heur->op_size; ++ref_i){
        ref_op = op + ref_i;

        // skip operators with no effects
        if (heur->eff[ref_i].size == 0)
            continue;

        // convert preconditions of reference operator to a state
        planPartStateToState(ref_op->pre, state);

        // get all applicable operators
        ops_size = planSuccGenFind(succ_gen, state, ops, heur->op_size);

        for (i = 0; i < ops_size; ++i){
            // don't compare two identical operators
            if (ref_op == ops[i])
                continue;

            // compute id of the operator
            op_i = ((unsigned long)ops[i] - (unsigned long)op)
                        / sizeof(plan_operator_t);

            // simplify effects, if possible delete in reference operator
            opSimplifyEffects(heur->eff + ref_i, heur->eff + op_i,
                              ref_op->cost, ops[i]->cost);
        }
    }

    // Free allocated resources
    planStateDel(state);
    BOR_FREE(ops);
}

static void precondInit(plan_heur_relax_t *heur)
{
    int i, opi, fact_id;
    eff_t *pre, *pre_end;
    precond_t *precond;

    heur->precond = BOR_CALLOC_ARR(precond_t, valToIdSize(&heur->vid));

    pre_end = heur->op_precond + heur->op_size;
    for (pre = heur->op_precond, opi = 0; pre < pre_end; ++pre, ++opi){
        if (heur->eff[opi].size == 0)
            continue;

        for (i = 0; i < pre->size; ++i){
            fact_id = pre->fact[i];
            precond = heur->precond + fact_id;

            ++precond->size;
            precond->op = BOR_REALLOC_ARR(precond->op, int, precond->size);
            precond->op[precond->size - 1] = opi;
        }
    }
}

static void precondFree(plan_heur_relax_t *heur)
{
    int i;

    for (i = 0; i < heur->fact_size; ++i)
        BOR_FREE(heur->precond[i].op);
    BOR_FREE(heur->precond);
}






static void ctxInit(plan_heur_relax_t *heur)
{
    memcpy(heur->op, heur->op_init, sizeof(op_t) * heur->op_size);
    memcpy(heur->fact, heur->fact_init, sizeof(fact_t) * heur->fact_size);
    heur->goal_unsat = heur->goal_unsat_init;
    planPrioQueueInit(&heur->queue);
}

static void ctxFree(plan_heur_relax_t *heur)
{
    planPrioQueueFree(&heur->queue);
}

static void ctxAddInitState(plan_heur_relax_t *heur,
                            const plan_state_t *state)
{
    int i, id, len;

    // insert all facts from the initial state into priority queue
    len = planStateSize(state);
    for (i = 0; i < len; ++i){
        id = valToId(&heur->vid, i, planStateGet(state, i));
        heur->fact[id].value = 0;
        planPrioQueuePush(&heur->queue, heur->fact[id].value, id);
        heur->fact[id].reached_by_op = -1;
    }
}

static void ctxAddEffects(plan_heur_relax_t *heur, int op_id)
{
    int i, id;
    fact_t *fact;
    int *eff_facts, eff_facts_len;
    int op_value = heur->op[op_id].value;

    eff_facts = heur->eff[op_id].fact;
    eff_facts_len = heur->eff[op_id].size;
    for (i = 0; i < eff_facts_len; ++i){
        id = eff_facts[i];
        fact = heur->fact + id;

        if (fact->value == -1 || fact->value > op_value){
            fact->value = op_value;
            fact->reached_by_op = op_id;
            planPrioQueuePush(&heur->queue, fact->value, id);
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
    heur->op[op_id].value = relaxHeurOpValue(heur->type,
                                             heur->op[op_id].cost,
                                             heur->op[op_id].value,
                                             fact->value);

    // satisfy operator precondition
    heur->op[op_id].unsat = BOR_MAX(heur->op[op_id].unsat - 1, 0);

    if (heur->op[op_id].unsat == 0){
        ctxAddEffects(heur, op_id);
    }
}

static int ctxMainLoop(plan_heur_relax_t *heur)
{
    fact_t *fact;
    int i, size, *precond, id;
    plan_cost_t value;

    while (!planPrioQueueEmpty(&heur->queue)){
        id = planPrioQueuePop(&heur->queue, &value);
        fact = heur->fact + id;
        if (fact->value != value)
            continue;

        if (fact->goal){
            fact->goal = 0;
            --heur->goal_unsat;
            if (heur->goal_unsat == 0){
                return 0;
            }
        }

        size = heur->precond[id].size;
        precond = heur->precond[id].op;
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

    h = PLAN_COST_ZERO;
    for (i = 0; i < heur->goal.size; ++i){
        id = heur->goal.fact[i];
        h = relaxHeurValue(heur->type, h, heur->fact[id].value);
    }

    return h;
}

static void markRelaxedPlan(plan_heur_relax_t *heur, int *relaxed_plan, int id)
{
    fact_t *fact = heur->fact + id;
    int i, *op_precond, op_precond_size, id2;

    if (!fact->relaxed_plan_visited && fact->reached_by_op != -1){
        fact->relaxed_plan_visited = 1;

        op_precond      = heur->op_precond[fact->reached_by_op].fact;
        op_precond_size = heur->op_precond[fact->reached_by_op].size;
        for (i = 0; i < op_precond_size; ++i){
            id2 = op_precond[i];

            if (!heur->fact[id2].relaxed_plan_visited
                    && heur->fact[id2].reached_by_op != -1){
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

    relaxed_plan = BOR_CALLOC_ARR(int, heur->op_size);

    for (i = 0; i < heur->goal.size; ++i){
        id = heur->goal.fact[i];
        markRelaxedPlan(heur, relaxed_plan, id);
    }

    for (i = 0; i < heur->op_size; ++i){
        if (relaxed_plan[i]){
            h += heur->op[i].cost;
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
