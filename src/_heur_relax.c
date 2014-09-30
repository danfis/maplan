#include <boruvka/alloc.h>
#include <boruvka/rbtree_int.h>

#include "plan/heur.h"
#include "plan/prioqueue.h"

#define TYPE_ADD   0
#define TYPE_MAX   1
#define TYPE_FF    2

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
#define HEUR_FACT_OP_FACT_T
#define HEUR_FACT_OP_FACT_INIT(fact) \
    (fact)->value = -1; \
    (fact)->reached_by_op = -1; \
    (fact)->goal = 0; \
    (fact)->relaxed_plan_visited = 0

#include "_heur.c"

struct _plan_heur_relax_t {
    int type;

    heur_fact_op_t data;

    const plan_op_t *base_op; /*!< Pointer to the input operators.
                                   This value is used for computing
                                   ID of the operator based on the
                                   value of its pointer. */
    op_t *op;
    fact_t *fact;
    int *relaxed_plan;   /*!< Prepared array for relaxed plan */

    int goal_unsat_init; /*!< Number of unsatisfied goal variables */
    int goal_unsat;      /*!< Counter of unsatisfied goals */
    factarr_t goal;      /*!< Current goal */
    plan_prio_queue_t queue;
};
typedef struct _plan_heur_relax_t plan_heur_relax_t;

/** Initializes main structure for computing relaxed solution. */
static void ctxInit(plan_heur_relax_t *heur, const plan_part_state_t *goal);
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
static plan_cost_t ctxHeur(plan_heur_relax_t *heur, plan_heur_res_t *pref);

/** Sets heur->relaxed_plan[i] to 1 if i'th operator is in relaxed plan and
 *  to 0 if it is not. */
static void markRelaxedPlan(plan_heur_relax_t *heur);


struct _pref_ops_selector_t {
    plan_heur_res_t *pref_ops; /*!< In/Out data structure */
    plan_op_t *base_op;  /*!< Base pointer to the source
                                    operator array. */
    plan_op_t **cur;     /*!< Cursor pointing to the next
                                    operator in .pref_ops->op[] */
    plan_op_t **end;     /*!< Points after .pref_ops->op[] */
    plan_op_t **ins;     /*!< Next insert position for
                                    preferred operator. */
};
typedef struct _pref_ops_selector_t pref_ops_selector_t;

/** This macro is here only to shut gcc compiler which is stupidly
 *  complaining about "may-be uninitialized" members of this struct. */
#define PREF_OPS_SELECTOR(name) \
    pref_ops_selector_t name = { NULL, NULL, NULL, NULL, NULL }

/** Initializes preferred-operator-selector. */
static void prefOpsSelectorInit(pref_ops_selector_t *selector,
                                plan_heur_res_t *pref_ops,
                                const plan_op_t *base_op);
/** Finalizes selecting of preferred operators */
static void prefOpsSelectorFinalize(pref_ops_selector_t *sel);
/** Mark the specified operator as preferred operator. This function must
 *  be called only with increasing values of op_id! */
static void prefOpsSelectorMarkPreferredOp(pref_ops_selector_t *sel,
                                           int op_id);

/** Main function that returns heuristic value. */
BOR_UNUSED(static void planHeurRelax(plan_heur_relax_t *heur,
                                     const plan_state_t *state,
                                     plan_heur_res_t *res));
static void planHeurRelax2(plan_heur_relax_t *heur,
                           const plan_state_t *state,
                           const plan_part_state_t *goal,
                           plan_heur_res_t *res);


static void planHeurRelaxInit(plan_heur_relax_t *heur, int type,
                              const plan_var_t *var, int var_size,
                              const plan_part_state_t *goal,
                              const plan_op_t *op, int op_size,
                              const plan_succ_gen_t *succ_gen)
{
    unsigned flags;
    int i;

    heur->type = type;
    heur->base_op = op;

    flags  = HEUR_FACT_OP_SIMPLIFY;
    flags |= HEUR_FACT_OP_NO_PRE_FACT;
    heurFactOpInit(&heur->data, var, var_size, goal,
                   op, op_size, succ_gen, flags);

    heur->op = BOR_ALLOC_ARR(op_t, heur->data.op_size);
    heur->fact = BOR_ALLOC_ARR(fact_t, heur->data.fact_size);
    heur->relaxed_plan = BOR_ALLOC_ARR(int, op_size);

    // set up goal
    heur->goal_unsat_init = heur->data.goal.size;
    for (i = 0; i < heur->data.goal.size; ++i){
        heur->data.fact[heur->data.goal.fact[i]].goal = 1;
    }

    // Preallocate goal fact-array
    heur->goal.fact = BOR_ALLOC_ARR(int, var_size);
}

