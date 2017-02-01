/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#include <boruvka/alloc.h>
#include "plan/prio_queue.h"
#include "plan/heur.h"
#include "heur_relax.h"

_bor_inline plan_cost_t _cost(plan_cost_t cost, unsigned flags)
{
    if (flags & PLAN_HEUR_OP_UNIT_COST)
        return 1;
    if (flags & PLAN_HEUR_OP_COST_PLUS_ONE)
        return cost + 1;
    return cost;
}

void planHeurRelaxInit(plan_heur_relax_t *relax, int type,
                       const plan_var_t *var, int var_size,
                       const plan_part_state_t *goal,
                       const plan_op_t *op, int op_size,
                       unsigned flags)
{
    int i, op_id;

    relax->type = type;
    planFactOpCrossRefInit(&relax->cref, var, var_size, goal,
                           op, op_size, flags);

    relax->op = BOR_ALLOC_ARR(plan_heur_relax_op_t, relax->cref.op_size);
    relax->op_init = BOR_ALLOC_ARR(plan_heur_relax_op_t, relax->cref.op_size);
    relax->fact = BOR_ALLOC_ARR(plan_heur_relax_fact_t,
                                relax->cref.fact_size);
    relax->fact_init = BOR_ALLOC_ARR(plan_heur_relax_fact_t,
                                     relax->cref.fact_size);

    // Initialize facts
    for (i = 0; i < relax->cref.fact_size; ++i){
        relax->fact_init[i].value = PLAN_COST_MAX;
        relax->fact_init[i].supp = -1;
    }

    // Initialize operators
    for (i = 0; i < relax->cref.op_size; ++i){
        relax->op_init[i].unsat = relax->cref.op_pre[i].size;
        relax->op_init[i].value = 0;
        relax->op_init[i].cost = 0;
        relax->op_init[i].supp = -1;

        op_id = relax->cref.op_id[i];
        if (op_id >= 0){
            relax->op_init[i].cost = _cost(op[op_id].cost, flags);
        }
    }

    relax->plan_fact = NULL;
    relax->plan_op = NULL;
    relax->goal_fact = NULL;
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
    if (relax->goal_fact)
        BOR_FREE(relax->goal_fact);
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
    plan_cost_t value;

    PLAN_FACT_ID_FOR_EACH_STATE(&relax->cref.fact_id, state, fact_id){
        relax->fact[fact_id].value = 0;
        planPrioQueuePush(queue, 0, fact_id);
    }

    len = relax->cref.fake_pre_size;
    for (i = 0; i < len; ++i){
        fact_id = relax->cref.fake_pre[i].fact_id;
        value   = relax->cref.fake_pre[i].value;
        relax->fact[fact_id].value = value;
        planPrioQueuePush(queue, value, fact_id);
    }

}

static void relaxAddEffects(plan_heur_relax_t *relax,
                            plan_prio_queue_t *queue,
                            int op_id, plan_cost_t op_value)
{
    plan_heur_relax_fact_t *fact;
    int fact_id;

    PLAN_ARR_INT_FOR_EACH(relax->cref.op_eff + op_id, fact_id){
        fact = relax->fact + fact_id;

        if (fact->value > op_value){
            fact->value = op_value;
            fact->supp = op_id;
            planPrioQueuePush(queue, fact->value, fact_id);
        }
    }
}

static void relaxOpAdd(plan_heur_relax_t *relax,
                       plan_prio_queue_t *queue,
                       int op_id, int fact_id,
                       plan_cost_t fact_value)
{
    plan_heur_relax_op_t *op = relax->op + op_id;

    // Update operator value
    if (op->value == 0)
        op->value = op->cost;
    op->value = op->value + fact_value;

    // Decrese counter of unsatisfied preconditions
    --op->unsat;

    // If all preconditions are satisfied, insert effects of the operator
    if (op->unsat == 0){
        op->supp = fact_id;
        relaxAddEffects(relax, queue, op_id, op->value);
    }
}

