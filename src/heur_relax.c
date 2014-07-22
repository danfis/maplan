#include <boruvka/alloc.h>
#include <boruvka/list.h>
#include <boruvka/lifo.h>
#include "plan/heur.h"
#include "plan/prioqueue.h"

#define TYPE_ADD    0
#define TYPE_MAX    1
#define TYPE_FF     2
#define TYPE_LM_CUT 3

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
struct _oparr_t {
    int *op;  /*!< List of operators' IDs for which the fact is
                   precondition */
    int size; /*!< Size of op[] array */
};
typedef struct _oparr_t oparr_t;

/**
 * Effects of an operator.
 */
struct _factarr_t {
    int *fact; /*!< List of facts that are effects of the operator */
    int size;  /*!< Size of fact[] array */
};
typedef struct _factarr_t factarr_t;

/**
 * Structure for translation of variable's value to fact id.
 */
struct _val_to_id_t {
    int **val_id;
    int var_size;
    int val_size;
};
typedef struct _val_to_id_t val_to_id_t;


/**
 * Extension of facts for lm-cut algorithm.
 */
struct _lm_cut_fact_t {
    unsigned goal_zone; /*!< True if the fact is connected with the goal
                             via zero-cost operators */
    unsigned in_queue;  /*!< True if the fact is already in queue */
};
typedef struct _lm_cut_fact_t lm_cut_fact_t;

struct _lm_cut_t {
    lm_cut_fact_t *fact;      /*!< Extension of fact structures */
    lm_cut_fact_t *fact_init;
    int *op_supporter;        /*!< ID of supporter fact corresponding to
                                   the operator */
    int *op_supporter_init;
    oparr_t cut;              /*!< Array of cut operators */
    int goal_id;              /*!< Fact ID of the artificial goal fact */
};
typedef struct _lm_cut_t lm_cut_t;

struct _plan_heur_relax_t {
    plan_heur_t heur;
    int type;

    const plan_operator_t *base_op; /*!< Pointer to the input operators.
                                         This value is used for computing
                                         ID of the operator based on the
                                         value of its pointer. */
    val_to_id_t vid;

    oparr_t *precond;      /*!< Operators for which the corresponding fact
                                is precondition -- the size of this array
                                equals to .fact_size */
    oparr_t *eff;          /*!< Operators for which the corresponding fact
                                is effect. This array is created only in
                                case of lm-cut heuristic. */
    factarr_t *op_eff;     /*!< Unrolled effects of operators. Size of this
                                array equals to .op_size */
    factarr_t *op_precond; /*!< Precondition facts of operators. Size of
                                this array is .op_size */

    fact_t *fact_init;  /*!< Initialization values for facts */
    fact_t *fact;       /*!< List of facts */
    int fact_size;      /*!< Number of the facts */

    op_t *op_init;      /*!< Initialization values for operators */
    op_t *op;           /*!< List of operators */
    int *op_id;         /*!< Mapping between index in .op[] and actual
                             index of operator. This is here because of
                             conditional effects. Note that .op_id[i] != i
                             only for i >= .actual_op_size */
    int op_size;        /*!< Number of operators */
    int actual_op_size; /*!< The actual number of operators (op_size
                             without conditional effects) */

    int goal_unsat_init; /*!< Number of unsatisfied goal variables */
    int goal_unsat;      /*!< Counter of unsatisfied goals */
    factarr_t goal;          /*!< Copied goal in terms of fact ID */

    int *relaxed_plan;   /*!< Prepared array for relaxed plan */

    plan_prio_queue_t queue;

    lm_cut_t lm_cut; /*!< LM-Cut specific data */
};
typedef struct _plan_heur_relax_t plan_heur_relax_t;

#define HEUR_FROM_PARENT(parent) \
    bor_container_of((parent), plan_heur_relax_t, heur)

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
/** Initializes and frees .precond structures. */
static void opPrecondInit(plan_heur_relax_t *heur,
                          const plan_operator_t *ops, int ops_size);
static void opPrecondFree(plan_heur_relax_t *heur);
/** Initializes and frees .eff* structures */
static void opEffInit(plan_heur_relax_t *heur,
                      const plan_operator_t *ops, int ops_size);
static void opEffFree(plan_heur_relax_t *heur);
/** Removes fact id from specified position. */
static void effRemoveId(factarr_t *eff, int pos);
/** Remove same effects where its cost is higher */
static void opSimplifyEffects(factarr_t *ref_eff, factarr_t *op_eff,
                              plan_cost_t ref_cost, plan_cost_t op_cost);
/** Simplifies internal representation of operators so that there are no
 *  two operators applicable in the same time with the same effect. */
static void opSimplify(plan_heur_relax_t *heur,
                       const plan_operator_t *op,
                       const plan_succ_gen_t *succ_gen);