static void planHeurRelaxFree(plan_heur_relax_t *heur)
{
    BOR_FREE(heur->fact);
    BOR_FREE(heur->op);
    BOR_FREE(heur->relaxed_plan);
    heurFactOpFree(&heur->data);
    if (heur->goal.fact)
        BOR_FREE(heur->goal.fact);
}

static void planHeurRelax(plan_heur_relax_t *heur, const plan_state_t *state,
                          plan_heur_res_t *res)
{
    planHeurRelax2(heur, state, NULL, res);
}

static void planHeurRelax2(plan_heur_relax_t *heur,
                           const plan_state_t *state,
                           const plan_part_state_t *goal,
                           plan_heur_res_t *res)
{
    plan_cost_t h = PLAN_HEUR_DEAD_END;

    ctxInit(heur, goal);
    ctxAddInitState(heur, state);
    if (ctxMainLoop(heur) == 0){
        h = ctxHeur(heur, res);
    }
    ctxFree(heur);

    res->heur = h;
}


static void ctxInit(plan_heur_relax_t *heur, const plan_part_state_t *goal)
{
    memcpy(heur->op, heur->data.op, sizeof(op_t) * heur->data.op_size);
    memcpy(heur->fact, heur->data.fact, sizeof(fact_t) * heur->data.fact_size);
    memcpy(heur->goal.fact, heur->data.goal.fact,
           sizeof(int) * heur->data.goal.size);
    heur->goal.size = heur->data.goal.size;
    heur->goal_unsat = heur->goal_unsat_init;
    planPrioQueueInit(&heur->queue);

    if (goal != NULL){
        int i, id;
        plan_var_id_t var;
        plan_val_t val;

        // Reset goal flags in .fact[] array
        for (i = 0; i < heur->data.fact_size; ++i)
            heur->fact[i].goal = 0;

        // Correctly set goal flags in .fact[] array and goal IDs in .goal
        PLAN_PART_STATE_FOR_EACH(goal, i, var, val){
            id = valToId(&heur->data.vid, var, val);
            heur->fact[id].goal = 1;
            heur->goal.fact[i] = id;
        }
        heur->goal.size = i;

        // Change number of unsatisfied goals
        heur->goal_unsat = goal->vals_size;
    }
}

static void ctxFree(plan_heur_relax_t *heur)
{
    planPrioQueueFree(&heur->queue);
}

_bor_inline void ctxUpdateFact(plan_heur_relax_t *heur, int fact_id,
                               int op_id, int op_value)
{
    fact_t *fact = heur->fact + fact_id;

    fact->value = op_value;
    fact->reached_by_op = op_id;

    // Insert only those facts that can be used later, i.e., can satisfy
    // goal or some operators have them as preconditions.
    // It should speed it up a little.
    if (fact->goal || heur->data.fact_pre[fact_id].size > 0)
        planPrioQueuePush(&heur->queue, fact->value, fact_id);
}

static void ctxAddInitState(plan_heur_relax_t *heur,
                            const plan_state_t *state)
{
    int i, id, len;

    // insert all facts from the initial state into priority queue
    len = planStateSize(state);
    for (i = 0; i < len; ++i){
        id = valToId(&heur->data.vid, i, planStateGet(state, i));
        ctxUpdateFact(heur, id, -1, 0);
    }
    // Deal with operators w/o preconditions
    ctxUpdateFact(heur, heur->data.no_pre_fact, -1, 0);
}