static void relaxOpMax(plan_heur_relax_t *relax,
                       plan_prio_queue_t *queue,
                       int op_id, int fact_id,
                       plan_cost_t fact_value)
{
    plan_heur_relax_op_t *op = relax->op + op_id;

    // Decrese counter of unsatisfied preconditions
    --op->unsat;

    // If all preconditions are satisfied, insert effects of the operator
    if (op->unsat == 0){
        op->supp = fact_id;
        // We don't need to compute max value during the algorithm because
        // we know that the last fact that enables this operator is also
        // the one with highest value.
        op->value = fact_value + op->cost;
        relaxAddEffects(relax, queue, op_id, op->value);
    }
}

static void exploreAdd(plan_heur_relax_t *relax, const plan_state_t *state)
{
    int goal_id = relax->cref.goal_id;

#define PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL  \
    if (fact_id == goal_id) break
#define PLAN_HEUR_RELAX_EXPLORE_OP_ADD \
    relaxOpAdd(relax, &queue, op_id, fact_id, value)
#include "_heur_relax_explore.h"
}

static void exploreMax(plan_heur_relax_t *relax, const plan_state_t *state)
{
    int goal_id = relax->cref.goal_id;

#define PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL  \
    if (fact_id == goal_id) break
#define PLAN_HEUR_RELAX_EXPLORE_OP_ADD \
    relaxOpMax(relax, &queue, op_id, fact_id, value)
#include "_heur_relax_explore.h"
}

void planHeurRelax(plan_heur_relax_t *relax, const plan_state_t *state)
{
    if (relax->type == PLAN_HEUR_RELAX_TYPE_ADD){
        exploreAdd(relax, state);
    }else{ // PLAN_HEUR_RELAX_TYPE_MAX
        exploreMax(relax, state);
    }
}

static void exploreAddFull(plan_heur_relax_t *relax, const plan_state_t *state)
{
#define PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL
#define PLAN_HEUR_RELAX_EXPLORE_OP_ADD \
    relaxOpAdd(relax, &queue, op_id, fact_id, value)
#include "_heur_relax_explore.h"
}

static void exploreMaxFull(plan_heur_relax_t *relax, const plan_state_t *state)
{
#define PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL
#define PLAN_HEUR_RELAX_EXPLORE_OP_ADD \
    relaxOpMax(relax, &queue, op_id, fact_id, value)
#include "_heur_relax_explore.h"
}

void planHeurRelaxFull(plan_heur_relax_t *relax, const plan_state_t *state)
{
    if (relax->type == PLAN_HEUR_RELAX_TYPE_ADD){
        exploreAddFull(relax, state);
    }else{ // PLAN_HEUR_RELAX_TYPE_MAX
        exploreMaxFull(relax, state);
    }
}

static void setGoalFact(int *goal_fact, int fact_size,
                        const plan_fact_id_t *fid,
                        const plan_part_state_t *goal)
{
    int fact_id;

    bzero(goal_fact, sizeof(int) * fact_size);
    PLAN_FACT_ID_FOR_EACH_PART_STATE(fid, goal, fact_id)
        goal_fact[fact_id] = 1;
}

static plan_cost_t exploreAdd2(plan_heur_relax_t *relax,
                               const plan_state_t *state,
                               const int *goal_fact,
                               int goal_count)
{
    int gc = goal_count;
    int i, len;
    plan_cost_t h;

#define PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL  \
    if (relax->goal_fact[fact_id] && --gc == 0) \
        break
#define PLAN_HEUR_RELAX_EXPLORE_OP_ADD \
    relaxOpAdd(relax, &queue, op_id, fact_id, value)
#include "_heur_relax_explore.h"

    if (gc != 0)
        return PLAN_HEUR_DEAD_END;

    h = 0;
    len = relax->cref.fact_size;
    for (i = 0; i < len; ++i){
        if (goal_fact[i]){
            h += relax->fact[i].value;
        }
    }

    return h;
}