/** Initializes and frees .precond structures */
static void precondInit(plan_heur_relax_t *heur);
static void precondFree(plan_heur_relax_t *heur);
/** Initializes and frees .eff structures */
static void effInit(plan_heur_relax_t *heur);
static void effFree(plan_heur_relax_t *heur);
static void lmCutInit(plan_heur_relax_t *heur,
                      const plan_part_state_t *goal);
static void lmCutFree(plan_heur_relax_t *heur);


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
static plan_cost_t ctxHeur(plan_heur_relax_t *heur,
                           plan_heur_preferred_ops_t *preferred_ops);

/** Sets heur->relaxed_plan[i] to 1 if i'th operator is in relaxed plan and
 *  to 0 if it is not. */
static void markRelaxedPlan(plan_heur_relax_t *heur);

/** Initialize context for lm-cut heuristic */
static void lmCutCtxInit(plan_heur_relax_t *heur);
/** Free context for lm-cut heuristic */
static void lmCutCtxFree(plan_heur_relax_t *heur);
/** Enqueue a signle fact into priority queue */
_bor_inline void lmCutEnqueue(plan_heur_relax_t *heur, int fact_id, int value);
/** Enqueue all operator's effects into priority queue with the specified
 *  value. */
_bor_inline void lmCutEnqueueEffects(plan_heur_relax_t *heur, int op_id,
                                     int value);
/** First relaxed exploration from the given state to the goal */
static void lmCutInitialExploration(plan_heur_relax_t *heur,
                                    const plan_state_t *state);
/** Marks goal-zone, i.e., facts that are connected with the goal with
 *  zero-cost operators */
static void lmCutMarkGoalZone(plan_heur_relax_t *heur, int fact_id);
/** Adds initial state facts into queue */
_bor_inline void lmCutFindCutAddInit(plan_heur_relax_t *heur,
                                     const plan_state_t *state,
                                     bor_lifo_t *queue);
/** Determines if the specified operator has effect in goal-zone. If so,
 *  the operator is appended to the heur->lm_cut.cut and returns true.
 *  Otherwise false is returned. */
_bor_inline int lmCutFindCutProcessOp(plan_heur_relax_t *heur, int op_id);
/** Enqueues all effects of operator (if not already in queue). */
_bor_inline void lmCutFindCutEnqueueEffects(plan_heur_relax_t *heur,
                                            int op_id,
                                            bor_lifo_t *queue);
/** Finds cut operators -- fills heur->lm_cut.cut structure */
static void lmCutFindCut(plan_heur_relax_t *heur, const plan_state_t *state);
/** Updates operator's supporter fact */
static void lmCutUpdateSupporter(plan_heur_relax_t *heur, int op_id);
/** Incrementally explores relaxed plan */
static void lmCutIncrementalExploration(plan_heur_relax_t *heur);
_bor_inline plan_cost_t lmCutUpdateOpCost(const oparr_t *cut, op_t *op);


struct _pref_ops_selector_t {
    plan_heur_preferred_ops_t *pref_ops; /*!< In/Out data structure */
    plan_operator_t *base_op;            /*!< Base pointer to the source
                                              operator array. */
    plan_operator_t **cur;               /*!< Cursor pointing to the next
                                              operator in .pref_ops->op[] */
    plan_operator_t **end;               /*!< Points after .pref_ops->op[] */
    plan_operator_t **ins;               /*!< Next insert position for
                                              preferred operator. */
};
typedef struct _pref_ops_selector_t pref_ops_selector_t;

/** This macro is here only to shut gcc compiler which is stupidly
 *  complaining about "may-be uninitialized" members of this struct. */
#define PREF_OPS_SELECTOR(name) \
    pref_ops_selector_t name = { NULL, NULL, NULL, NULL, NULL }

/** Initializes preferred-operator-selector. */
static void prefOpsSelectorInit(pref_ops_selector_t *selector,
                                plan_heur_preferred_ops_t *pref_ops,
                                const plan_operator_t *base_op);
/** Finalizes selecting of preferred operators */
static void prefOpsSelectorFinalize(pref_ops_selector_t *sel);
/** Mark the specified operator as preferred operator. This function must
 *  be called only with increasing values of op_id! */
static void prefOpsSelectorMarkPreferredOp(pref_ops_selector_t *sel,
                                           int op_id);

/** Main function that returns heuristic value. */
static plan_cost_t planHeurRelax(plan_heur_t *heur, const plan_state_t *state,
                                 plan_heur_preferred_ops_t *preferred_ops);
static plan_cost_t planHeurLMCut(plan_heur_t *heur, const plan_state_t *state,
                                 plan_heur_preferred_ops_t *preferred_ops);
/** Delete method */
static void planHeurRelaxDel(plan_heur_t *_heur);


