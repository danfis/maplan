#include <boruvka/alloc.h>
#include <boruvka/lifo.h>
#include "plan/heur.h"
#include "plan/prioqueue.h"


/**
 * Structure representing a single fact.
 */
struct _fact_t {
    plan_cost_t value;  /*!< Value assigned to the fact */
    unsigned goal_zone; /*!< True if the fact is connected with the goal
                             via zero-cost operators */
    unsigned in_queue;  /*!< True if the fact is already in queue */
};
typedef struct _fact_t fact_t;
#define HEUR_FACT_OP_FACT_T
#define HEUR_FACT_OP_FACT_INIT(fact) \
    (fact)->value = -1; \
    (fact)->goal_zone = 0; \
    (fact)->in_queue = 0

#define HEUR_FACT_OP_EXTRA_OP int supporter
#define HEUR_FACT_OP_EXTRA_OP_INIT(dst, src, is_cond_eff) \
    (dst)->supporter = -1;

#include "_heur.c"

struct _plan_heur_lm_cut_t {
    plan_heur_t heur;

    heur_fact_op_t data;

    op_t *op;
    fact_t *fact;
    plan_prio_queue_t queue;
    oparr_t cut;              /*!< Array of cut operators */
};
typedef struct _plan_heur_lm_cut_t plan_heur_lm_cut_t;

#define HEUR_FROM_PARENT(parent) \
    bor_container_of((parent), plan_heur_lm_cut_t, heur)

/** Main function that returns heuristic value. */
static plan_cost_t planHeurLMCut(plan_heur_t *heur, const plan_state_t *state,
                                 plan_heur_preferred_ops_t *preferred_ops);
/** Delete method */
static void planHeurLMCutDel(plan_heur_t *_heur);

/** Initialize context for lm-cut heuristic */
static void lmCutCtxInit(plan_heur_lm_cut_t *heur);
/** Free context for lm-cut heuristic */
static void lmCutCtxFree(plan_heur_lm_cut_t *heur);
/** Enqueue a signle fact into priority queue */
_bor_inline void lmCutEnqueue(plan_heur_lm_cut_t *heur, int fact_id, int value);
/** Enqueue all operator's effects into priority queue with the specified
 *  value. */
_bor_inline void lmCutEnqueueEffects(plan_heur_lm_cut_t *heur, int op_id,
                                     int value);
/** First relaxed exploration from the given state to the goal */
static void lmCutInitialExploration(plan_heur_lm_cut_t *heur,
                                    const plan_state_t *state);
/** Marks goal-zone, i.e., facts that are connected with the goal with
 *  zero-cost operators */
static void lmCutMarkGoalZone(plan_heur_lm_cut_t *heur, int fact_id);
/** Adds initial state facts into queue */
_bor_inline void lmCutFindCutAddInit(plan_heur_lm_cut_t *heur,
                                     const plan_state_t *state,
                                     bor_lifo_t *queue);
/** Determines if the specified operator has effect in goal-zone. If so,
 *  the operator is appended to the heur->lm_cut.cut and returns true.
 *  Otherwise false is returned. */
_bor_inline int lmCutFindCutProcessOp(plan_heur_lm_cut_t *heur, int op_id);
/** Enqueues all effects of operator (if not already in queue). */
_bor_inline void lmCutFindCutEnqueueEffects(plan_heur_lm_cut_t *heur,
                                            int op_id,
                                            bor_lifo_t *queue);
/** Finds cut operators -- fills heur->lm_cut.cut structure */
static void lmCutFindCut(plan_heur_lm_cut_t *heur, const plan_state_t *state);
/** Updates operator's supporter fact */
static void lmCutUpdateSupporter(plan_heur_lm_cut_t *heur, int op_id);
/** Incrementally explores relaxed plan */
static void lmCutIncrementalExploration(plan_heur_lm_cut_t *heur);
_bor_inline plan_cost_t lmCutUpdateOpCost(const oparr_t *cut, op_t *op);

plan_heur_t *planHeurLMCutNew(const plan_var_t *var, int var_size,
                              const plan_part_state_t *goal,
                              const plan_operator_t *op, int op_size,
                              const plan_succ_gen_t *succ_gen)
{
    plan_heur_lm_cut_t *heur;
    unsigned flags;

    heur = BOR_ALLOC(plan_heur_lm_cut_t);
    planHeurInit(&heur->heur, planHeurLMCutDel, planHeurLMCut);

    flags  = HEUR_FACT_OP_INIT_FACT_EFF;
    flags |= HEUR_FACT_OP_SIMPLIFY;
    flags |= HEUR_FACT_OP_ARTIFICIAL_GOAL;
    heurFactOpInit(&heur->data, var, var_size, goal,
                   op, op_size, succ_gen, flags);

    heur->op   = BOR_ALLOC_ARR(op_t, heur->data.op_size);
    heur->fact = BOR_ALLOC_ARR(fact_t, heur->data.fact_size);
    heur->cut.op = BOR_ALLOC_ARR(int, heur->data.op_size);
    heur->cut.size = 0;

    return &heur->heur;
}