static plan_cost_t exploreMax2(plan_heur_relax_t *relax,
                               const plan_state_t *state,
                               const int *goal_fact,
                               int goal_count)
{
    int gc = goal_count;
    int i, len;
    plan_cost_t h;

#define PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL  \
    if (relax->goal_fact[fact_id] && --gc == 0) \
        break
#define PLAN_HEUR_RELAX_EXPLORE_OP_ADD \
    relaxOpMax(relax, &queue, op_id, fact_id, value)
#include "_heur_relax_explore.h"

    if (gc != 0)
        return PLAN_HEUR_DEAD_END;

    h = 0;
    len = relax->cref.fact_size;
    for (i = 0; i < len; ++i){
        if (goal_fact[i]){
            h = BOR_MAX(h, relax->fact[i].value);
        }
    }

    return h;
}

plan_cost_t planHeurRelax2(plan_heur_relax_t *relax,
                           const plan_state_t *state,
                           const plan_part_state_t *goal)
{
    plan_cost_t h = PLAN_HEUR_DEAD_END;
    int goal_count;

    if (!relax->goal_fact)
        relax->goal_fact = BOR_ALLOC_ARR(int, relax->cref.fact_size);

    // Mark goal facts
    setGoalFact(relax->goal_fact, relax->cref.fact_size,
                &relax->cref.fact_id, goal);

    // Number of goal facts
    goal_count = goal->vals_size;

    // Explore relaxation problem
    if (relax->type == PLAN_HEUR_RELAX_TYPE_ADD){
        h = exploreAdd2(relax, state, relax->goal_fact, goal_count);
    }else{ // PLAN_HEUR_RELAX_TYPE_MAX
        h = exploreMax2(relax, state, relax->goal_fact, goal_count);
    }

    return h;
}


static void incUpdateOpMax(plan_heur_relax_t *relax, int op_id)
{
    plan_heur_relax_op_t *op = relax->op + op_id;
    int fact_id, supp;
    plan_cost_t value;

    supp = op->supp;
    value = relax->fact[supp].value;
    PLAN_ARR_INT_FOR_EACH(relax->cref.op_pre + op_id, fact_id){
        if (relax->fact[fact_id].value > value){
            supp = fact_id;
            value = relax->fact[fact_id].value;
        }
    }

    op->supp = supp;
    op->value = value + op->cost;
}

static void incRelaxOpMax(plan_heur_relax_t *relax,
                          plan_prio_queue_t *queue,
                          int op_id, int fact_id,
                          plan_cost_t fact_value)
{
    plan_heur_relax_op_t *op = relax->op + op_id;

    // Consider this operator only if the enabling fact is the supporter
    // because we know that the supporting fact maximizes the operator's
    // value.
    // Also skip operators that are not reachable.
    if (op->supp != fact_id || op->unsat > 0)
        return;

    // If the value does not correspond to the supporter's value, find out
    // the correct value of the operator and update its effects.
    if (op->value - op->cost != fact_value){
        incUpdateOpMax(relax, op_id);
        relaxAddEffects(relax, queue, op_id, op->value);
    }
}

static void incMax(plan_heur_relax_t *relax,
                   const int *changed_op, int changed_op_size, int goal_id)
{
    plan_prio_queue_t queue;
    int i, fact_id, op_id;
    plan_cost_t value;
    plan_heur_relax_fact_t *fact;

    planPrioQueueInit(&queue);

    for (i = 0; i < changed_op_size; ++i){
        // Skip unreachable operators
        if (relax->op[changed_op[i]].unsat > 0)
            continue;
        relaxAddEffects(relax, &queue, changed_op[i],
                        relax->op[changed_op[i]].value);
    }

    while (!planPrioQueueEmpty(&queue)){
        fact_id = planPrioQueuePop(&queue, &value);
        fact = relax->fact + fact_id;
        if (fact->value != value)
            continue;

        if (fact_id == goal_id)
            break;

        PLAN_ARR_INT_FOR_EACH(relax->cref.fact_pre + fact_id, op_id){
            incRelaxOpMax(relax, &queue, op_id, fact_id, value);
        }
    }

    planPrioQueueFree(&queue);
}