static plan_heur_t *planHeurRelaxNew(int type,
                                     const plan_var_t *var, int var_size,
                                     const plan_part_state_t *goal,
                                     const plan_operator_t *op, int op_size,
                                     const plan_succ_gen_t *_succ_gen)
{
    plan_heur_relax_t *heur;
    plan_succ_gen_t *succ_gen;
    plan_cost_t (*heur_fn)(plan_heur_t *, const plan_state_t *,
                           plan_heur_preferred_ops_t *);

    heur_fn = planHeurRelax;
    if (type == TYPE_LM_CUT)
        heur_fn = planHeurLMCut;

    if (_succ_gen){
        succ_gen = (plan_succ_gen_t *)_succ_gen;
    }else{
        succ_gen = planSuccGenNew(op, op_size);
    }

    heur = BOR_ALLOC(plan_heur_relax_t);
    planHeurInit(&heur->heur, planHeurRelaxDel, heur_fn);
    heur->type = type;
    heur->base_op = op;

    valToIdInit(&heur->vid, var, var_size);
    goalInit(heur, goal);
    factInit(heur, goal);
    opInit(heur, op, op_size);
    opPrecondInit(heur, op, op_size);
    opEffInit(heur, op, op_size);
    opSimplify(heur, op, succ_gen);
    precondInit(heur);
    effInit(heur);
    lmCutInit(heur, goal);
    heur->relaxed_plan = BOR_ALLOC_ARR(int, op_size);

    if (!_succ_gen)
        planSuccGenDel(succ_gen);

    return &heur->heur;
}

plan_heur_t *planHeurRelaxAddNew(const plan_var_t *var, int var_size,
                                 const plan_part_state_t *goal,
                                 const plan_operator_t *op, int op_size,
                                 const plan_succ_gen_t *succ_gen)
{
    return planHeurRelaxNew(TYPE_ADD, var, var_size, goal,
                            op, op_size, succ_gen);
}

plan_heur_t *planHeurRelaxMaxNew(const plan_var_t *var, int var_size,
                                 const plan_part_state_t *goal,
                                 const plan_operator_t *op, int op_size,
                                 const plan_succ_gen_t *succ_gen)
{
    return planHeurRelaxNew(TYPE_MAX, var, var_size, goal,
                            op, op_size, succ_gen);
}

plan_heur_t *planHeurRelaxFFNew(const plan_var_t *var, int var_size,
                                const plan_part_state_t *goal,
                                const plan_operator_t *op, int op_size,
                                const plan_succ_gen_t *succ_gen)
{
    return planHeurRelaxNew(TYPE_FF, var, var_size, goal,
                            op, op_size, succ_gen);
}

plan_heur_t *planHeurLMCutNew(const plan_var_t *var, int var_size,
                              const plan_part_state_t *goal,
                              const plan_operator_t *op, int op_size,
                              const plan_succ_gen_t *succ_gen)
{
    return planHeurRelaxNew(TYPE_LM_CUT, var, var_size, goal,
                            op, op_size, succ_gen);
}

static void planHeurRelaxDel(plan_heur_t *_heur)
{
    plan_heur_relax_t *heur = HEUR_FROM_PARENT(_heur);
    lmCutFree(heur);
    effFree(heur);
    precondFree(heur);
    opEffFree(heur);
    opPrecondFree(heur);
    opFree(heur);
    factFree(heur);
    goalFree(heur);
    valToIdFree(&heur->vid);

    planHeurFree(&heur->heur);
    BOR_FREE(heur->relaxed_plan);
    BOR_FREE(heur);
}

static plan_cost_t planHeurRelax(plan_heur_t *_heur, const plan_state_t *state,
                                 plan_heur_preferred_ops_t *preferred_ops)
{
    plan_heur_relax_t *heur = HEUR_FROM_PARENT(_heur);
    plan_cost_t h = PLAN_HEUR_DEAD_END;

    if (preferred_ops)
        preferred_ops->preferred_size = 0;

    ctxInit(heur);
    ctxAddInitState(heur, state);
    if (ctxMainLoop(heur) == 0){
        h = ctxHeur(heur, preferred_ops);
    }
    ctxFree(heur);

    return h;
}