static void planHeurLMCutDel(plan_heur_t *_heur)
{
    plan_heur_lm_cut_t *heur = HEUR_FROM_PARENT(_heur);

    BOR_FREE(heur->cut.op);
    BOR_FREE(heur->fact);
    BOR_FREE(heur->op);
    heurFactOpFree(&heur->data);
    BOR_FREE(heur);
}

static plan_cost_t planHeurLMCut(plan_heur_t *_heur, const plan_state_t *state,
                                 plan_heur_preferred_ops_t *preferred_ops)
{
    plan_heur_lm_cut_t *heur = HEUR_FROM_PARENT(_heur);
    plan_cost_t h = PLAN_HEUR_DEAD_END;
    int i;

    // Preferred operators are not supported for now
    if (preferred_ops)
        preferred_ops->preferred_size = 0;

    // Initialize context for the current run
    lmCutCtxInit(heur);

    // Perform initial exploration of relaxed plan state
    lmCutInitialExploration(heur, state);

    // Check whether the goal was reached, if so prepare output variable
    if (heur->fact[heur->data.artificial_goal].value >= 0)
        h = 0;

    while (heur->fact[heur->data.artificial_goal].value > 0){
        // Initialize fact additional flags
        for (i = 0; i < heur->data.fact_size; ++i){
            heur->fact[i].goal_zone = 0;
            heur->fact[i].in_queue = 0;
        }

        // Mark facts that are connected with the goal via zero-cost
        // operators
        lmCutMarkGoalZone(heur, heur->data.artificial_goal);

        // Find cut operators, i.e., operators connected with effects in
        // goal-zone
        lmCutFindCut(heur, state);

        // Determine the minimal cost from all cut-operators. Substract
        // this cost from their cost and add it to the final heuristic
        // value.
        h += lmCutUpdateOpCost(&heur->cut, heur->op);

        // Performat incremental exploration of relaxed plan state.
        lmCutIncrementalExploration(heur);
    }

    lmCutCtxFree(heur);

    return h;
    return 0;
}

static void lmCutCtxInit(plan_heur_lm_cut_t *heur)
{
    memcpy(heur->op, heur->data.op, sizeof(op_t) * heur->data.op_size);
    memcpy(heur->fact, heur->data.fact, sizeof(fact_t) * heur->data.fact_size);
    planPrioQueueInit(&heur->queue);
}

static void lmCutCtxFree(plan_heur_lm_cut_t *heur)
{
    planPrioQueueFree(&heur->queue);
}

_bor_inline void lmCutEnqueue(plan_heur_lm_cut_t *heur, int fact_id, int value)
{
    fact_t *fact = heur->fact + fact_id;

    if (fact->value == -1 || fact->value > value){
        fact->value = value;
        planPrioQueuePush(&heur->queue, fact->value, fact_id);
    }
}

_bor_inline void lmCutEnqueueEffects(plan_heur_lm_cut_t *heur, int op_id,
                                     int value)
{
    int i, len, *eff;
    len = heur->data.op_eff[op_id].size;
    eff = heur->data.op_eff[op_id].fact;
    for (i = 0; i < len; ++i)
        lmCutEnqueue(heur, eff[i], value);
}

static void lmCutInitialExploration(plan_heur_lm_cut_t *heur,
                                    const plan_state_t *state)
{
    int i, id, len, cost;
    int *precond;
    plan_cost_t value;
    fact_t *fact;
    op_t *op;

    // Insert initial state
    len = planStateSize(state);
    for (i = 0; i < len; ++i){
        id = valToId(&heur->data.vid, i, planStateGet(state, i));
        lmCutEnqueue(heur, id, 0);
    }

    while (!planPrioQueueEmpty(&heur->queue)){
        id = planPrioQueuePop(&heur->queue, &value);
        fact = heur->fact + id;
        if (fact->value != value)
            continue;

        len = heur->data.fact_pre[id].size;
        precond = heur->data.fact_pre[id].op;
        for (i = 0; i < len; ++i){
            op = heur->op + precond[i];
            --op->unsat;
            if (op->unsat <= 0){
                op->value = fact->value;
                op->supporter = id;
                cost = fact->value + op->cost;
                lmCutEnqueueEffects(heur, precond[i], cost);
            }
        }
    }
}

static void lmCutMarkGoalZone(plan_heur_lm_cut_t *heur, int fact_id)
{
    int i, len, *op_ids, op_id;
    op_t *op;

    if (!heur->fact[fact_id].goal_zone){
        heur->fact[fact_id].goal_zone = 1;

        len    = heur->data.fact_eff[fact_id].size;
        op_ids = heur->data.fact_eff[fact_id].op;
        for (i = 0; i < len; ++i){
            op_id = op_ids[i];
            op = heur->op + op_id;
            if (op->cost == 0 && op->supporter != -1)
                lmCutMarkGoalZone(heur, op->supporter);
        }
    }
}