void planHeurRelaxIncMax(plan_heur_relax_t *relax, const int *op, int op_size)
{
    incMax(relax, op, op_size, relax->cref.goal_id);
}

void planHeurRelaxIncMaxFull(plan_heur_relax_t *relax,
                             const int *op, int op_size)
{
    incMax(relax, op, op_size, -1);
}


static int updateFactValue(plan_heur_relax_t *relax, int fact_id)
{
    int i, len, *ops, op_id;
    int value, original_value, supp, num;

    len = relax->cref.fact_eff[fact_id].size;
    if (len == 0)
        return 0;

    // Remember the original value before change
    original_value = relax->fact[fact_id].value;

    // Determine correct value of fact and operator by which it is reached
    ops = relax->cref.fact_eff[fact_id].arr;
    value = INT_MAX;
    supp = relax->fact[fact_id].supp;
    for (num = 0, i = 0; i < len; ++i){
        op_id = ops[i];

        // Consider only operators that are somehow reachable from initial
        // state. Since we assume that full h^max was already run
        // previously, the operators that have not zero .unsat cannot be
        // reachable.
        if (relax->op[op_id].unsat != 0)
            continue;

        ++num;
        if (value > relax->op[op_id].value){
            value = relax->op[op_id].value;
            supp = op_id;
        }
    }

    if (num == 0)
        return 0;

    // Record new value and supporting operator
    relax->fact[fact_id].value = value;
    relax->fact[fact_id].supp  = supp;

    if (value != original_value)
        return -1;
    return 0;
}

static void updateMaxOp(plan_heur_relax_t *relax,
                        plan_prio_queue_t *queue, int op_id)
{
    int fact_id, value;

    PLAN_ARR_INT_FOR_EACH(relax->cref.op_eff + op_id, fact_id){
        // If the operator isn't supporter of the fact, we can skip this
        // fact because there is other operator that minimizes the fact's
        // value
        if (op_id != relax->fact[fact_id].supp)
            continue;

        // Update fact value and insert it into queue if it changed
        if (updateFactValue(relax, fact_id) != 0){
            value = relax->fact[fact_id].value;
            planPrioQueuePush(queue, value, fact_id);
        }
    }
}

static void updateMaxFact(plan_heur_relax_t *relax,
                          plan_prio_queue_t *queue,
                          int fact_id, int fact_value)
{
    plan_heur_relax_op_t *op;
    int op_id;

    PLAN_ARR_INT_FOR_EACH(relax->cref.fact_pre + fact_id, op_id){
        op = relax->op + op_id;
        if (op->unsat == 0 && op->value < op->cost + fact_value){
            op->value = op->cost + fact_value;
            op->supp = fact_id;
            updateMaxOp(relax, queue, op_id);
        }
    }
}

void planHeurRelaxUpdateMaxFull(plan_heur_relax_t *relax,
                                const int *op, int op_size)
{
    int i, fact_id, fact_value;
    plan_prio_queue_t queue;

    planPrioQueueInit(&queue);

    for (i = 0; i < op_size; ++i){
        // Skip unreachable operators
        if (relax->op[op[i]].unsat > 0)
            continue;
        relax->op[op[i]].supp = -1;
        updateMaxOp(relax, &queue, op[i]);
    }


    while (!planPrioQueueEmpty(&queue)){
        fact_id = planPrioQueuePop(&queue, &fact_value);
        if (relax->fact[fact_id].value != fact_value)
            continue;

        updateMaxFact(relax, &queue, fact_id, fact_value);
    }

    planPrioQueueFree(&queue);
}