static plan_cost_t planHeurLMCut(plan_heur_t *_heur, const plan_state_t *state,
                                 plan_heur_preferred_ops_t *preferred_ops)
{
    plan_heur_relax_t *heur = HEUR_FROM_PARENT(_heur);
    plan_cost_t h = PLAN_HEUR_DEAD_END;

    if (preferred_ops)
        preferred_ops->preferred_size = 0;

    // Initialize context for the current run
    lmCutCtxInit(heur);

    // Perform initial exploration of relaxed plan state
    lmCutInitialExploration(heur, state);

    // Check whether the goal was reached, if so prepare output variable
    if (heur->fact[heur->lm_cut.goal_id].value >= 0)
        h = 0;

    while (heur->fact[heur->lm_cut.goal_id].value > 0){
        // Initialize fact additional flags
        memcpy(heur->lm_cut.fact, heur->lm_cut.fact_init,
               sizeof(lm_cut_fact_t) * heur->fact_size);

        // Mark facts that are connected with the goal via zero-cost
        // operators
        lmCutMarkGoalZone(heur, heur->lm_cut.goal_id);

        // Find cut operators, i.e., operators connected with effects in
        // goal-zone
        lmCutFindCut(heur, state);

        // Determine the minimal cost from all cut-operators. Substract
        // this cost from their cost and add it to the final heuristic
        // value.
        h += lmCutUpdateOpCost(&heur->lm_cut.cut, heur->op);

        // Performat incremental exploration of relaxed plan state.
        lmCutIncrementalExploration(heur);
    }

    lmCutCtxFree(heur);

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
    if (heur->type == TYPE_LM_CUT)
        heur->fact_size += 1;
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
    int i, j, cond_eff_size;
    int cond_eff_ins;
    plan_operator_cond_eff_t *cond_eff;

    // determine number of conditional effects in operators
    cond_eff_size = 0;
    for (i = 0; i < ops_size; ++i){
        cond_eff_size += ops[i].cond_eff_size;
    }

    // prepare operator arrays
    heur->op_size = ops_size + cond_eff_size;
    if (heur->type == TYPE_LM_CUT)
        heur->op_size += 1;
    heur->actual_op_size = ops_size;
    heur->op = BOR_ALLOC_ARR(op_t, heur->op_size);
    heur->op_init = BOR_ALLOC_ARR(op_t, heur->op_size);
    heur->op_id = BOR_ALLOC_ARR(int, heur->op_size);

    // Determine starting position for insertion of conditional effects.
    // The conditional effects are inserted as a (somehow virtual)
    // operators at the end of the heur->op array.
    cond_eff_ins = ops_size;

    for (i = 0; i < ops_size; ++i){
        heur->op_init[i].unsat = ops[i].pre->vals_size;
        heur->op_init[i].value = ops[i].cost;
        heur->op_init[i].cost  = ops[i].cost;
        heur->op_id[i] = i;

        // Insert conditional effects
        for (j = 0; j < ops[i].cond_eff_size; ++j){
            cond_eff = ops[i].cond_eff + j;
            heur->op_init[cond_eff_ins].unsat  = cond_eff->pre->vals_size;
            heur->op_init[cond_eff_ins].unsat += ops[i].pre->vals_size;
            heur->op_init[cond_eff_ins].value = ops[i].cost;
            heur->op_init[cond_eff_ins].cost  = ops[i].cost;
            heur->op_id[cond_eff_ins] = i;
            ++cond_eff_ins;
        }
    }

    if (heur->type == TYPE_LM_CUT){
        heur->op_init[heur->op_size - 1].unsat = heur->goal.size;
        heur->op_init[heur->op_size - 1].value = 0;
        heur->op_init[heur->op_size - 1].cost  = 0;
        heur->op_id[i] = -1;
    }
}

static void opFree(plan_heur_relax_t *heur)
{
    BOR_FREE(heur->op_init);
    BOR_FREE(heur->op);
    BOR_FREE(heur->op_id);
}

static void opPrecondCondEffInit(plan_heur_relax_t *heur, factarr_t *precond,
                                 const plan_operator_t *op,
                                 const plan_operator_cond_eff_t *cond_eff)
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
            id = valToId(&heur->vid, var, val);
            precond->fact[i++] = id;

        }else if (planPartStateIsSet(cond_eff_pre, var)){
            val = planPartStateGet(cond_eff_pre, var);
            id = valToId(&heur->vid, var, val);
            precond->fact[i++] = id;
        }
    }
}

static void opPrecondInit(plan_heur_relax_t *heur,
                          const plan_operator_t *ops, int ops_size)
{
    int i, j, id, cond_eff_ins;
    plan_var_id_t var;
    plan_val_t val;
    factarr_t *precond;

    precond = heur->op_precond = BOR_ALLOC_ARR(factarr_t, heur->op_size);

    // The conditional effects are after "ordinary" operators
    cond_eff_ins = ops_size;

    for (i = 0; i < ops_size; ++i, ++precond){
        precond->size = ops[i].pre->vals_size;
        precond->fact = BOR_ALLOC_ARR(int, precond->size);

        PLAN_PART_STATE_FOR_EACH(ops[i].pre, j, var, val){
            id = valToId(&heur->vid, var, val);
            precond->fact[j] = id;
        }

        for (j = 0; j < ops[i].cond_eff_size; ++j){
            opPrecondCondEffInit(heur, heur->op_precond + cond_eff_ins,
                                 ops + i, ops[i].cond_eff + j);
            ++cond_eff_ins;
        }
    }

    if (heur->type == TYPE_LM_CUT){
        precond = heur->op_precond + heur->op_size - 1;
        precond->size = heur->goal.size;
        precond->fact = BOR_ALLOC_ARR(int, precond->size);
        memcpy(precond->fact, heur->goal.fact, sizeof(int) * precond->size);
    }
}