static void ctxAddEffects(plan_heur_relax_t *heur, int op_id)
{
    int i, id;
    fact_t *fact;
    int *eff_facts, eff_facts_len;
    int op_value = heur->op[op_id].value;

    eff_facts = heur->data.op_eff[op_id].fact;
    eff_facts_len = heur->data.op_eff[op_id].size;
    for (i = 0; i < eff_facts_len; ++i){
        id = eff_facts[i];
        fact = heur->fact + id;

        if (fact->value == -1 || fact->value > op_value){
            ctxUpdateFact(heur, id, op_id, op_value);
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

        size = heur->data.fact_pre[id].size;
        precond = heur->data.fact_pre[id].op;
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

static plan_cost_t relaxHeurAddMax(plan_heur_relax_t *heur,
                                   plan_heur_res_t *pref_ops)
{
    int i, id;
    plan_cost_t h;

    h = PLAN_COST_ZERO;
    for (i = 0; i < heur->goal.size; ++i){
        id = heur->goal.fact[i];
        h = relaxHeurValue(heur->type, h, heur->fact[id].value);
    }

    if (pref_ops->pref_op != NULL){
        pref_ops_selector_t pref_ops_selector;
        markRelaxedPlan(heur);
        prefOpsSelectorInit(&pref_ops_selector, pref_ops, heur->base_op);
        for (i = 0; i < heur->data.actual_op_size; ++i){
            if (heur->relaxed_plan[i])
                prefOpsSelectorMarkPreferredOp(&pref_ops_selector, i);
        }
        prefOpsSelectorFinalize(&pref_ops_selector);
    }

    return h;
}




static plan_cost_t relaxHeurFF(plan_heur_relax_t *heur,
                               plan_heur_res_t *preferred_ops)
{
    int i;
    plan_cost_t h = PLAN_COST_ZERO;
    PREF_OPS_SELECTOR(pref_ops_selector);

    if (preferred_ops->pref_op != NULL){
        prefOpsSelectorInit(&pref_ops_selector, preferred_ops,
                            heur->base_op);
    }

    markRelaxedPlan(heur);
    for (i = 0; i < heur->data.actual_op_size; ++i){
        if (heur->relaxed_plan[i]){
            if (preferred_ops->pref_op != NULL){
                prefOpsSelectorMarkPreferredOp(&pref_ops_selector, i);
            }

            h += heur->op[i].cost;
        }
    }

    if (preferred_ops->pref_op != NULL){
        prefOpsSelectorFinalize(&pref_ops_selector);
    }

    return h;
}

static plan_cost_t ctxHeur(plan_heur_relax_t *heur,
                           plan_heur_res_t *preferred_ops)
{
    if (heur->type == TYPE_ADD || heur->type == TYPE_MAX){
        return relaxHeurAddMax(heur, preferred_ops);
    }else{ // heur->type == TYPE_FF
        return relaxHeurFF(heur, preferred_ops);
    }
}


static void _markRelaxedPlan(plan_heur_relax_t *heur,
                             int *relaxed_plan, int id)
{
    fact_t *fact = heur->fact + id;
    int i, *op_precond, op_precond_size, id2;

    if (!fact->relaxed_plan_visited && fact->reached_by_op != -1){
        fact->relaxed_plan_visited = 1;

        op_precond      = heur->data.op_pre[fact->reached_by_op].fact;
        op_precond_size = heur->data.op_pre[fact->reached_by_op].size;
        for (i = 0; i < op_precond_size; ++i){
            id2 = op_precond[i];

            if (!heur->fact[id2].relaxed_plan_visited
                    && heur->fact[id2].reached_by_op != -1){
                _markRelaxedPlan(heur, relaxed_plan, id2);
            }
        }

        relaxed_plan[heur->data.op_id[fact->reached_by_op]] = 1;
    }
}

static void markRelaxedPlan(plan_heur_relax_t *heur)
{
    int i, id;

    bzero(heur->relaxed_plan, sizeof(int) * heur->data.actual_op_size);
    for (i = 0; i < heur->goal.size; ++i){
        id = heur->goal.fact[i];
        _markRelaxedPlan(heur, heur->relaxed_plan, id);
    }
}


static int sortPreferredOpsByPtrCmp(const void *a, const void *b)
{
    const plan_op_t *op1 = *(const plan_op_t **)a;
    const plan_op_t *op2 = *(const plan_op_t **)b;
    return op1 - op2;
}

static void prefOpsSelectorInit(pref_ops_selector_t *selector,
                                plan_heur_res_t *pref_ops,
                                const plan_op_t *base_op)
{
    selector->pref_ops = pref_ops;
    selector->base_op  = (plan_op_t *)base_op;

    // Sort array of operators by their pointers.
    if (pref_ops->pref_op_size > 0){
        qsort(pref_ops->pref_op, pref_ops->pref_op_size,
              sizeof(plan_op_t *), sortPreferredOpsByPtrCmp);
    }

    // Set up cursors
    selector->cur = pref_ops->pref_op;
    selector->end = pref_ops->pref_op + pref_ops->pref_op_size;
    selector->ins = pref_ops->pref_op;
}

static void prefOpsSelectorFinalize(pref_ops_selector_t *sel)
{
    // Compute number of preferred operators from .ins pointer
    sel->pref_ops->pref_size = sel->ins - sel->pref_ops->pref_op;
}

static void prefOpsSelectorMarkPreferredOp(pref_ops_selector_t *sel,
                                           int op_id)
{
    plan_op_t *op = sel->base_op + op_id;
    plan_op_t *op_swp;

    // Skip operators with lower ID
    for (; sel->cur < sel->end && *sel->cur < op; ++sel->cur);
    if (sel->cur == sel->end)
        return;

    if (*sel->cur == op){
        // If we found corresponding operator -- move it to the beggining
        // of the .op[] array.
        if (sel->ins != sel->cur){
            BOR_SWAP(*sel->ins, *sel->cur, op_swp);
        }
        ++sel->ins;
        ++sel->cur;
    }
}
