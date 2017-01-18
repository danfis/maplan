/***
 * maplan
 * -------
 * Copyright (c)2017 Daniel Fiser <danfis@danfis.cz>,
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
#include "plan/problem_2.h"

static int *genCombs(plan_problem_2_t *p2, const plan_part_state_t *ps,
                     int fact, int add_fact, int *size)
{
    int *comb, fact1, i;

    *size = ps->vals_size + add_fact;
    comb = BOR_ALLOC_ARR(int, ps->vals_size + add_fact);
    if (add_fact)
        comb[0] = fact;
    for (i = 0; i < ps->vals_size; ++i){
        fact1 = planFactIdVar(&p2->fact_id,
                              ps->vals[i].var, ps->vals[i].val);
        comb[i + add_fact] = planFactIdFact2(&p2->fact_id, fact1, fact);
    }

    return comb;
}

static void setEff(plan_problem_2_t *p2,
                   plan_op_2_t *op, const plan_op_t *oop)
{
    const plan_part_state_t *pre = oop->pre;
    const plan_part_state_t *eff = oop->eff;
    int prei, effi;
    int fact, *eff2, size;

    op->eff = planFactIdPartState2(&p2->fact_id, oop->eff, &op->eff_size);

    // Add also combinations of prevails
    for (prei = effi = 0; prei < pre->vals_size && effi < eff->vals_size;){
        if (pre->vals[prei].var == eff->vals[effi].var){
            ++prei;
            ++effi;
        }else if (pre->vals[prei].var < eff->vals[effi].var){
            fact = planFactIdVar(&p2->fact_id, pre->vals[prei].var,
                                               pre->vals[prei].val);
            eff2 = genCombs(p2, eff, fact, 0, &size);
            op->eff = BOR_REALLOC_ARR(op->eff, int, op->eff_size + size);
            memcpy(op->eff + op->eff_size, eff2, sizeof(int) * size);
            op->eff_size += size;
            BOR_FREE(eff2);

            ++prei;
        }else{
            ++effi;
        }
    }

    for (; prei < pre->vals_size; ++prei){
        fact = planFactIdVar(&p2->fact_id, pre->vals[prei].var,
                                           pre->vals[prei].val);
        eff2 = genCombs(p2, eff, fact, 0, &size);
        op->eff = BOR_REALLOC_ARR(op->eff, int, op->eff_size + size);
        memcpy(op->eff + op->eff_size, eff2, sizeof(int) * size);
        op->eff_size += size;
        BOR_FREE(eff2);
    }
}

static void addPre(plan_problem_2_t *p2, int op_id, int pre_id)
{
    plan_fact_2_t *fact;

    fact = p2->fact + pre_id;
    if (fact->pre_op_alloc == fact->pre_op_size){
        if (fact->pre_op_alloc == 0)
            fact->pre_op_alloc = 1;
        fact->pre_op_alloc *= 2;
        fact->pre_op = BOR_REALLOC_ARR(fact->pre_op, int, fact->pre_op_alloc);
    }
    fact->pre_op[fact->pre_op_size++] = op_id;
}

static void addEff(plan_problem_2_t *p2, int op_id, int eff_id)
{
    plan_fact_2_t *fact;

    fact = p2->fact + eff_id;
    if (fact->eff_op_alloc == fact->eff_op_size){
        if (fact->eff_op_alloc == 0)
            fact->eff_op_alloc = 1;
        fact->eff_op_alloc *= 2;
        fact->eff_op = BOR_REALLOC_ARR(fact->eff_op, int, fact->eff_op_alloc);
    }
    fact->eff_op[fact->eff_op_size++] = op_id;
}

static plan_op_2_t *nextOp(plan_problem_2_t *p2,
                           const plan_op_t *oop,
                           int original_op_id, int *op_id)
{
    plan_op_2_t *op;

    if (p2->op_alloc == p2->op_size){
        if (p2->op_alloc == 0)
            p2->op_alloc = 1;
        p2->op_alloc *= 2;
        p2->op = BOR_REALLOC_ARR(p2->op, plan_op_2_t, p2->op_alloc);
    }

    if (op_id != NULL)
        *op_id = p2->op_size;
    op = p2->op + p2->op_size++;
    bzero(op, sizeof(*op));
    op->original_op_id = original_op_id;
    op->parent = -1;
    if (oop != NULL)
        op->cost = oop->cost;

    return op;
}

static void addChild(plan_problem_2_t *p2, int parent, int child)
{
    plan_op_2_t *op = p2->op + parent;

    if (op->child_alloc == op->child_size){
        if (op->child_alloc == 0)
            op->child_alloc = 1;
        op->child_alloc *= 2;
        op->child = BOR_REALLOC_ARR(op->child, int, op->child_alloc);
    }

    op->child[op->child_size++] = child;
}

static void addSlaveOp(plan_problem_2_t *p2,
                       const plan_op_t *oop,
                       int parent,
                       int fact)
{
    plan_op_2_t *op;
    int op_id, i, *pre, size;

    op = nextOp(p2, oop, p2->op[parent].original_op_id, &op_id);
    op->parent = parent;
    addChild(p2, parent, op_id);
    op->eff = genCombs(p2, oop->eff, fact, 0, &op->eff_size);

    pre = genCombs(p2, oop->pre, fact, 1, &size);
    op->pre_size = p2->op[parent].pre_size + size;
    for (i = 0; i < size; ++i)
        addPre(p2, op_id, pre[i]);
    if (pre != NULL)
        BOR_FREE(pre);
    for (i = 0; i < op->eff_size; ++i)
        addEff(p2, op_id, op->eff[i]);
}

static void addSlaveOps(plan_problem_2_t *p2,
                        const plan_problem_t *p,
                        const plan_op_t *oop,
                        int parent)
{
    const plan_part_state_t *pre = oop->pre;
    const plan_part_state_t *eff = oop->eff;
    int prei, effi, var, val;

    for (prei = effi = var = 0; var < p->var_size; ++var){
        if (effi < eff->vals_size && var == eff->vals[effi].var){
            if (prei < pre->vals_size
                    && pre->vals[prei].var == eff->vals[effi].var){
                ++prei;
            }
            ++effi;
            continue;
        }

        if (prei < pre->vals_size && var == pre->vals[prei].var){
            ++prei;
            continue;
        }

        for (val = 0; val < p->var[var].range; ++val){
            addSlaveOp(p2, oop, parent,
                       planFactIdVar(&p2->fact_id, var, val));
        }

        /*
        if (prei < pre->vals_size && var == pre->vals[prei].var)
            ++prei;
        */
    }
}