static void opPrecondFree(plan_heur_relax_t *heur)
{
    int i;

    for (i = 0; i < heur->op_size; ++i)
        BOR_FREE(heur->op_precond[i].fact);
    BOR_FREE(heur->op_precond);
}

static void opEffCondEffInit(plan_heur_relax_t *heur, factarr_t *eff,
                             const plan_operator_cond_eff_t *cond_eff)
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
        eff->fact[j++] = valToId(&heur->vid, var, val);
    }
}

static void opEffInit(plan_heur_relax_t *heur,
                    const plan_operator_t *ops, int ops_size)
{
    int i, j, cond_eff_ins;
    plan_var_id_t var;
    plan_val_t val;
    factarr_t *eff;

    eff = heur->op_eff = BOR_ALLOC_ARR(factarr_t, heur->op_size);

    // Conditional effects are after all operators
    cond_eff_ins = ops_size;

    // First insert effects ignoring conditional effects
    for (i = 0; i < ops_size; ++i, ++eff){
        eff->size = ops[i].eff->vals_size;
        eff->fact = BOR_ALLOC_ARR(int, eff->size);
        PLAN_PART_STATE_FOR_EACH(ops[i].eff, j, var, val){
            eff->fact[j] = valToId(&heur->vid, var, val);
        }

        for (j = 0; j < ops[i].cond_eff_size; ++j){
            opEffCondEffInit(heur, heur->op_eff + cond_eff_ins,
                             ops[i].cond_eff + j);
            ++cond_eff_ins;
        }
    }

    if (heur->type == TYPE_LM_CUT){
        eff = heur->op_eff + heur->op_size - 1;
        eff->size = 1;
        eff->fact = BOR_ALLOC_ARR(int, 1);
        eff->fact[0] = heur->fact_size - 1;
    }
}

static void opEffFree(plan_heur_relax_t *heur)
{
    int i;

    for (i = 0; i < heur->op_size; ++i)
        BOR_FREE(heur->op_eff[i].fact);
    BOR_FREE(heur->op_eff);
}

static void effRemoveId(factarr_t *eff, int pos)
{
    int i;
    for (i = pos + 1; i < eff->size; ++i)
        eff->fact[i - 1] = eff->fact[i];
    --eff->size;
}

static void opSimplifyEffects(factarr_t *ref_eff, factarr_t *op_eff,
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
                       const plan_succ_gen_t *succ_gen)
{
    int i, ref_i, op_i;
    const plan_operator_t *ref_op; // reference operator
    plan_operator_t **ops;   // list of operators applicable in ref_op->pre
    int ops_size;            // number of applicable operators

    // Allocate array for applicable operators
    ops = BOR_ALLOC_ARR(plan_operator_t *, heur->op_size);

    for (ref_i = 0; ref_i < heur->op_size; ++ref_i){
        if (heur->op_id[ref_i] < 0)
            continue;
        ref_op = op + heur->op_id[ref_i];

        // skip operators with no effects
        if (heur->op_eff[ref_i].size == 0)
            continue;

        // get all applicable operators
        ops_size = planSuccGenFindPart(succ_gen, ref_op->pre, ops, heur->op_size);

        for (i = 0; i < ops_size; ++i){
            // don't compare two identical operators
            if (ref_op == ops[i])
                continue;

            // compute id of the operator
            op_i = ((unsigned long)ops[i] - (unsigned long)op)
                        / sizeof(plan_operator_t);

            // simplify effects, if possible delete in reference operator
            opSimplifyEffects(heur->op_eff + ref_i, heur->op_eff + op_i,
                              ref_op->cost, ops[i]->cost);
        }
    }

    BOR_FREE(ops);
}

