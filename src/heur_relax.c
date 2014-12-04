#include <boruvka/alloc.h>
#include "plan/prioqueue.h"
#include "heur_relax.h"

void planHeurRelaxInit(plan_heur_relax_t *relax,
                       const plan_var_t *var, int var_size,
                       const plan_part_state_t *goal,
                       const plan_op_t *op, int op_size)
{
    int i, op_id;

    planFactOpCrossRefInit(&relax->cref, var, var_size, goal, op, op_size);

    relax->op = BOR_ALLOC_ARR(plan_heur_relax_op_t, relax->cref.op_size);
    relax->op_init = BOR_ALLOC_ARR(plan_heur_relax_op_t, relax->cref.op_size);
    relax->fact = BOR_ALLOC_ARR(plan_heur_relax_fact_t,
                                relax->cref.fact_size);
    relax->fact_init = BOR_ALLOC_ARR(plan_heur_relax_fact_t,
                                     relax->cref.fact_size);

    // Initialize facts
    for (i = 0; i < relax->cref.fact_size; ++i){
        relax->fact_init[i].value = PLAN_COST_MAX;
        relax->fact_init[i].reached_by_op = -1;
    }

    // Initialize operators
    for (i = 0; i < relax->cref.op_size; ++i){
        relax->op_init[i].unsat = relax->cref.op_pre[i].size;
        relax->op_init[i].value = 0;
        relax->op_init[i].cost = 0;

        op_id = relax->cref.op_id[i];
        if (op_id >= 0){
            relax->op_init[i].cost = op[op_id].cost;
        }
    }

    relax->plan_fact = NULL;
    relax->plan_op = NULL;
}

void planHeurRelaxFree(plan_heur_relax_t *relax)
{
    BOR_FREE(relax->op);
    BOR_FREE(relax->op_init);
    BOR_FREE(relax->fact);
    BOR_FREE(relax->fact_init);
    if (relax->plan_fact)
        BOR_FREE(relax->plan_fact);
    if (relax->plan_op)
        BOR_FREE(relax->plan_op);
    planFactOpCrossRefFree(&relax->cref);
}

static void relaxInit(plan_heur_relax_t *relax)
{
    memcpy(relax->op, relax->op_init,
           sizeof(plan_heur_relax_op_t) * relax->cref.op_size);
    memcpy(relax->fact, relax->fact_init,
           sizeof(plan_heur_relax_fact_t) * relax->cref.fact_size);
}

static void relaxAddInitState(plan_heur_relax_t *relax,
                              plan_prio_queue_t *queue,
                              const plan_state_t *state)
{
    int i, len, fact_id;

    len = planStateSize(state);
    for (i = 0; i < len; ++i){
        fact_id = planFactId(&relax->cref.fact_id, i, planStateGet(state, i));
        relax->fact[fact_id].value = 0;
        planPrioQueuePush(queue, 0, fact_id);
    }

    fact_id = relax->cref.no_pre_id;
    relax->fact[fact_id].value = 0;
    planPrioQueuePush(queue, 0, fact_id);
}

static void relaxAddEffects(plan_heur_relax_t *relax,
                            plan_prio_queue_t *queue,
                            int op_id, plan_cost_t op_value)
{
    int i, size, *fact_ids, fact_id;
    plan_heur_relax_fact_t *fact;

    size     = relax->cref.op_eff[op_id].size;
    fact_ids = relax->cref.op_eff[op_id].fact;
    for (i = 0; i < size; ++i){
        fact_id = fact_ids[i];
        fact = relax->fact + fact_id;

        if (fact->value > op_value){
            fact->value = op_value;
            fact->reached_by_op = op_id;
            planPrioQueuePush(queue, fact->value, fact_id);
        }
    }
}

static void relaxOp(plan_heur_relax_t *relax, int type,
                    plan_prio_queue_t *queue,
                    int op_id, plan_cost_t fact_value)
{
    plan_heur_relax_op_t *op = relax->op + op_id;

    // Update operator value
    if (type == PLAN_HEUR_RELAX_TYPE_ADD){
        if (op->value == 0)
            op->value = op->cost;
        op->value = op->value + fact_value;
    }else{ // PLAN_HEUR_RELAX_TYPE_MAX
        op->value = BOR_MAX(op->value, fact_value + op->cost);
    }

    // Decrese counter of unsatisfied preconditions
    --op->unsat;

    // If all preconditions are satisfied, insert effects of the operator
    if (op->unsat == 0)
        relaxAddEffects(relax, queue, op_id, op->value);
}

void planHeurRelaxRun(plan_heur_relax_t *relax, int type,
                      const plan_state_t *state)
{
    plan_prio_queue_t queue;
    int i, size, *op, op_id;
    int fact_id, goal_id;
    plan_cost_t value;
    plan_heur_relax_fact_t *fact;

    relaxInit(relax);
    planPrioQueueInit(&queue);

    goal_id = relax->cref.goal_id;

    relaxAddInitState(relax, &queue, state);
    while (!planPrioQueueEmpty(&queue)){
        fact_id = planPrioQueuePop(&queue, &value);
        fact = relax->fact + fact_id;
        if (fact->value != value)
            continue;

        if (fact_id == goal_id)
            break;

        size = relax->cref.fact_pre[fact_id].size;
        op   = relax->cref.fact_pre[fact_id].op;
        for (i = 0; i < size; ++i){
            op_id = op[i];
            relaxOp(relax, type, &queue, op_id, value);
        }
    }

    planPrioQueueFree(&queue);
}

static void markPlan(plan_heur_relax_t *relax, int fact_id)
{
    plan_heur_relax_fact_t *fact = relax->fact + fact_id;
    int i, size, *facts;

    // mark fact in relaxed plan
    relax->plan_fact[fact_id] = 1;

    size  = relax->cref.op_pre[fact->reached_by_op].size;
    facts = relax->cref.op_pre[fact->reached_by_op].fact;
    for (i = 0; i < size; ++i){
        fact_id = facts[i];

        if (!relax->plan_fact[fact_id]
                && relax->fact[fact_id].reached_by_op != -1){
            markPlan(relax, fact_id);
        }
    }

    // mark operator in relaxed plan
    relax->plan_op[fact->reached_by_op] = 1;
}

void planHeurRelaxMarkPlan(plan_heur_relax_t *relax)
{
    if (relax->plan_fact == NULL)
        relax->plan_fact = BOR_ALLOC_ARR(int, relax->cref.fact_size);
    if (relax->plan_op == NULL)
        relax->plan_op = BOR_ALLOC_ARR(int, relax->cref.op_size);

    bzero(relax->plan_fact, sizeof(int) * relax->cref.fact_size);
    bzero(relax->plan_op, sizeof(int) * relax->cref.op_size);

    if (relax->fact[relax->cref.goal_id].value != PLAN_COST_MAX)
        markPlan(relax, relax->cref.goal_id);
}