_bor_inline void lmCutFindCutAddInit(plan_heur_lm_cut_t *heur,
                                     const plan_state_t *state,
                                     bor_lifo_t *queue)
{
    plan_var_id_t var, len;
    plan_val_t val;
    int id;

    len = planStateSize(state);
    for (var = 0; var < len; ++var){
        val = planStateGet(state, var);
        id = valToId(&heur->data.vid, var, val);
        heur->fact[id].in_queue = 1;
        borLifoPush(queue, &id);
    }
}

_bor_inline int lmCutFindCutProcessOp(plan_heur_lm_cut_t *heur, int op_id)
{
    int i, len, *facts, fact_id;

    len   = heur->data.op_eff[op_id].size;
    facts = heur->data.op_eff[op_id].fact;
    for (i = 0; i < len; ++i){
        fact_id = facts[i];
        if (heur->fact[fact_id].goal_zone){
            heur->cut.op[heur->cut.size++] = op_id;
            return 1;
        }
    }

    return 0;
}

_bor_inline void lmCutFindCutEnqueueEffects(plan_heur_lm_cut_t *heur,
                                            int op_id,
                                            bor_lifo_t *queue)
{
    int i, len, *facts, fact_id;

    len   = heur->data.op_eff[op_id].size;
    facts = heur->data.op_eff[op_id].fact;
    for (i = 0; i < len; ++i){
        fact_id = facts[i];
        if (!heur->fact[fact_id].in_queue){
            heur->fact[fact_id].in_queue = 1;
            borLifoPush(queue, &fact_id);
        }
    }
}

static void lmCutFindCut(plan_heur_lm_cut_t *heur, const plan_state_t *state)
{
    int i, fact_id, op_id;
    bor_lifo_t queue;

    // Reset output structure
    heur->cut.size = 0;

    // Initialize queue and adds initial state
    borLifoInit(&queue, sizeof(int));
    lmCutFindCutAddInit(heur, state, &queue);

    while (!borLifoEmpty(&queue)){
        // Pop next fact from queue
        fact_id = *(int *)borLifoBack(&queue);
        borLifoPop(&queue);

        for (i = 0; i < heur->data.fact_pre[fact_id].size; ++i){
            op_id = heur->data.fact_pre[fact_id].op[i];
            if (heur->op[op_id].supporter == fact_id){
                if (!lmCutFindCutProcessOp(heur, op_id)){
                    // Goal-zone was not reached -- add operator's effects to
                    // the queue
                    lmCutFindCutEnqueueEffects(heur, op_id, &queue);
                }
            }
        }
    }

    borLifoFree(&queue);
}

static void lmCutUpdateSupporter(plan_heur_lm_cut_t *heur, int op_id)
{
    int i, len, *precond;
    int supp;

    len = heur->data.op_pre[op_id].size;
    if (len == 0)
        return;
    precond = heur->data.op_pre[op_id].fact;
    supp = heur->op[op_id].supporter;

    for (i = 0; i < len; ++i){
        if (heur->fact[precond[i]].value > heur->fact[supp].value){
            supp = precond[i];
        }
    }

    heur->op[op_id].supporter = supp;
    heur->op[op_id].value     = heur->fact[supp].value;
}

static void lmCutIncrementalExploration(plan_heur_lm_cut_t *heur)
{
    int i, op_id, cost, id, value, len, *precond;
    int old_value, new_value;
    fact_t *fact;
    op_t *op;

    // Adds effects of cut-operators to the queue
    for (i = 0; i < heur->cut.size; ++i){
        op_id = heur->cut.op[i];
        op = heur->op + op_id;

        cost = op->value + op->cost;
        lmCutEnqueueEffects(heur, op_id, cost);
    }

    while (!planPrioQueueEmpty(&heur->queue)){
        id = planPrioQueuePop(&heur->queue, &value);
        fact = heur->fact + id;
        if (fact->value != value)
            continue;

        len     = heur->data.fact_pre[id].size;
        precond = heur->data.fact_pre[id].op;
        for (i = 0; i < len; ++i){
            op = heur->op + precond[i];

            // Consider only supporter facts
            if (op->supporter == id){
                old_value = op->value;
                if (old_value > fact->value){
                    lmCutUpdateSupporter(heur, precond[i]);
                    new_value = op->value;
                    if (new_value != old_value){
                        cost = new_value + op->cost;
                        lmCutEnqueueEffects(heur, precond[i], cost);
                    }
                }
            }
        }
    }
}

_bor_inline plan_cost_t lmCutUpdateOpCost(const oparr_t *cut, op_t *op)
{
    int cut_cost = INT_MAX;
    int i, len, op_id;
    const int *ops;

    len = cut->size;
    ops = cut->op;

    // Find minimal cost from the cut operators
    for (i = 0; i < len; ++i){
        op_id = ops[i];
        cut_cost = BOR_MIN(cut_cost, op[op_id].cost);
    }

    // Substract the minimal cost from all cut operators
    for (i = 0; i < len; ++i){
        op_id = ops[i];
        op[op_id].cost -= cut_cost;
    }

    return cut_cost;
}