static void crossReferenceInit(oparr_t **dst, int fact_size,
                               factarr_t *src, int op_size,
                               factarr_t *test)
{
    int i, opi, fact_id;
    oparr_t *fact, *update;
    factarr_t *cur, *end;

    *dst = BOR_CALLOC_ARR(oparr_t, fact_size);
    fact = *dst;

    cur = src;
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

static void crossReferenceFree(oparr_t *arr, int size)
{
    int i;

    if (arr == NULL)
        return;

    for (i = 0; i < size; ++i)
        BOR_FREE(arr[i].op);
    BOR_FREE(arr);
}

static void precondInit(plan_heur_relax_t *heur)
{
    crossReferenceInit(&heur->precond, heur->fact_size,
                       heur->op_precond, heur->op_size,
                       heur->op_eff);
}

static void precondFree(plan_heur_relax_t *heur)
{
    crossReferenceFree(heur->precond, heur->fact_size);
}

static void effInit(plan_heur_relax_t *heur)
{
    if (heur->type != TYPE_LM_CUT){
        heur->eff = NULL;
        return;
    }

    crossReferenceInit(&heur->eff, heur->fact_size,
                       heur->op_eff, heur->op_size, NULL);
}

static void effFree(plan_heur_relax_t *heur)
{
    crossReferenceFree(heur->eff, heur->fact_size);
}

static void lmCutInit(plan_heur_relax_t *heur,
                      const plan_part_state_t *goal)
{
    int i;

    if (heur->type != TYPE_LM_CUT)
        return;

    heur->lm_cut.fact      = BOR_ALLOC_ARR(lm_cut_fact_t, heur->fact_size);
    heur->lm_cut.fact_init = BOR_CALLOC_ARR(lm_cut_fact_t, heur->fact_size);

    heur->lm_cut.op_supporter      = BOR_ALLOC_ARR(int, heur->op_size);
    heur->lm_cut.op_supporter_init = BOR_ALLOC_ARR(int, heur->op_size);
    for (i = 0; i < heur->op_size; ++i)
        heur->lm_cut.op_supporter_init[i] = -1;

    heur->lm_cut.cut.op = BOR_ALLOC_ARR(int, heur->op_size);
    heur->lm_cut.cut.size = 0;
    heur->lm_cut.goal_id = heur->fact_size - 1;
}

static void lmCutFree(plan_heur_relax_t *heur)
{
    if (heur->type != TYPE_LM_CUT)
        return;

    BOR_FREE(heur->lm_cut.fact);
    BOR_FREE(heur->lm_cut.fact_init);
    BOR_FREE(heur->lm_cut.op_supporter);
    BOR_FREE(heur->lm_cut.op_supporter_init);
    BOR_FREE(heur->lm_cut.cut.op);
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

_bor_inline void ctxUpdateFact(plan_heur_relax_t *heur, int fact_id,
                               int op_id, int op_value)
{
    fact_t *fact = heur->fact + fact_id;

    fact->value = op_value;
    fact->reached_by_op = op_id;

    // Insert only those facts that can be used later, i.e., can satisfy
    // goal or some operators have them as preconditions.
    // It should speed it up a little.
    if (fact->goal || heur->precond[fact_id].size > 0)
        planPrioQueuePush(&heur->queue, fact->value, fact_id);
}

static void ctxAddInitState(plan_heur_relax_t *heur,
                            const plan_state_t *state)
{
    int i, id, len;

    // insert all facts from the initial state into priority queue
    len = planStateSize(state);
    for (i = 0; i < len; ++i){
        id = valToId(&heur->vid, i, planStateGet(state, i));
        ctxUpdateFact(heur, id, -1, 0);
    }
}

static void ctxAddEffects(plan_heur_relax_t *heur, int op_id)
{
    int i, id;
    fact_t *fact;
    int *eff_facts, eff_facts_len;
    int op_value = heur->op[op_id].value;

    eff_facts = heur->op_eff[op_id].fact;
    eff_facts_len = heur->op_eff[op_id].size;
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

static plan_cost_t relaxHeurAddMax(plan_heur_relax_t *heur,
                                   plan_heur_preferred_ops_t *pref_ops)
{
    int i, id;
    plan_cost_t h;

    h = PLAN_COST_ZERO;
    for (i = 0; i < heur->goal.size; ++i){
        id = heur->goal.fact[i];
        h = relaxHeurValue(heur->type, h, heur->fact[id].value);
    }

    if (pref_ops){
        pref_ops_selector_t pref_ops_selector;
        markRelaxedPlan(heur);
        prefOpsSelectorInit(&pref_ops_selector, pref_ops, heur->base_op);
        for (i = 0; i < heur->actual_op_size; ++i){
            if (heur->relaxed_plan[i])
                prefOpsSelectorMarkPreferredOp(&pref_ops_selector, i);
        }
        prefOpsSelectorFinalize(&pref_ops_selector);
    }

    return h;
}




static plan_cost_t relaxHeurFF(plan_heur_relax_t *heur,
                               plan_heur_preferred_ops_t *preferred_ops)
{
    int i;
    plan_cost_t h = PLAN_COST_ZERO;
    PREF_OPS_SELECTOR(pref_ops_selector);

    if (preferred_ops){
        prefOpsSelectorInit(&pref_ops_selector, preferred_ops,
                            heur->base_op);
    }

    markRelaxedPlan(heur);
    for (i = 0; i < heur->actual_op_size; ++i){
        if (heur->relaxed_plan[i]){
            if (preferred_ops){
                prefOpsSelectorMarkPreferredOp(&pref_ops_selector, i);
            }

            h += heur->op[i].cost;
        }
    }

    if (preferred_ops){
        prefOpsSelectorFinalize(&pref_ops_selector);
    }

    return h;
}

static plan_cost_t ctxHeur(plan_heur_relax_t *heur,
                           plan_heur_preferred_ops_t *preferred_ops)
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

        op_precond      = heur->op_precond[fact->reached_by_op].fact;
        op_precond_size = heur->op_precond[fact->reached_by_op].size;
        for (i = 0; i < op_precond_size; ++i){
            id2 = op_precond[i];

            if (!heur->fact[id2].relaxed_plan_visited
                    && heur->fact[id2].reached_by_op != -1){
                _markRelaxedPlan(heur, relaxed_plan, id2);
            }
        }

        relaxed_plan[heur->op_id[fact->reached_by_op]] = 1;
    }
}

static void markRelaxedPlan(plan_heur_relax_t *heur)
{
    int i, id;

    bzero(heur->relaxed_plan, sizeof(int) * heur->actual_op_size);
    for (i = 0; i < heur->goal.size; ++i){
        id = heur->goal.fact[i];
        _markRelaxedPlan(heur, heur->relaxed_plan, id);
    }
}



static void lmCutCtxInit(plan_heur_relax_t *heur)
{
    memcpy(heur->op, heur->op_init, sizeof(op_t) * heur->op_size);
    memcpy(heur->fact, heur->fact_init, sizeof(fact_t) * heur->fact_size);
    memcpy(heur->lm_cut.op_supporter, heur->lm_cut.op_supporter_init,
           sizeof(int) * heur->op_size);
    planPrioQueueInit(&heur->queue);
}

static void lmCutCtxFree(plan_heur_relax_t *heur)
{
    planPrioQueueFree(&heur->queue);
}

_bor_inline void lmCutEnqueue(plan_heur_relax_t *heur, int fact_id, int value)
{
    fact_t *fact = heur->fact + fact_id;

    if (fact->value == -1 || fact->value > value){
        fact->value = value;
        planPrioQueuePush(&heur->queue, fact->value, fact_id);
    }
}

_bor_inline void lmCutEnqueueEffects(plan_heur_relax_t *heur, int op_id,
                                     int value)
{
    int i, len, *eff;
    len = heur->op_eff[op_id].size;
    eff = heur->op_eff[op_id].fact;
    for (i = 0; i < len; ++i)
        lmCutEnqueue(heur, eff[i], value);
}

static void lmCutInitialExploration(plan_heur_relax_t *heur,
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
        id = valToId(&heur->vid, i, planStateGet(state, i));
        lmCutEnqueue(heur, id, 0);
    }

    while (!planPrioQueueEmpty(&heur->queue)){
        id = planPrioQueuePop(&heur->queue, &value);
        fact = heur->fact + id;
        if (fact->value != value)
            continue;

        len = heur->precond[id].size;
        precond = heur->precond[id].op;
        for (i = 0; i < len; ++i){
            op = heur->op + precond[i];
            --op->unsat;
            if (op->unsat <= 0){
                op->value = fact->value;
                heur->lm_cut.op_supporter[precond[i]] = id;
                cost = fact->value + op->cost;
                lmCutEnqueueEffects(heur, precond[i], cost);
            }
        }
    }
}