static void addOp(plan_problem_2_t *p2, const plan_problem_t *p,
                  int original_op_id)
{
    const plan_op_t *oop = p->op + original_op_id;
    plan_op_2_t *op;
    int *pre, op_id, i;

    op = nextOp(p2, oop, original_op_id, &op_id);
    setEff(p2, op, oop);
    pre = planFactIdPartState2(&p2->fact_id, oop->pre, &op->pre_size);
    for (i = 0; i < op->pre_size; ++i)
        addPre(p2, op_id, pre[i]);
    if (pre != NULL)
        BOR_FREE(pre);
    for (i = 0; i < op->eff_size; ++i)
        addEff(p2, op_id, op->eff[i]);

    addSlaveOps(p2, p, p->op + original_op_id, op_id);
}

static void addGoal(plan_problem_2_t *p2, const plan_problem_t *p)
{
    plan_op_2_t *op;
    plan_fact_2_t *fact;
    int op_id, *pre, size, i;

    op = nextOp(p2, NULL, -1, &op_id);
    op->cost = 0;
    pre = planFactIdPartState2(&p2->fact_id, p->goal, &size);
    for (i = 0; i < size; ++i)
        addPre(p2, op_id, pre[i]);
    if (pre != NULL)
        BOR_FREE(pre);
    op->pre_size = size;
    p2->goal_op_id = op_id;

    p2->goal_fact_id = p2->fact_id.fact_size;
    fact = p2->fact + p2->goal_fact_id;
    bzero(fact, sizeof(plan_fact_2_t));
    fact->eff_op_size = fact->eff_op_alloc = 1;
    fact->eff_op = BOR_ALLOC(int);
    fact->eff_op[0] = op_id;

    op->eff_size = 1;
    op->eff = BOR_ALLOC(int);
    op->eff[0] = p2->goal_fact_id;

    p2->goal = planFactIdPartState2(&p2->fact_id, p->goal, &p2->goal_size);
}