static void markPlan(plan_heur_relax_t *relax, int fact_id)
{
    plan_heur_relax_fact_t *fact = relax->fact + fact_id;
    int op_id, fid;

    // mark fact in relaxed plan
    relax->plan_fact[fact_id] = 1;

    if (fact->supp < 0)
        return;

    PLAN_ARR_INT_FOR_EACH(relax->cref.op_pre + fact->supp, fid){
        if (!relax->plan_fact[fid]
                && relax->fact[fid].supp != -1){
            markPlan(relax, fid);
        }
    }

    // Mark operator in relaxed plan but mark only the real operators not
    // the artificial ones or those made from condition effects.
    op_id = relax->cref.op_id[fact->supp];
    if (op_id >= 0)
        relax->plan_op[op_id] = 1;
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

void planHeurRelaxMarkPlan2(plan_heur_relax_t *relax,
                            const plan_part_state_t *goal)
{
    int fact_id;

    if (relax->plan_fact == NULL)
        relax->plan_fact = BOR_ALLOC_ARR(int, relax->cref.fact_size);
    if (relax->plan_op == NULL)
        relax->plan_op = BOR_ALLOC_ARR(int, relax->cref.op_size);

    bzero(relax->plan_fact, sizeof(int) * relax->cref.fact_size);
    bzero(relax->plan_op, sizeof(int) * relax->cref.op_size);

    PLAN_FACT_ID_FOR_EACH_PART_STATE(&relax->cref.fact_id, goal, fact_id)
        markPlan(relax, fact_id);
}

int planHeurRelaxAddFakePre(plan_heur_relax_t *relax, int op_id)
{
    int fact_id;

    fact_id = planFactOpCrossRefAddFakePre(&relax->cref, 0);
    planFactOpCrossRefAddPre(&relax->cref, op_id, fact_id);
    relax->op_init[op_id].unsat += 1;

    relax->fact = BOR_REALLOC_ARR(relax->fact, plan_heur_relax_fact_t,
                                  relax->cref.fact_size);
    relax->fact_init = BOR_REALLOC_ARR(relax->fact_init,
                                       plan_heur_relax_fact_t,
                                       relax->cref.fact_size);
    relax->fact_init[fact_id].value = PLAN_COST_MAX;
    relax->fact_init[fact_id].supp = -1;

    if (relax->plan_fact){
        BOR_FREE(relax->plan_fact);
        relax->plan_fact = NULL;
    }

    if (relax->goal_fact){
        BOR_FREE(relax->goal_fact);
        relax->goal_fact = NULL;
    }

    return fact_id;
}

void planHeurRelaxSetFakePreValue(plan_heur_relax_t *relax,
                                  int fact_id, plan_cost_t value)
{
    planFactOpCrossRefSetFakePreValue(&relax->cref, fact_id, value);
}


void planHeurRelaxDumpJustificationDotGraph(const plan_heur_relax_t *relax,
                                            const plan_state_t *_state,
                                            const char *fn)
{
    FILE *fout;
    int i, j, supp;
    int state[planStateSize(_state)];

    fout = fopen(fn, "w");
    if (fout == NULL){
        fprintf(stderr, "Error: Could not open `%s'\n", fn);
        return;
    }

    fprintf(fout, "digraph JustificationGraph {\n");

    for (j = 0; j < planStateSize(_state); ++j){
        state[j] = planFactIdVar(&relax->cref.fact_id, j, planStateGet(_state, j));
    }

    for (i = 0; i < relax->cref.fact_size; ++i){
        fprintf(fout, "%d [label=\"%d;value:%d\"", i, i, relax->fact[i].value);
        for (j = 0; j < planStateSize(_state); ++j){
            if (state[j] == i){
                fprintf(fout, ",color=red,style=bold,rank=\"same\"");
                break;
            }
        }
        if (i == relax->cref.goal_id)
            fprintf(fout, ",color=blue,style=bold");

        fprintf(fout, "];\n");
    }

    for (i = 0; i < relax->cref.op_size; ++i){
        supp = relax->op[i].supp;
        if (supp == -1)
            continue;

        for (j = 0; j < relax->cref.op_eff[i].size; ++j){
            fprintf(fout, "%d -> %d [label=\"op:%d, v:%d\"];\n", supp,
                    relax->cref.op_eff[i].arr[j],
                    i, relax->op[i].value);
        }
    }

    fprintf(fout, "}\n");
    fclose(fout);
}