static void lmCutMarkGoalZone(plan_heur_relax_t *heur, int fact_id)
{
    int i, len, *op_ids, op_id;
    op_t *op;

    if (!heur->lm_cut.fact[fact_id].goal_zone){
        heur->lm_cut.fact[fact_id].goal_zone = 1;

        len    = heur->eff[fact_id].size;
        op_ids = heur->eff[fact_id].op;
        for (i = 0; i < len; ++i){
            op_id = op_ids[i];
            op = heur->op + op_id;
            if (op->cost == 0 && heur->lm_cut.op_supporter[op_id] != -1)
                lmCutMarkGoalZone(heur, heur->lm_cut.op_supporter[op_id]);
        }
    }
}

_bor_inline void lmCutFindCutAddInit(plan_heur_relax_t *heur,
                                     const plan_state_t *state,
                                     bor_lifo_t *queue)
{
    plan_var_id_t var, len;
    plan_val_t val;
    int id;

    len = planStateSize(state);
    for (var = 0; var < len; ++var){
        val = planStateGet(state, var);
        id = valToId(&heur->vid, var, val);
        heur->lm_cut.fact[id].in_queue = 1;
        borLifoPush(queue, &id);
    }
}

_bor_inline int lmCutFindCutProcessOp(plan_heur_relax_t *heur, int op_id)
{
    int i, len, *facts, fact_id;

    len   = heur->op_eff[op_id].size;
    facts = heur->op_eff[op_id].fact;
    for (i = 0; i < len; ++i){
        fact_id = facts[i];
        if (heur->lm_cut.fact[fact_id].goal_zone){
            heur->lm_cut.cut.op[heur->lm_cut.cut.size++] = op_id;
            return 1;
        }
    }

    return 0;
}