void planProblem2Init(plan_problem_2_t *p2, const plan_problem_t *p)
{
    int i;

    bzero(p2, sizeof(*p2));

    planFactIdInit(&p2->fact_id, p->var, p->var_size, PLAN_FACT_ID_H2);
    p2->fact_size = p2->fact_id.fact_size + 1;
    p2->fact = BOR_CALLOC_ARR(plan_fact_2_t, p2->fact_size);

    for (i = 0; i < p->op_size; ++i)
        addOp(p2, p, i);

    addGoal(p2, p);

    // TODO: Free memory
    for (i = 0; i < p2->fact_id.fact_size; ++i){
        fprintf(stderr, "fact[%d]: pre:", i);
        for (int j = 0; j < p2->fact[i].pre_op_size; ++j)
            fprintf(stderr, " %d", p2->fact[i].pre_op[j]);
        fprintf(stderr, "\n");
    }

    for (i = 0; i < p2->op_size; ++i){
        fprintf(stderr, "op[%d]: original_op_id: %d, parent: %d,"
                        " pre_size: %d, eff:",
                        i, p2->op[i].original_op_id,
                        p2->op[i].parent, p2->op[i].pre_size);
        for (int j = 0; j < p2->op[i].eff_size; ++j){
            fprintf(stderr, " %d", p2->op[i].eff[j]);
        }
        if (p2->op[i].original_op_id >= 0)
            fprintf(stderr, ", name: (%s)",
                    p->op[p2->op[i].original_op_id].name);
        fprintf(stderr, "\n");
    }
}

void planProblem2Free(plan_problem_2_t *p2)
{
    int i;

    if (p2->goal != NULL)
        BOR_FREE(p2->goal);

    for (i = 0; i < p2->fact_size; ++i){
        if (p2->fact[i].pre_op != NULL)
            BOR_FREE(p2->fact[i].pre_op);
    }
    if (p2->fact != NULL)
        BOR_FREE(p2->fact);

    for (i = 0; i < p2->op_size; ++i){
        if (p2->op[i].eff != NULL)
            BOR_FREE(p2->op[i].eff);
    }
    if (p2->op)
        BOR_FREE(p2->op);

    planFactIdFree(&p2->fact_id);
}

static void emptyOp(plan_problem_2_t *p2, int op_id)
{
    plan_op_2_t *op = p2->op + op_id;

    if (op->eff != NULL)
        BOR_FREE(op->eff);
    op->eff = NULL;
    op->eff_size = 2;
}

void planProblem2PruneByMutex(plan_problem_2_t *p2, int fact_id)
{
    plan_fact_2_t *fact = p2->fact + fact_id;
    int i;

    for (i = 0; i < fact->pre_op_size; ++i)
        emptyOp(p2, fact->pre_op[i]);

    if (fact->pre_op != NULL)
        BOR_FREE(fact->pre_op);
    fact->pre_op = NULL;
    fact->pre_op_size = fact->pre_op_alloc = 0;
}

void planProblem2PruneEmptyOps(plan_problem_2_t *p2)
{
    plan_fact_2_t *fact;
    int i, fact_id;

    for (fact_id = 0; fact_id < p2->fact_id.fact_size; ++fact_id){
        fact = p2->fact + fact_id;
        for (i = 0; i < fact->pre_op_size; ++i){
            if (p2->op[fact->pre_op[i]].eff_size == 0
                    && fact->pre_op[i] != p2->goal_op_id){
                fact->pre_op[i--] = fact->pre_op[--fact->pre_op_size];
            }
        }
    }
}