_bor_inline void lmCutFindCutEnqueueEffects(plan_heur_relax_t *heur,
                                            int op_id,
                                            bor_lifo_t *queue)
{
    int i, len, *facts, fact_id;

    len   = heur->op_eff[op_id].size;
    facts = heur->op_eff[op_id].fact;
    for (i = 0; i < len; ++i){
        fact_id = facts[i];
        if (!heur->lm_cut.fact[fact_id].in_queue){
            heur->lm_cut.fact[fact_id].in_queue = 1;
            borLifoPush(queue, &fact_id);
        }
    }
}

static void lmCutFindCut(plan_heur_relax_t *heur, const plan_state_t *state)
{
    int i, fact_id, op_id;
    bor_lifo_t queue;

    // Reset output structure
    heur->lm_cut.cut.size = 0;

    // Initialize queue and adds initial state
    borLifoInit(&queue, sizeof(int));
    lmCutFindCutAddInit(heur, state, &queue);

    while (!borLifoEmpty(&queue)){
        // Pop next fact from queue
        fact_id = *(int *)borLifoBack(&queue);
        borLifoPop(&queue);

        for (i = 0; i < heur->precond[fact_id].size; ++i){
            op_id = heur->precond[fact_id].op[i];
            if (heur->lm_cut.op_supporter[op_id] == fact_id){
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

static void lmCutUpdateSupporter(plan_heur_relax_t *heur, int op_id)
{
    int i, len, *precond;
    int supp;

    len = heur->op_precond[op_id].size;
    if (len == 0)
        return;
    precond = heur->op_precond[op_id].fact;
    supp = heur->lm_cut.op_supporter[op_id];

    for (i = 0; i < len; ++i){
        if (heur->fact[precond[i]].value > heur->fact[supp].value){
            supp = precond[i];
        }
    }

    heur->lm_cut.op_supporter[op_id] = supp;
    heur->op[op_id].value = heur->fact[supp].value;
}

static void lmCutIncrementalExploration(plan_heur_relax_t *heur)
{
    int i, op_id, cost, id, value, len, *precond;
    int old_value, new_value;
    fact_t *fact;
    op_t *op;

    // Adds effects of cut-operators to the queue
    for (i = 0; i < heur->lm_cut.cut.size; ++i){
        op_id = heur->lm_cut.cut.op[i];
        op = heur->op + op_id;

        cost = op->value + op->cost;
        lmCutEnqueueEffects(heur, op_id, cost);
    }

    while (!planPrioQueueEmpty(&heur->queue)){
        id = planPrioQueuePop(&heur->queue, &value);
        fact = heur->fact + id;
        if (fact->value != value)
            continue;

        len     = heur->precond[id].size;
        precond = heur->precond[id].op;
        for (i = 0; i < len; ++i){
            op = heur->op + precond[i];

            // Consider only supporter facts
            if (heur->lm_cut.op_supporter[precond[i]] == id){
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



static int sortPreferredOpsByPtrCmp(const void *a, const void *b)
{
    const plan_operator_t *op1 = *(const plan_operator_t **)a;
    const plan_operator_t *op2 = *(const plan_operator_t **)b;
    return op1 - op2;
}

static void prefOpsSelectorInit(pref_ops_selector_t *selector,
                                plan_heur_preferred_ops_t *pref_ops,
                                const plan_operator_t *base_op)
{
    selector->pref_ops = pref_ops;
    selector->base_op  = (plan_operator_t *)base_op;

    // Sort array of operators by their pointers.
    if (pref_ops->op_size > 0){
        qsort(pref_ops->op, pref_ops->op_size, sizeof(plan_operator_t *),
              sortPreferredOpsByPtrCmp);
    }

    // Set up cursors
    selector->cur = pref_ops->op;
    selector->end = pref_ops->op + pref_ops->op_size;
    selector->ins = pref_ops->op;
}

static void prefOpsSelectorFinalize(pref_ops_selector_t *sel)
{
    // Compute number of preferred operators from .ins pointer
    sel->pref_ops->preferred_size = sel->ins - sel->pref_ops->op;
}

static void prefOpsSelectorMarkPreferredOp(pref_ops_selector_t *sel,
                                           int op_id)
{
    plan_operator_t *op = sel->base_op + op_id;
    plan_operator_t *op_swp;

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
